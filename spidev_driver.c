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
uint32_t speed = 500000;
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

	if (verbose)
		hex_dump(tx, len, 32, "TX");

	if (output_file) {
		out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (out_fd < 0)
			pabort("could not open output file");

		ret = write(out_fd, rx, len);
		if (ret != len)
			pabort("not all bytes written to output file");

		close(out_fd);
	}

	if (verbose)
		hex_dump(rx, len, 32, "RX");
}

void print_usage(const char *prog)
{
	printf("Usage: %s [-DsbdlHOLC3B]\n", prog);
	puts("  -D --device   device to use (default /dev/spidev0.0)\n"
		 "  -s --speed	max speed (Hz)\n"
		 "  -d --delay	delay (usec)\n"
		 "  -b --bpw	  bits per word\n"
		 "  -i --input	input data from a file (e.g. \"test.bin\")\n"
		 "  -o --output   output data to a file (e.g. \"results.bin\")\n"
		 "  -l --loop	 loopback\n"
		 "  -H --cpha	 clock phase\n"
		 "  -O --cpol	 clock polarity\n"
		 "  -L --lsb	  least significant bit first\n"
		 "  -C --cs-high  chip select active high\n"
		 "  -3 --3wire	SI/SO signals shared\n"
		 "  -v --verbose  Verbose (show tx buffer)\n"
		 "  -p			Send data (e.g. \"1234\\xde\\xad\")\n"
		 "  -N --no-cs	no chip select\n"
		 "  -R --ready	slave pulls low to pause\n"
		 "  -2 --dual	 dual transfer\n"
		 "  -4 --quad	 quad transfer\n"
		 "  -B  --backup   backup whole chip to a file (eg. \"backup.bin\") \n"
		 "  -r   --read [addr]:[lenth(byte)]   read some data with length a addr\n"
		 "\t\teg. -r 0x000025:1024 means read 1024 byte data at 0x000025\n");
	exit(1);
}

void parse_opts(int argc, char *argv[])
{
	while (1) {
		const struct option lopts[] = {
			{ "device",  1, 0, 'D' },
			{ "speed",   1, 0, 's' },
			{ "delay",   1, 0, 'd' },
			{ "bpw",	 1, 0, 'b' },
			{ "input",   1, 0, 'i' },
			{ "output",  1, 0, 'o' },
			{ "loop",	0, 0, 'l' },
			{ "cpha",	0, 0, 'H' },
			{ "cpol",	0, 0, 'O' },
			{ "lsb",	 0, 0, 'L' },
			{ "cs-high", 0, 0, 'C' },
			{ "3wire",   0, 0, '3' },
			{ "no-cs",   0, 0, 'N' },
			{ "ready",   0, 0, 'R' },
			{ "dual",	0, 0, '2' },
			{ "verbose", 0, 0, 'v' },
			{ "quad",	0, 0, '4' },
			{"backup", 0, 0, 'B'},
			{"read",0,0,'r'},
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "D:s:d:b:i:o:lHOLC3NR24p:vB:r:",
				lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'D':
			device = optarg;
			break;
		case 's':
			speed = atoi(optarg);
			break;
		case 'd':
			delay = atoi(optarg);
			break;
		case 'b':
			bits = atoi(optarg);
			break;
		case 'i':
			input_file = optarg;
			break;
		case 'o':
			output_file = optarg;
			break;
		case 'l':
			mode |= SPI_LOOP;
			break;
		case 'H':
			mode |= SPI_CPHA;
			break;
		case 'O':
			mode |= SPI_CPOL;
			break;
		case 'L':
			mode |= SPI_LSB_FIRST;
			break;
		case 'C':
			mode |= SPI_CS_HIGH;
			break;
		case '3':
			mode |= SPI_3WIRE;
			break;
		case 'N':
			mode |= SPI_NO_CS;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'R':
			mode |= SPI_READY;
			break;
		case 'p':
			input_tx = optarg;
			break;
		case '2':
			mode |= SPI_TX_DUAL;
			break;
		case '4':
			mode |= SPI_TX_QUAD;
			break;
		case 'B':
			backup_file = optarg;
			break;
		case 'r':
			read_addr_arg = optarg;
			break;
		default:
			print_usage(argv[0]);
			break;
		}
	}
	if (mode & SPI_LOOP) {
		if (mode & SPI_TX_DUAL)
			mode |= SPI_RX_DUAL;
		if (mode & SPI_TX_QUAD)
			mode |= SPI_RX_QUAD;
	}
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



int is_flash_busy(int fd){
	default_tx[0]=Read_SR1;
	default_tx[1]=Dummy;
	transfer(fd, default_tx,default_rx,2);
	
	return (default_rx[1]&0x01);
}


void backup_chip(int fd , char *out_file, uint32_t flash_size_byte){
	uint32_t data_counter=0;
	int out_fd;
	int ret;
	int instruction_len;
	while(is_flash_busy( fd));

	sprintf(default_tx,"%c%c%c%c",Read_Data,0,0,0 );
	instruction_len=4;
	
	out_fd = open(out_file,  O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (out_fd < 0)
			pabort("could not open backup file");
	
	while(data_counter< flash_size_byte){
		transfer(fd, default_tx,default_rx,BUFFER_SIZE-instruction_len);
		sprintf(default_tx,"%c%c%c%c",Read_Data,0,0,0 );
		
		ret = write(out_fd, default_rx+instruction_len, BUFFER_SIZE);
		if (ret != BUFFER_SIZE)
		pabort("not all bytes written to backup file");
		data_counter+=BUFFER_SIZE;
		if ((100*data_counter/flash_size_byte)%10==0)
			printf("%d%%...\n",(100*data_counter/flash_size_byte));
		
	}
	
	close(out_fd);
	
	
	
}

/**
 *  \brief Read data from flash
 *  
 *  \param [in] fd spi device file descriptor
 *  \param [in] addr address for reading data 3/4 byte 
 *  \param [in] len Data length in byte 
 *  \param [in] addr_len Address length 3 or 4  only 
 *  \param [in] out_file File name for save the data which read from flash
 *  \param [in] buffer Buffer pointer  for save the data which read from flash
 *  \return nothing returns
 *  
 *  \details Details
 */

void read_addr(int fd, uint32_t addr, uint32_t len, uint8_t addr_len,char* out_file, char * buffer ){
	//3byte addr only now.
	
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

		printf("Reading %d byte data at address 0x%x\n", len, addr);
		
		//copy instruction into tx buffer 
		memcpy(default_tx,cmd, (addr_len+1));

	#ifdef DEBUG
		printf("DEBUG ON\n");
		hex_dump(default_tx,5,5,"TX");
	#endif

	//sending instruction
	//transfer(fd,default_tx,default_rx,4);
	//receive data
	
		size_temp = ((len -data_counter)>=(BUFFER_SIZE-(addr_len+1)))?(BUFFER_SIZE-(addr_len+1)):(len -data_counter);
		transfer(fd, default_tx, default_rx, size_temp+(addr_len+1));
		if(buffer){
			memcpy(buffer,default_rx+(addr_len+1), size_temp);
			
		}
		if (out_file){
			ret = write(out_fd, default_rx+(addr_len+1), (BUFFER_SIZE-(addr_len+1)));
			if (ret != (BUFFER_SIZE-(addr_len+1)))
			pabort("not all bytes written to out file");
		}
		if (!(out_file||buffer)){
			hex_dump(default_rx +(addr_len+1), size_temp, 32,"Rx");
		}
		data_counter+=size_temp;
		addr+=size_temp;
		// if(data_counter< len){
			//
			// cmd[0] = Read_Data;
			// cmd[1] = (addr>>((addr_len-1)*8))&0xff;
			// cmd[2] = (addr>>((addr_len-2)*8))&0xff;
			// cmd[3] = (addr>>((addr_len-3)*8))&0xff;
			// if(addr_len-3)
				// cmd[4] = addr & 0xff;

			// printf("Reading %d byte data at address 0x%x\n", len, addr);
			// memcpy(default_tx,cmd, (addr_len+1));

			// }

		
	}while(data_counter< len);
	if (out_file){
		close(out_fd);

	}
	
	
	
}

