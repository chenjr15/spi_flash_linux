/*
spi flash reader & writer 
 */

#include "spidev_driver.h"


void pabort(const char *s)
{
	perror(s);
	abort();
}
const char *device = "/dev/spidev0.0";
uint32_t mode=3;
uint8_t bits = 8;
char *input_file;
char *output_file;
uint32_t speed = 1000000;
uint16_t delay;
int verbose;
char *backup_file;
char *read_addr_arg;

uint8_t default_tx[BUFFER_SIZE] = {
	0x9F, 0xFF, 0xFF, 0xFF, 0x48, 0xFF,
};

uint8_t default_rx[BUFFER_SIZE] = {0, };
char *input_tx;

void hex_dump(const void *src, size_t length, size_t line_size,
			 char *prefix)
{
	int i = 0;
	const unsigned char *address = src;
	const unsigned char *line = address;
	unsigned char c;

	printf("%s | ", prefix);
	while (length-- > 0) {
		printf("%02X ", *address++);
		if (!(++i % line_size) || (length == 0 && i % line_size)) {
			if (length == 0) {
				while (i++ % line_size)
					printf("__ ");
			}
			printf(" | ");  /* right close */
			while (line < address) {
				c = *line++;
				printf("%c", (c < 33 || c == 255) ? 0x2E : c);
			}
			printf("\n");
			if (length > 0)
				printf("%s | ", prefix);
		}
	}
}

/*
 *  Unescape - process hexadecimal escape character
 *	  converts shell input "\x23" -> 0x23
 */
int unescape(char *_dst, char *_src, size_t len)
{
	int ret = 0;
	int match;
	char *src = _src;
	char *dst = _dst;
	unsigned int ch;

	while (*src) {
		if (*src == '\\' && *(src+1) == 'x') {
			match = sscanf(src + 2, "%2x", &ch);
			if (!match)
				pabort("malformed input string");

			src += 4;
			*dst++ = (unsigned char)ch;
		} else {
			*dst++ = *src++;
		}
		ret++;
	}
	return ret;
}

void transfer(int fd, uint8_t const *tx, uint8_t const *rx, size_t len)
{
	int ret;
	int out_fd;
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = len,
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	if (mode & SPI_TX_QUAD)
		tr.tx_nbits = 4;
	else if (mode & SPI_TX_DUAL)
		tr.tx_nbits = 2;
	if (mode & SPI_RX_QUAD)
		tr.rx_nbits = 4;
	else if (mode & SPI_RX_DUAL)
		tr.rx_nbits = 2;
	if (!(mode & SPI_LOOP)) {
		if (mode & (SPI_TX_QUAD | SPI_TX_DUAL))
			tr.rx_buf = 0;
		else if (mode & (SPI_RX_QUAD | SPI_RX_DUAL))
			tr.tx_buf = 0;
	}

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't send spi message");

	if (verbose){
		hex_dump(tx, len, 32, "TX");
		hex_dump(rx, len, 32, "RX");
		}
	// if (output_file) {
		// out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		// if (out_fd < 0)
			// pabort("could not open output file");

		// ret = write(out_fd, rx, len);
		// if (ret != len)
			// pabort("not all bytes written to output file");

		// close(out_fd);
	// }

}


void transfer_escaped_string(int fd, char *str)
{
	size_t size = strlen(str);
	uint8_t *tx;
	uint8_t *rx;

	tx = malloc(size);
	if (!tx)
		pabort("can't allocate tx buffer");

	rx = malloc(size);
	if (!rx)
		pabort("can't allocate rx buffer");

	size = unescape((char *)tx, str, size);
	transfer(fd, tx, rx, size);
	free(rx);
	free(tx);
}

int write_enable(int fd){
	default_tx[0]=Write_Enable;
	transfer(fd, default_tx,default_rx,1);
	return 1;
	
}

int is_flash_busy(int fd){
	default_tx[0]=Read_SR1;
	default_tx[1]=Dummy;
	transfer(fd, default_tx,default_rx,2);
	
	return (default_rx[1]&0x01);
}


void backup_chip(int fd , char *out_file, uint32_t flash_size_byte, uint8_t addr_len){

	read_addr(fd , 0x00, flash_size_byte, addr_len,out_file, NULL);
	
	
}



void read_addr(int fd, uint32_t addr, uint32_t len, uint8_t addr_len,char* out_file, uint8_t * buffer ){
	//To record how many bytes datas had been read.
	uint32_t data_counter=0;

	int out_fd;
	int ret;
	int size_temp=0;
	uint8_t cmd[INS_BUF_LEN]={0};
	
	if (out_file){
		out_fd = open(out_file,  O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (out_fd < 0)
			pabort("could not open output file");
		}
	
	do{
		//make up instruction 
		cmd[0] = Read_Data;
		
		cmd[1] = (addr>>((addr_len-1)*8))&0xff;
		cmd[2] = (addr>>((addr_len-2)*8))&0xff;
		cmd[3] = (addr>>((addr_len-3)*8))&0xff;
		if(addr_len-3)
			cmd[4] = addr & 0xff;

		printf("Reading %d byte data at address 0x%x\n", len-data_counter, addr);
		
		//copy instruction into tx buffer 
		memcpy(default_tx,cmd, (addr_len+1));

	#ifdef DEBUG
		printf("DEBUG ON\n");
		hex_dump(default_tx,5,5,"TX");
	#endif

	
		size_temp = ((len -data_counter)>=(BUFFER_SIZE-(addr_len+1)))?(BUFFER_SIZE-(addr_len+1)):(len -data_counter);
		transfer(fd, default_tx, default_rx, size_temp+(addr_len+1));
		if(buffer){
			memcpy(buffer,default_rx+(addr_len+1), size_temp);
			
		}
		if (out_file){
			ret = write(out_fd, default_rx+(addr_len+1), size_temp);
			if (ret != size_temp)
				pabort("not all bytes written to out file");
		}
		if (!(out_file||buffer)){
			hex_dump(default_rx +(addr_len+1), size_temp, 32,"Rx");
		}
		data_counter+=size_temp;
		addr+=size_temp;


		
	}while(data_counter< len);
	if (out_file){
		close(out_fd);

	}
	
	
	
}//end of read_addr()

void sector_erase(int fd, uint32_t addr, uint8_t addr_len){
	uint8_t cmd[INS_BUF_LEN]={0};
	//make up instruction 
	cmd[0] = Sector_Erase;
	
	cmd[1] = (addr>>((addr_len-1)*8))&0xff;
	cmd[2] = (addr>>((addr_len-2)*8))&0xff;
	cmd[3] = (addr>>((addr_len-3)*8))&0xff;
	if(addr_len-3) cmd[4] = addr & 0xff;
	memcpy(default_tx,cmd, (addr_len+1));
	
	transfer(fd, default_tx, default_rx, (addr_len+1));

}//end of  sector_erase()



int  page_program(int fd, uint32_t addr, uint8_t addr_len, uint8_t * data, uint16_t data_len ){
	if (addr&0xFF)return -1;
	if (data_len>256) return -2;
	if ((!addr)||(!data_len)) return -3;
	write_enable(fd);
	makeup_instruction(Page_Program,addr , addr_len, default_tx);
	memcpy( default_tx+(addr_len+1),data,data_len);
	transfer(fd , default_tx,default_rx,data_len+(addr_len+1));
	
	
	return OK;
	
	
}//end of page_program()

int makeup_instruction(uint8_t ins, uint32_t addr, uint8_t addr_len, uint8_t *buffer){
	if ((addr_len!=4) &&(addr_len!=3)) return -1;
	if (buffer == NULL) return -2;
	//make up instruction 
	buffer[0] = ins;
	
	buffer[1] = (addr>>((addr_len-1)*8))&0xff;
	buffer[2] = (addr>>((addr_len-2)*8))&0xff;
	buffer[3] = (addr>>((addr_len-3)*8))&0xff;
	if(addr_len-3) buffer[4] = addr & 0xff;
	return OK;
	
}//end of makeup_instruction()

int sector_program(int fd, uint32_t page_addr,  uint8_t addr_len, uint8_t * data){

	int written_times=0;
	do{
		
		page_program(fd,page_addr, addr_len, data, W25_PAGE_SIZE);
		page_addr+=W25_PAGE_SIZE;
		data+= W25_PAGE_SIZE;
		written_times++;
		
	}while( written_times <(W25_SECTOR_SIZE/W25_PAGE_SIZE) );
	return 1;
	
}//end of sector_program

int write_addr(int fd, uint32_t addr,  uint8_t addr_len, uint8_t * data, uint32_t data_len){
	//dc for writen data counter
	uint32_t sector_addr,next_sector,page_addr,dc;
	uint8_t *template_buffer ;
	uint8_t *buffer_p;
	
	uint32_t write_len;
	dc = 0;
	template_buffer = malloc(W25_SECTOR_SIZE);
	//calculate sector which addr at 
	sector_addr = addr - (addr%W25_SECTOR_SIZE);
	next_sector = sector_addr + W25_PAGE_SIZE;
	//read out data in sector to template_buffer
	read_addr(fd , sector_addr , W25_SECTOR_SIZE , addr_len , NULL , template_buffer);
	//if the end of data is outside the  this sector 
 	if ((addr + data_len)>next_sector)
		write_len = next_sector-(addr+dc);
	//else, the data is ended inside this sector 
	else 
		write_len = W25_SECTOR_SIZE;
	//fill new data into buffer
	memcpy( template_buffer+(addr - sector_addr) , data , write_len);
	
	sector_erase(fd, sector_addr ,addr_len);
	//wait for flash working done
	while(is_flash_busy(fd))
		for(uint16_t i = 1;i;i++);
	//program data into sector 
	sector_program(fd, sector_addr, addr_len , template_buffer);
	dc +=write_len;
	//first sector wrtten done 
	while ((addr + data_len)>next_sector ){
	
		sector_addr +=W25_SECTOR_SIZE;
		next_sector = sector_addr +  W25_PAGE_SIZE;
			//read out data in sector to template_buffer
		read_addr(fd , sector_addr , W25_SECTOR_SIZE , addr_len , NULL , template_buffer);
			//if the end of data is outside the  this sector 
		if ((addr + data_len)>next_sector)
		write_len = next_sector-(addr+dc);
			//else, the data is ended inside this sector 
		else 
			write_len = data_len-dc;
			//fill new data into buffer
		memcpy( template_buffer , data +dc, write_len);
		
		sector_erase(fd, sector_addr ,addr_len);
		//wait for flash working done
		while(is_flash_busy(fd))
			for(uint16_t i = 1;i;i++);
		//program data into sector 
		sector_program(fd, sector_addr, addr_len , template_buffer);
		dc +=write_len;
		
		
	}
		
		
		
		
	free( template_buffer);
	return 1;
	
	
}//end of write_addr()