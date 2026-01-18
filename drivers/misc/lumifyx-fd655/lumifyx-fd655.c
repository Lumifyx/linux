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


#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/ioctl.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <linux/of_gpio.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/keyboard.h>
#include <linux/ioport.h>
#include <linux/timer.h>
#include <linux/clk.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>

#include "lumifyx-fd655.h"

static DEFINE_MUTEX(showbit_mutex_lock);

static struct class *fd65_class;
unsigned char  val = 0x11;
static FD655_DEV *pdata = NULL;


static u_int8 Led_Get_Code(char cTemp)
{
	u_int8 i, bitmap=0x00;

	for(i=0; i<LEDMAPNUM; i++)
	{
		if(LED_decode_tab[i].character == cTemp)
		{
			bitmap = LED_decode_tab[i].bitmap;
			break;
		}
	}

	return bitmap;
}


static void FD655_Start( FD655_DEV *dev )		 	
{
//	FD655_SDA_D_OUT;			
//	FD655_SCL_D_OUT;				
//	FD655_SDA_SET;  			
    gpio_direction_output(dev->dat_pin, 1);
//	FD655_SCL_SET;
	gpio_direction_output(dev->clk_pin, 1);
	FD655_DELAY_1us;
//	FD655_SDA_CLR;					
    gpio_direction_output(dev->dat_pin, 0);
	FD655_DELAY_1us;      
//	FD655_SCL_CLR;
    gpio_direction_output(dev->clk_pin, 0);
}					     


static void FD655_Stop( FD655_DEV *dev )	
{
//	FD655_SDA_D_OUT;				
//	FD655_SCL_D_OUT;				 
//	FD655_SDA_CLR;
    gpio_direction_output(dev->dat_pin, 0);
	FD655_DELAY_1us;
//	FD655_SCL_SET;
    gpio_direction_output(dev->clk_pin, 1);
	FD655_DELAY_1us;
//	FD655_SDA_SET;				
    gpio_direction_output(dev->dat_pin, 1);
    FD655_DELAY_1us;
}



static void FD655_Writebyte(u_int8 dat,FD655_DEV *dev)		 
{
	u_int8 i;
	for( i = 0; i != 8; i++ )
	{
		if( dat & 0x80 ) 
		{
		   // FD655_SDA_SET;
		   gpio_direction_output(dev->dat_pin, 1);
		}
		else 
		{
		   // FD655_SDA_CLR;
		   gpio_direction_output(dev->dat_pin, 0);
		}
		FD655_DELAY_1us;
	//	FD655_SCL_SET;
	    gpio_direction_output(dev->clk_pin, 1);
		dat <<= 1;
		FD655_DELAY_1us;  
	//	FD655_SCL_CLR;
	    gpio_direction_output(dev->clk_pin, 0);
	}
	//FD655_SDA_SET;
	gpio_direction_output(dev->dat_pin, 1);
	FD655_DELAY_1us;
	//FD655_SCL_SET;
	gpio_direction_output(dev->clk_pin, 1);
	FD655_DELAY_1us;
	//FD655_SCL_CLR;
	gpio_direction_output(dev->clk_pin, 0);
}


#if 0
static u_int8 FD655_Readbyte(FD655_DEV *dev)				  
{
	u_int8 dat,i;
	//FD655_SDA_D_IN;
	//printk(KERN_ERR "---xfwu-------%s---%d-\n",__func__,__LINE__);
	dat = 0;
	for( i = 0; i != 8; i++ )
	{
		FD655_DELAY_1us;  // 可选延时
		//FD655_SCL_SET;
		gpio_direction_output(dev->clk_pin, 1);
		FD655_DELAY_1us;  // 可选延时
		dat <<= 1;
		//if( FD655_SDA_IN ) 
		if( gpio_get_value(dev->dat_pin) ) 
			dat++;
		//FD655_SCL_CLR;
		gpio_direction_output(dev->clk_pin, 0);
	}
//	FD655_SDA_D_OUT;
//	FD655_SDA_SET;
    gpio_direction_output(dev->dat_pin, 1);
	FD655_DELAY_1us;
//	FD655_SCL_SET;
    gpio_direction_output(dev->clk_pin, 1);
	FD655_DELAY_1us;
//	FD655_SCL_CLR;
    gpio_direction_output(dev->clk_pin, 0);
	return dat;
}
#endif

void FD655_Command( u_int8 cmd ,FD655_DEV *dev)		  		
{								
	FD655_Start(dev);
	FD655_Writebyte(SET,dev);
	FD655_Writebyte(cmd,dev);
	FD655_Stop(dev);	
}

void FD655_Disp(u_int8 address ,u_int8 dat,FD655_DEV *dev)
{
	FD655_Start(dev);
	FD655_Writebyte(address,dev);
	FD655_Writebyte(dat,dev);
	FD655_Stop(dev);
}


void LedShow( u_int8 *acFPStr, FD655_DEV *dev)
{
	u_int8 i, iLenth;
	u_int8 dat[5]={0};
	if( strcmp(acFPStr, "") == 0 )
	{
		return;
	}
	iLenth = strlen(acFPStr);
	if(iLenth>5)
		iLenth = 5;
	
	for(i=0; i<iLenth; i++)
	{
		dat[i] = Led_Get_Code(acFPStr[i]);
	}
	//Send display data
	
	FD655_Disp(DIG1,dat[0],dev);
	FD655_Disp(DIG2,dat[1],dev);
	FD655_Disp(DIG3,dat[2],dev);
	FD655_Disp(DIG4,dat[3],dev);
	FD655_Disp(DIG5,dat[4],dev);
}

void fd655_disp_onoff(int i)
{

	if(i==1){
		FD655_Command(FD655SYSON,pdata);
		printk("fd655_disp on now.............................\r\n");
	}else{
		
		FD655_Command(0x00,pdata);
		printk("fd655_disp off now.............................\r\n");
	}
    return;
}

EXPORT_SYMBOL(fd655_disp_onoff);

/**************************
 * bit0
 * ***********************/
void fd655_bit0onoff(int bit0){
	mutex_lock(&showbit_mutex_lock);
	FD655_Command(FD655SYSON,pdata);

	if(bit0==1){
		val |=(1<<0);
	}else {
		val &=~(1<<0);
	}
	FD655_Disp(DIG5,val,pdata);
	mutex_unlock(&showbit_mutex_lock);
}

static ssize_t store_fd655_bt0(struct class *cls,struct class_attribute *attr,
               const char *buf, size_t count){
	int bit0_status;

	if(kstrtoint(buf, 0, &bit0_status)){
		return -EINVAL;
	}
                    
	if(bit0_status >=0 && bit0_status<2){
		pdata->bit0_status = bit0_status;
		fd655_bit0onoff(pdata->bit0_status);
	}

	return count;
}

/***********************************
 *bit1
 **********************************/
void fd655_bit1onoff(int bit1){
	mutex_lock(&showbit_mutex_lock);
	FD655_Command(FD655SYSON,pdata);
	if(bit1==1){
		val |=(1<<1);
	}else {
		val &=~(1<<1);
	}
	FD655_Disp(DIG5,val,pdata);
	mutex_unlock(&showbit_mutex_lock);
}

static ssize_t store_fd655_bt1(struct class *cls,struct class_attribute *attr,
               const char *buf, size_t count){
				   int bit1_status;
				  
               if (kstrtoint(buf, 0, &bit1_status)){
                       return -EINVAL;
			   }
                      
				if(bit1_status >=0 && bit1_status<2){
                	pdata->bit1_status = bit1_status;
					fd655_bit1onoff(pdata->bit1_status);
				}
			
				 return count;
}

/***********************************
 * bit2
 * ********************************/
void fd655_bit2onoff(int bit2){

	mutex_lock(&showbit_mutex_lock);
	FD655_Command(FD655SYSON,pdata);
	if(bit2==1){
		val |=(1<<2);
	}else {
		val &=~(1<<2);
	}
	FD655_Disp(DIG5,val,pdata);
	mutex_unlock(&showbit_mutex_lock);
}

static ssize_t store_fd655_bt2(struct class *cls,struct class_attribute *attr,
               const char *buf, size_t count){
				   int bit2_status;
               if (kstrtoint(buf, 0, &bit2_status)){
                       return -EINVAL;
			   }
                 
				if(bit2_status >=0 && bit2_status < 2){
                    pdata->bit2_status = bit2_status; 
					fd655_bit2onoff(pdata->bit2_status);
				}
				
				 return count;
}

/***********************************
 * bit3
 * ********************************/

void fd655_bit3onoff(int bit3){
	mutex_lock(&showbit_mutex_lock);
	FD655_Command(FD655SYSON,pdata);
	if(bit3==1){
		val |=(1<<3);
	}else {
		val &=~(1<<3);
	}
	FD655_Disp(DIG5,val,pdata);
	mutex_unlock(&showbit_mutex_lock);

}

static ssize_t store_fd655_bt3(struct class *cls,struct class_attribute *attr,
               const char *buf, size_t count){
                   int bit3_status=0;
               if (kstrtoint(buf, 0, &bit3_status)){
                       return -EINVAL;
			   }

                if(bit3_status >=0 && bit3_status < 2){
                    pdata->bit3_status = bit3_status;
                    fd655_bit3onoff(pdata->bit3_status);
                }

                 return count;
}

/***********************************
 * bit4
 * ********************************/

void fd655_bit4onoff(int bit4){
	mutex_lock(&showbit_mutex_lock);
	FD655_Command(FD655SYSON,pdata);
	if(bit4==1){
		val |=(1<<4);
	}else {
		val &=~(1<<4);
	}
	FD655_Disp(DIG5,val,pdata);
	mutex_unlock(&showbit_mutex_lock);
}

static ssize_t store_fd655_bt4(struct class *cls,struct class_attribute *attr,
               const char *buf, size_t count){
                   int bit4_status;
               if (kstrtoint(buf, 0, &bit4_status)){
                      return -EINVAL;
			    }

                if(bit4_status >=0 && bit4_status < 2){
                    pdata->bit4_status = bit4_status;
                    fd655_bit4onoff(pdata->bit4_status);
                }

                 return count;
}


/****************************
 * bit5
 * *************************/
void fd655_bit5onoff(int bit5){
	mutex_lock(&showbit_mutex_lock);
	FD655_Command(FD655SYSON,pdata);
	if(bit5==1){
		val |=(1<<5);
	}else {
		val &=~(1<<5);
	}
	FD655_Disp(DIG5,val,pdata);
	mutex_unlock(&showbit_mutex_lock);
}

static ssize_t store_fd655_bt5(struct class *cls,struct class_attribute *attr,
               const char *buf, size_t count){
                   int bit5_status=0;

               if (kstrtoint(buf, 0, &bit5_status)){
                       return -EINVAL;
			    }

                if(bit5_status >=0 && bit5_status < 2){
                    pdata->bit5_status = bit5_status;
                    fd655_bit5onoff(pdata->bit5_status);
                }

                 return count;
}

/*****************************
 * bit6
 * **************************/

void fd655_bit6onoff(int bit6){
	mutex_lock(&showbit_mutex_lock);
	FD655_Command(FD655SYSON,pdata);
	if(bit6==1){
		val |=(1<<6);
	}else {
		val &=~(1<<6);
	}
	FD655_Disp(DIG5,val,pdata);
	mutex_unlock(&showbit_mutex_lock);
}

static ssize_t store_fd655_bt6(struct class *cls,struct class_attribute *attr,
               const char *buf, size_t count){
                   int bit6_status=0;
               if (kstrtoint(buf, 0, &bit6_status)){
                       return -EINVAL;
			    }

                if(bit6_status >=0 && bit6_status < 2){
                    pdata->bit6_status = bit6_status;
                    fd655_bit6onoff(pdata->bit6_status);
                }

                 return count;
}

static ssize_t show_fd655_bt0(struct class *cls,struct class_attribute *attr,
               char *buf){
    return sprintf(buf, "fd655_bit0_status: %d\n", pdata->bit0_status);
}

static ssize_t show_fd655_bt1(struct class *cls,struct class_attribute *attr,
                char *buf){
    return sprintf(buf, "fd655_bit1_status: %d\n", pdata->bit1_status);
}

static ssize_t show_fd655_bt2(struct class *cls,struct class_attribute *attr,
                char *buf){
    return sprintf(buf, "fd655_bit2_status: %d\n", pdata->bit2_status);
}

static ssize_t show_fd655_bt3(struct class *cls,struct class_attribute *attr,
               char *buf){
    return sprintf(buf, "fd655_bit3_status: %d\n", pdata->bit3_status);
}

static ssize_t show_fd655_bt4(struct class *cls,struct class_attribute *attr,
               char *buf){
    return sprintf(buf, "fd655_bit4_status: %d\n", pdata->bit4_status);
}
static ssize_t show_fd655_bt5(struct class *cls,struct class_attribute *attr,
               char *buf){
    return sprintf(buf, "fd655_bit5_status: %d\n", pdata->bit5_status);
}

static ssize_t show_fd655_bt6(struct class *cls,struct class_attribute *attr,
               char *buf){
    return sprintf(buf, "fd655_bit6_status: %d\n", pdata->bit6_status);
}

/********************************
 * Display Driver Enable
 * *****************************/

static ssize_t store_fd655_onoff(struct class *cls,struct class_attribute *attr,
              const  char *buf, size_t count)
{
               int fd655_onoff_val;
               

               if (kstrtoint(buf, 0, &fd655_onoff_val)){
                       return -EINVAL;
			   }

            // 0: off, 1: on
               if( fd655_onoff_val >=0 && fd655_onoff_val<2){
                  pdata->FD655SYS_ON_OF=fd655_onoff_val;
                  fd655_disp_onoff(pdata->FD655SYS_ON_OF);
               };
              
               return count;
}

static ssize_t show_fd655_onoff(struct class *cls,
struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "fd655_onoff_status: %d\n", pdata->FD655SYS_ON_OF);
}

static struct class_attribute fd65_class_attrs[] = {
        __ATTR(fd655disp, 0644, show_fd655_onoff, store_fd655_onoff),
		__ATTR(bit0, 0644, show_fd655_bt0, store_fd655_bt0),
		__ATTR(bit1, 0644, show_fd655_bt1, store_fd655_bt1),
		__ATTR(bit2, 0644, show_fd655_bt2, store_fd655_bt2),
		__ATTR(bit3, 0644, show_fd655_bt3, store_fd655_bt3),
		__ATTR(bit4, 0644, show_fd655_bt4, store_fd655_bt4),
		__ATTR(bit5, 0644, show_fd655_bt5, store_fd655_bt5),
		__ATTR(bit6, 0644, show_fd655_bt6, store_fd655_bt6),

};

static void create_fd655_attrs(void) {
        int i;

        fd65_class= class_create(THIS_MODULE, "fd655ctl");
		if (IS_ERR(fd65_class)) {
			pr_err("create fd655ctl debug class fail\n");
			return;
        }
        for (i = 0; i < ARRAY_SIZE(fd65_class_attrs); i++) {
         if (class_create_file(fd65_class,&fd65_class_attrs[i]))
            pr_err("create workled attribute %s fail\n", fd65_class_attrs[i].attr.name);
		  //  goto error_sysfs;
        }
}

/******************************
 * shutdown zeroing
 * ***************************/

void fd655_disp_powerDown(void){

	FD655_Disp(DIG1,0x00,pdata);
	FD655_Disp(DIG2,0x00,pdata);
	FD655_Disp(DIG3,0x00,pdata);
	FD655_Disp(DIG4,0x00,pdata);
	FD655_Disp(DIG5,0x00,pdata);
	FD655_Command(0x00,pdata);
	gpio_direction_output(pdata->dat_pin, 0);
        gpio_direction_output(pdata->clk_pin, 0);

}

static int fd655_dev_open(struct inode *inode, struct file *file)
{
    file->private_data = pdata;
    FD655_Command(FD655SYSON,pdata);
    return 0;
}

static ssize_t  fd655_dev_write(struct file *filp, const char __user *buf,size_t count, loff_t *f_pos)
{

		FD655_DEV *dev;
		unsigned long	ret;
		size_t		    status = 0, i = 0;
		char data[5] = {0};
		char tmp[5]= {0};
		dev = filp->private_data;


		if (count >5)		
			count = 5;

		ret = copy_from_user(data, buf, count);
		
		if (ret == 0)
		{	
			 mutex_lock(&showbit_mutex_lock);
				   data[4]=(data[4] & val);
			 mutex_unlock(&showbit_mutex_lock);

		   for(i=0; i<count; i++)
		   {
			
			   tmp[i] = Led_Get_Code(data[i]);
			   printk("Led_Get_Code buf: %x \r\n",tmp[i]);
		   }
				
			   
		  FD655_Disp(DIG1,tmp[0],dev);
		  FD655_Disp(DIG2,tmp[1],dev);
		  FD655_Disp(DIG3,tmp[2],dev);
		  FD655_Disp(DIG4,tmp[3],dev);
		  FD655_Disp(DIG5,data[4],dev);
		  status = count;
	    } 


	//	printk("fd655_dev_write count : %d\n", count );
		return status;

}


static int fd655_dev_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;

    //FD655_Command(SLEEP,pdata);

    LedShow(" ",pdata);
  
    FD655_Command(FD655SYSOFF,pdata);
    printk("succes to close  fd655_dev.............\n");

    return 0;
}

static struct file_operations fd655_fops = {
	.owner		=	THIS_MODULE,
	.open		=	fd655_dev_open,
	.release	=	fd655_dev_release,
	.write      =   fd655_dev_write,
	
};
static struct miscdevice fd655_device = {
	.minor	=	MISC_DYNAMIC_MINOR,
	.name	=	DEV_NAME,
	.fops	=	&fd655_fops,
};




static int register_fd655_driver(void)
{
    int ret = 0;
    ret = misc_register(&fd655_device);
	if(ret<0)
		printk("%s:%d failed to add fd655  module\n", __func__,__LINE__);
	else
		printk("%s:%d Successed to add fd655  module \n", __func__,__LINE__);

    return ret;
}

static void deregister_fd655_driver(void) 
{
	
	misc_deregister(&fd655_device);
}

static int fd655_driver_remove(struct platform_device *pdev)
{
  	fd655_disp_onoff(0);
    deregister_fd655_driver();
	class_unregister(&pdata->sysfs);
#ifdef CONFIG_OF
	gpio_free(pdata->clk_pin);
   	gpio_free(pdata->dat_pin);	
	kfree(pdata);
#endif
    return 0;
}

static int fd655_driver_suspend(struct platform_device *dev, pm_message_t state)
{
    
   	fd655_disp_onoff(0);
   	printk("fd655_driver_suspend \n");
	return 0;
}

static int fd655_driver_resume(struct platform_device *dev)
{

    fd655_disp_onoff(1);
    printk("fd655_driver_resume \n");
    return 0;
}


static void lumifyx_fd655_shutdown(struct platform_device *pdev)
{
  
	fd655_disp_powerDown();
}

static int fd655_driver_probe(struct platform_device *pdev)
{
	int state=-EINVAL;
	char buf[32];
	int ret;
	const char *str;
	unsigned int desc;


	printk("%s start!\n", __func__);

    if (!pdev->dev.of_node) {
		printk("Fd655_driver: pdev->dev.of_node == NULL!\n");
		state = -EINVAL;
		goto get_fd655_node_fail;
	}

	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
    if (!pdata) {
        printk("platform data is required!\n");
        state = -EINVAL;
        goto get_fd655_mem_fail;
    }

	snprintf(buf, sizeof(buf), "clk_pin");

	ret = of_property_read_string(pdev->dev.of_node, buf, &str);
	if(!ret){
		desc = of_get_named_gpio_flags(pdev->dev.of_node, buf, 0, NULL);		
		pdata->clk_pin = desc;
		printk("%s: %d\n", buf, desc);
	}else{
		printk("cannot find resource:%s \n", buf);
	}


	snprintf(buf, sizeof(buf), "dat_pin");
	ret = of_property_read_string(pdev->dev.of_node, buf, &str);
	if(!ret){
		desc = of_get_named_gpio_flags(pdev->dev.of_node, buf, 0, NULL);		
		pdata->dat_pin = desc;
		printk("%s: %d\n", buf, desc);
	}else{
		printk("cannot find resource \"%s\"\n", buf);
	}


    ret = gpio_request(pdata->clk_pin, "i2c1_clk_pin");
	if(ret){
		printk("---%s----can not request pin %d\n",__func__,pdata->clk_pin);
		goto get_fd655_mem_fail ;
	}	


	ret = gpio_request(pdata->dat_pin, "i2c1_dat_pin");
	if(ret){
		printk("---%s----can not request pin %d\n",__func__,pdata->dat_pin);
		goto get_fd655_mem_fail ;
	}

	platform_set_drvdata(pdev, pdata);
	
    register_fd655_driver();
	
	FD655_Command(FD655SYSON,pdata);
	FD655_Disp(DIG1,0x7f,pdata);
	FD655_Disp(DIG2,0x7f,pdata);
	FD655_Disp(DIG3,0x7f,pdata);
	FD655_Disp(DIG4,0x7f,pdata);
	FD655_Disp(DIG5,0x7f,pdata);

   create_fd655_attrs();


   return 0;
	
get_fd655_mem_fail:
	kfree(pdata);

get_fd655_node_fail:
    return state;

}



static const struct of_device_id fd655_dt_match[]={
	{	.compatible = "lumifyx,fd655_dev", .data = NULL},
	{ /* sentinel */ }
};


static struct platform_driver fd655_driver = {
    .probe      = fd655_driver_probe,
    .remove     = fd655_driver_remove,
    .suspend    = fd655_driver_suspend,
    .resume     = fd655_driver_resume,
    .driver     = {
        .name   = "lumifyx-fd655",
		.owner	= THIS_MODULE,
        .of_match_table = of_match_ptr(fd655_dt_match),
    },
	.shutdown = lumifyx_fd655_shutdown,
};

static int __init fd655_driver_init(void)
{
    printk( "Fd655 Driver init.\n");

	if (platform_driver_register(&fd655_driver)) {
		printk("platform_driver_register failed\n");
		return -1;
	}
	return 0;
}

static void __exit fd655_driver_exit(void)
{
    printk("Fd655 Driver exit.\n");
    platform_driver_unregister(&fd655_driver);
}

module_init(fd655_driver_init);
module_exit(fd655_driver_exit);

MODULE_AUTHOR("lumifyx");
MODULE_DESCRIPTION("Fd655 Driver");
MODULE_LICENSE("GPL");
