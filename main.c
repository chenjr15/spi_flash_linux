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

	if (input_tx)
		transfer_escaped_string(fd, input_tx);
	else if (input_file)
		transfer_file(fd, input_file);
	else {
		/*Not in normal mode, enter spi flash mode */
		
		if (rx_buff[1] == WINBOND_FLASH){
			if (rx_buff[2] == 0x40)
				printf("Stander SPI\n");
			chip_size_Mbit= 0x01<<(rx_buff[3]-0x11);
			printf("Winbond serial flash found.\n");
			printf("flash size: %dMbit\n", chip_size_Mbit);	
			}
		else{
			close(fd);
			exit(1);
		}
		if (backup_file){
			printf("Start backuping...");
			backup_chip(fd, backup_file, (chip_size_Mbit)<<17);
			printf("Backing up to %s successed!", backup_file);
			
			}
		else if (read_addr_arg)
		{
			uint32_t addr,read_len;
			sscanf( read_addr_arg, "%s:%d", &addr, &read_len);
			read_addr(fd , addr , read_len , NULL,NULL)
			
			
		}
		
	}

  
	
	close(fd);
	return ret;
}
