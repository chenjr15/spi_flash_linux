/*
spi flash reader & writer 
 */

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "spidev_driver.h"



uint8_t tx_buff[BUFFER_SIZE] = {0, };

uint8_t rx_buff[BUFFER_SIZE] = {0, };
char *input_tx;


void print_usage(const char *prog)
{
	printf("Usage: %s [-DsbdlHOLC3B]\n", prog);
	puts("  -D --device   device to use (default /dev/spidev0.0)\n"
		 "  -s --speed	max speed (Hz)\n"
		 "  -d --delay	delay (usec)\n"
		// "  -b --bpw	  bits per word\n"
		 "  -i --input	input data from a file (e.g. \"test.bin\")\n"
		 "  -o --output   output data to a file (e.g. \"results.bin\")\n"
		// "  -l --loop	 loopback\n"
		// "  -H --cpha	 clock phase\n"
		// "  -O --cpol	 clock polarity\n"
		// "  -L --lsb	  least significant bit first\n"
		// "  -C --cs-high  chip select active high\n"
		// "  -3 --3wire	SI/SO signals shared\n"
		 "  -v --verbose  Verbose (show tx buffer)\n"
		 "  -p			Send data (e.g. \"1234\\xde\\xad\")\n"
		 "  -N --no-cs	no chip select\n"
		 "  -R --ready	slave pulls low to pause\n"
		// "  -2 --dual	 dual transfer\n"
		// "  -4 --quad	 quad transfer\n"
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
			//{ "bpw",	 1, 0, 'b' },
			{ "input",   1, 0, 'i' },
			{ "output",  1, 0, 'o' },
			//{ "loop",	0, 0, 'l' },
			//{ "cpha",	0, 0, 'H' },
			//{ "cpol",	0, 0, 'O' },
			//{ "lsb",	 0, 0, 'L' },
			//{ "cs-high", 0, 0, 'C' },
			//{ "3wire",   0, 0, '3' },
			{ "no-cs",   0, 0, 'N' },
			{ "ready",   0, 0, 'R' },
			//{ "dual",	0, 0, '2' },
			{ "verbose", 0, 0, 'v' },
			//{ "quad",	0, 0, '4' },
			{"backup", 0, 0, 'B'},
			{"read",0,0,'r'},
			{"write",0,0,'w'},
			{ NULL, 0, 0, 0 },
		};
		int c;

		//c = getopt_long(argc, argv, "D:s:d:b:i:o:lHOLC3NR24p:vB:r:",
				//lopts, NULL);
				c= getopt_long(argc, argv, "D:s:d:b:i:o:NRp:vB:r:w:",
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
/*
		case 'b':
			bits = atoi(optarg);
			break;
		*/
		case 'i':
			input_file = optarg;
			break;
		case 'o':
			output_file = optarg;
			break;
			/*
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
			*/
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
/*
		case '2':
			mode |= SPI_TX_DUAL;
			break;
		case '4':
			mode |= SPI_TX_QUAD;
			break;
			*/
		case 'B':
			backup_file = optarg;
			break;
		case 'r':
			read_addr_arg = optarg;
			break;
		case 'w':
			write_addr_arg=optarg;
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

int main(int argc, char *argv[])
{
	int ret = 0;
	int fd;
	uint32_t chip_size_Mbit;

	parse_opts(argc, argv);

	fd = open(device, O_RDWR);
	if (fd < 0)
		pabort("can't open device");

	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE32, &mode);
	if (ret == -1)
		pabort("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE32, &mode);
	if (ret == -1)
		pabort("can't get spi mode");

	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");
   
	printf("spi mode: 0x%x\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);
	
	 tx_buff[0]=Read_Jedec_Id;
	 transfer(fd,tx_buff,rx_buff,4);
	if (input_tx && input_file)
		pabort("only one of -p and --input may be selected");
		/*Not in normal mode, enter spi flash mode */
		
	if (rx_buff[1] == WINBOND_FLASH){
		if (rx_buff[2] == 0x40)
			printf("Stander SPI\n");
		chip_size_Mbit= 0x01<<(rx_buff[3]-0x11);
		printf("Winbond serial flash found.\n");
		printf("flash size: %dMbit(%dMB)\n", chip_size_Mbit,chip_size_Mbit>>3);
		if (read_addr_arg&&write_addr_arg)
				pabort("conflict args\n");
		if (write_addr_arg&&(!input_tx))
				pabort("-w(--write) should be used with -p\n");

		if (backup_file){
			printf("Start backuping...");
			backup_chip(fd, backup_file, (chip_size_Mbit)<<17,3);
			printf("Backing up to %s successed!", backup_file);

			}
		else if (read_addr_arg)
		{
			uint32_t addr,read_len;
			sscanf( read_addr_arg, "0x%x:%d", &addr, &read_len);
			read_addr(fd , addr , read_len , 3,  NULL,NULL);
		}
		else if (write_addr_arg&&input_tx){
				uint32_t w_len;
				uint8_t* wbuffer;
				uint32_t waddr;
				sscanf(write_addr_arg,"0x%d:%d",&waddr,&w_len);
				wbuffer=(uint8_t*)malloc(w_len);
				//Attention! This unescape function can't restrict the input len as we see. The len in -w can't not smaller than length of unescaped string, or it will cause crash!
				unescape(wbuffer,input_tx, strlen(input_tx));
				printf("Writing %d byte datas into flash at 0x%x\n",w_len,waddr);
				write_addr(fd, waddr,3,wbuffer,w_len);
				free(wbuffer);
		}
	
		}
  
	
	close(fd);
	return ret;
}
