/*
 * Lumifyx Tx6 LED Display control driver
 *
 * Written by: John <john.zhao@lumifyx.com>
 *
 * Copyright (C) 2022 Lingye Technologies Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LUMIFYX_FD655H__
#define __LUMIFYX_FD655H__

#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/io.h>

#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/of.h>

#include <linux/pinctrl/consumer.h>

#include <linux/time.h>


typedef unsigned char   u_int8;
typedef unsigned short  u_int16;	
typedef unsigned long 	u_int32;



#if 1
#define pr_dbg(args...) printk(KERN_ALERT "FD655: " args)
#else
#define pr_dbg(args...)
#endif

#define pr_error(args...) printk(KERN_ALERT "FD655: " args)

#ifndef CONFIG_OF
#define CONFIG_OF
#endif



#define MOD_NAME_CLK       "fd655"
#define MOD_NAME_DAT       "fd655_dat"
#define DEV_NAME           "fd655_dev"


#define 	FD655_DELAY_1us						udelay(4)					    	
#define 	FD655_DELAY_LOW		     	FD628_DELAY_1us                     		       
#define		FD655_DELAY_HIGH     	 	FD628_DELAY_1us 	   									
#define  	FD655_DELAY_BUF		 		 	FD628_DELAY_1us
#define  	FD655_DELAY_STB					FD628_DELAY_1us





typedef struct _tag_fd655_dev{
	int clk_pin;    
	int dat_pin;
	char wbuf[5];
	struct class sysfs;
    int vplay_flag;
    int vpause_flag;
    int test_flag;
	int  FD655SYS_ON_OF;
	int bit0_status;
	int bit1_status;
	int bit2_status;
	int bit3_status;
	int bit4_status;
	int bit5_status;
	int bit6_status;
}FD655_DEV;

#define LEDMAPNUM 63

 /** Character conversion of digital tube display code*/
typedef struct _led_bitmap
{
	u_int8 character;
	u_int8 bitmap;
} led_bitmap;

/** Character conversion of digital tube display code array*/
static const led_bitmap LED_decode_tab[LEDMAPNUM] = 
{

	{'0', 0x3F}, {'1', 0x06}, {'2', 0x5B}, {'3', 0x4F},
	{'4', 0x66}, {'5', 0x6D}, {'6', 0x7D}, {'7', 0x07},
	{'8', 0x7F}, {'9', 0x6F}, {'a', 0x77}, {'A', 0x77},
	{'b', 0x7C}, {'B', 0x7C}, {'c', 0x58}, {'C', 0x39},
	{'d', 0x5E}, {'D', 0x5E}, {'e', 0x79}, {'E', 0x79},
	{'f', 0x71}, {'F', 0x71}, {'I', 0X60}, {'i', 0x60},
	{'L', 0x38}, {'l', 0x38}, {'r', 0x38}, {'R', 0x38},
	{'n', 0x54}, {'N', 0x37}, {'O', 0x3F}, {'o', 0x3f},
	{'p', 0xf3}, {'P', 0x38}, {'S', 0x6D}, {'s', 0x6d},
	{'y', 0x6e}, {'Y', 0x6E}, {'_', 0x08}, {0,   0x3F}, 
	{1,   0x06}, {2,   0x5B}, {3,   0x4F}, {4,   0x66}, 
	{5,   0x6D}, {6,   0x7D}, {7,   0x07}, {8,   0x7F}, 
	{9,   0x6F}, {'!', 0X01}, {'@', 0X02}, {'#', 0X04},
	{'$', 0X08}, {':', 0X10}, {'^', 0X20}, {'&', 0X40},
	{0xC5,0X00}, {0x3b,0x18}, {0xc4,0x08}, {0x3c,0x14},
	{0xc3,0x04}, {0x3d,0x1c}, {0xc2,0x0c},
#if 0
    {'0', 0x3F}, {'1', 0x30}, {'2', 0x5B}, {'3', 0x79},
	{'4', 0x74}, {'5', 0x6D}, {'6', 0x6F}, {'7', 0x38},
	{'8', 0x7F}, {'9', 0x7D}, {'a', 0x77}, {'A', 0x77},
	{'b', 0x7C}, {'B', 0x7C}, {'c', 0x58}, {'C', 0x39},
	{'d', 0x5E}, {'D', 0x5E}, {'e', 0x79}, {'E', 0x79},
	{'f', 0x71}, {'F', 0x71}, {':', 0X10}, {0,   0x3F}, 
	{1,   0x30}, {2,   0x5B}, {3,   0x79}, {4,   0x74}, 
	{5,   0x6D}, {6,   0x6F}, {7,   0x38}, {8,   0x7F}, 
	{9,   0x7D}

#endif 	
};



/* ****************************************************************/
#define READKEY 	0x4f       
#define SET     	0x48       

#define DISP		0x01		
#define SLEEP		0x80		
#define KEYB		0x02		

#define INTENS0		0x00		

#define INTENS1		0x20		
#define INTENS2		0x40		

#define INTENS3		0x60	
								


#define FD655SYSON 	( DISP|KEYB |INTENS0 ) 
#define FD655SYSOFF  0x00

#define DIG1		0x68		
#define DIG2		0x6a		
#define DIG3		0x6c		
#define DIG4		0x6e	

#define DIG5		0x66		



void FD655_Command( u_int8 cmd ,FD655_DEV *dev)	;

void FD655_Disp(u_int8 address ,u_int8 dat,FD655_DEV *dev);

void LedShow( u_int8 *acFPStr, FD655_DEV *dev);



























#endif
