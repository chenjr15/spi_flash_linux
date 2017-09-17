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
#define BUFFER_SIZE 256

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
#define Dummy 0xff

extern void pabort(const char *s)

extern const char *device ;
extern uint32_t mode;
extern uint8_t bits ;
extern char *input_file;
extern char *output_file;
extern uint32_t speed ;
extern uint16_t delay;
extern int verbose;

/*
uint8_t default_tx[BUFFER_SIZE];

uint8_t default_rx[BUFFER_SIZE] ;

*/
char *input_tx;

extern void hex_dump(const void *src, size_t length, size_t line_size,
		     char *prefix)

/*
 *  Unescape - process hexadecimal escape character
 *      converts shell input "\x23" -> 0x23
 */
extern int unescape(char *_dst, char *_src, size_t len)


extern void transfer(int fd, uint8_t const *tx, uint8_t const *rx, size_t len)


extern void print_usage(const char *prog)


extern void parse_opts(int argc, char *argv[])

extern void transfer_escaped_string(int fd, char *str)


#endif