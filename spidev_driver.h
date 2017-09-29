/*
spi flash reader & writer 
 */
#ifndef __SPIDEV_DRIVER_H__
#define __SPIDEV_DRIVER_H__

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define TRANSFER_BYTE(b) transfer(fd ,b,b,1)
#define BUFFER_SIZE 2052
#define INS_BUF_LEN 10
#define VERBOSE 1
#define W25_SECTOR_SIZE 4096
#define W25_PAGE_SIZE 256
#define OK 1


//指令定义
#define Write_Enable 0x06                  //写使能
#define Write_Enable_Volatile_SR 0x50      //易失性寄存器写使能
#define Write_Disable 0x04                 //写禁止
#define Read_SR1 0x05                      //读状态寄存器1
#define Read_SR2 0x35                      //读状态寄存器2  
#define Write_Status_Register 0x01         //写状态寄存器
#define Read_Data 0x03                     //读数据                   
#define Page_Program 0x02                  //页编程命令 
#define Sector_Erase 0x20                  //扇区擦除 
#define Block_Erase_32K 0x52               //32K块擦除
#define Block_Erase_64K 0xd8               //64K块擦除 
#define Chip_Erase 0xc7                    //芯片擦除 0x60
#define Power_Down 0xB9                    //Power_Down模式
#define Release_Power_Down 0xab            //恢复power_down模式
#define Read_Chip_ID 0x90                  //读取设备ID
#define Read_Jedec_Id 0x9f
#define Dummy 0xff
#define WINBOND_FLASH 0xef

extern void pabort(const char *s);

extern const char *device ;
extern uint32_t mode;
extern uint8_t bits ;
extern char *input_file;
extern char *output_file;
extern uint32_t speed ;
extern uint16_t delay;
extern int verbose;
extern char* backup_file;
extern char *read_addr_arg;

/*
uint8_t default_tx[BUFFER_SIZE];

uint8_t default_rx[BUFFER_SIZE] ;

*/
// char *input_tx;

extern void hex_dump(const void *src, size_t length, size_t line_size,
		     char *prefix);

/*
 *  Unescape - process hexadecimal escape character
 *      converts shell input "\x23" -> 0x23
 */
extern int unescape(char *_dst, char *_src, size_t len);


extern void transfer(int fd, uint8_t const *tx, uint8_t const *rx, size_t len);

//extern void print_usage(const char *prog);
//extern void parse_opts(int argc, char *argv[]);

extern void transfer_escaped_string(int fd, char *str);
extern void backup_chip(int fd , char *out_file, uint32_t flash_size_byte , uint8_t addr_len);


/**
 *  \brief get the busy status of flash 
 *  
 *  \param [in] fd Parameter_Description
 *  \return value of busy bit of status register
 *  
 *  \details Details
 */
extern int is_flash_busy(int fd);

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
extern void read_addr(int fd, uint32_t addr, uint32_t len, uint8_t addr_len, uint8_t* out_file, char * buffer );
extern void write_chip();
extern int  page_program(int fd, uint32_t addr, uint8_t addr_len, uint8_t * data, uint16_t data_len  );
extern void sector_erase(int fd, uint32_t addr, uint8_t addr_len);

/**
 *  \brief Send Write_Enable byte to flash
 *  
 *  \param [in] fd Parameter_Description
 *  \return useless
 *  
 *  \details Details
 */
extern int write_enable(int fd);

extern int makeup_instruction(uint8_t ins, uint32_t addr, uint8_t addr_len, uint8_t *buffer);

extern int write_addr(int fd, uint32_t addr,  uint8_t addr_len, uint8_t * data, uint32_t data_len);
extern int sector_program(int fd, uint32_t addr,  uint8_t addr_len, uint8_t * data);

typedef struct {
	uint8_t byte0;
	uint8_t byte1;
	uint8_t byte2;
	uint8_t byte3;	
} fourbyte;



#endif
