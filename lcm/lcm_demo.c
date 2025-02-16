/****************************************************************************
 *                                                                          *
 * Copyright (c) 2008 Nuvoton Technology Corp. All rights reserved.         *
 *                                                                          *
 ***************************************************************************/
 
/****************************************************************************
 * 
 * FILENAME
 *     lcm.c
 *
 * VERSION
 *     1.0
 *
 * DESCRIPTION
 *     This file is a LCM sample program
 *
 * DATA STRUCTURES
 *     None
 *
 * FUNCTIONS
 *     None
 *
 * HISTORY
 *     2011/05/01		 Ver 1.0 Created by PX40 MHKuo
 *
 * REMARK
 *     None
 **************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <asm/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>

/*IOCTLs*/
//#define IOCTLSETCURSOR			_IOW('v', 21, unsigned int)	//set cursor position
#define VIDEO_ACTIVE_WINDOW_COORDINATES		_IOW('v', 22, unsigned int)	//set display-start line in display buffer
//#define IOCTLREADSTATUS			_IOW('v', 23, unsigned int)	//read lcd module status
#define VIDEO_DISPLAY_ON			_IOW('v', 24, unsigned int)	//display on
#define VIDEO_DISPLAY_OFF			_IOW('v', 25, unsigned int)	//display off
//#define IOCTLCLEARSCREEN		_IOW('v', 26, unsigned int)	//clear screen
//#define VIDEO_UPSCALING			_IOW('v', 27, unsigned int) //video up scaling
#define IOCTL_LCD_BRIGHTNESS		_IOW('v', 27, unsigned int)  //brightness control	



#define VIDEO_DISPLAY_LCD			_IOW('v', 38, unsigned int)	//display on
#define VIDEO_DISPLAY_TV			_IOW('v', 39, unsigned int)	//display off
#define VIDEO_FORMAT_CHANGE			_IOW('v', 50, unsigned int)	//frame buffer format change
#define VIDEO_TV_SYSTEM				_IOW('v', 51, unsigned int)	//set TV NTSC/PAL system 

//#define LCD_RGB565_2_RGB555		_IOW('v', 30, unsigned int)	//RGB565_2_RGB555
//#define LCD_RGB555_2_RGB565		_IOW('v', 31, unsigned int)	//RGB555_2_RGB565

#define LCD_RGB565_2_RGB555		_IO('v', 30)
#define LCD_RGB555_2_RGB565		_IO('v', 31)

#define LCD_ENABLE_INT		_IO('v', 28)
#define LCD_DISABLE_INT		_IO('v', 29)


#define DISPLAY_MODE_RGB555	0
#define DISPLAY_MODE_RGB565	1
#define DISPLAY_MODE_CBYCRY	4
#define DISPLAY_MODE_YCBYCR	5
#define DISPLAY_MODE_CRYCBY	6
#define DISPLAY_MODE_YCRYCB	7
/* Macros about LCM */
#define CMD_DISPLAY_ON						0x3F
#define CMD_DISPLAY_OFF						0x3E
#define CMD_SET_COL_ADDR					0x40
#define	CMD_SET_ROW_ADDR					0xB8
#define CMD_SET_DISP_START_LINE		0xC0


#define CHAR_WIDTH 		5

static struct fb_var_screeninfo var;

typedef struct Cursor
{
	unsigned char x;
	unsigned char y;
}Cursor;

typedef struct
{
	unsigned int start;
	unsigned int end;
}ActiveWindow;

typedef struct
{
	int hor;
	int ver;
}video_scaling;

typedef  struct _font {
	unsigned char c[CHAR_WIDTH];
}font;

/* Dot-matrix of 0,1,2,3,4,5,6,7,8,9 */
font myFont[11] = {{0x3e, 0x41, 0x41, 0x3e, 0x00}, //zero
			 {0x00, 0x41, 0x7f, 0x40, 0x00}, //un
			 {0x71, 0x49, 0x49, 0x46, 0x00}, //deux
			 {0x49, 0x49, 0x49, 0x36, 0x00}, //trois
			 {0x0f, 0x08, 0x08, 0x7f, 0x00}, //quatre
			 {0x4f, 0x49, 0x49, 0x31, 0x00}, //cinq
			 {0x3e, 0x49, 0x49, 0x32, 0x00}, //six
			 {0x01, 0x01, 0x01, 0x7f, 0x00}, //sept
			 {0x36, 0x49, 0x49, 0x36, 0x00}, //huit
			 {0x06, 0x49, 0x49, 0x3e, 0x00}, //neuf
			 {0x00, 0x00, 0x60, 0x00, 0x00}}; // point

int main()
{
	int fd, ret;
	int i, t = 0;
	Cursor cur;		
	FILE *fpVideoImg;
	unsigned char *pVideoBuffer;
	unsigned long uVideoSize;
	char x[5], y[5];
	ActiveWindow window;
	unsigned int uStartLine;
	video_scaling v_scaling;
	unsigned int select = 0;
	char inputstring[10];
	int brightness=2000;
	
	cur.x = cur.y = 10;
	memset(x, 0, 5);
	memset(y, 0, 5);
	
	fd = open("/dev/fb0", O_RDWR);
	if (fd == -1)
	{
		printf("Cannot open fb0!\n");
		return -1;
	}
	
	if (ioctl(fd, FBIOGET_VSCREENINFO, &var) < 0) {
		perror("ioctl FBIOGET_VSCREENINFO");
		close(fd);
		return -1;
	}
	
//	ioctl(fd,LCD_ENABLE_INT);
	uVideoSize = var.xres * var.yres * var.bits_per_pixel / 8;
	
printf("uVideoSize = 0x%x\n", uVideoSize);
printf("var.xres = 0x%x\n", var.xres);
printf("var.yres = 0x%x\n", var.yres);	
	//	printf("uVideoSize = 0x%x \n", uVideoSize);
	pVideoBuffer = mmap(NULL, uVideoSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	
	printf("pVideoBuffer = 0x%x\n", pVideoBuffer);
	if(pVideoBuffer == MAP_FAILED)
	{
		printf("LCD Video Map Failed!\n");
		exit(0);
	}
	//memset(pVideoBuffer, 0xf8, uVideoSize);	
	
	if (uVideoSize == 320*240*2)
		fpVideoImg = fopen("320x240.dat", "r");	
	else if (uVideoSize == 640*480*2)
		fpVideoImg = fopen("640x480.dat", "r");	
	else if (uVideoSize == 480*272*2)
		fpVideoImg = fopen("480x272.dat", "r");	
	else if (uVideoSize == 720*480*2)
		fpVideoImg = fopen("720x480.dat", "r");	
	else if (uVideoSize == 800*480*2)
		fpVideoImg = fopen("800x480.dat", "r");	
	

	 if(fpVideoImg == NULL)
    {
    	printf("open Image FILE fail !! \n");
    	exit(0);
    }  
    
	if( fread(pVideoBuffer, uVideoSize, 1, fpVideoImg) <= 0)
	{
		printf("Cannot Read the Image File!\n");
		exit(0);
	}

	ioctl(fd,LCD_ENABLE_INT);
	
	while(select != 0xFF)
	{
		printf("\n************* LCM demo ************\n");
		printf("1.Video Display On ..\n");
   		printf("2.Video Display Off ..\n");
//		printf("3.Change start line in buffer ..\n");
//		printf("4.Scaling Up ..\n");
//		printf("3.Brightness setting ..\n");
//		printf("4.RGB565 to RGB555 ..\n");
//		printf("5.RGB555 to RGB565 ..\n");
//		printf("6.Enable int ..\n");
//		printf("7.Disabe int ..\n");
		printf("8.Video Display by LCD ..\n");
   		printf("9.Video Display by TV ..\n");
   		printf("a.Video format change (to RGB565) ..\n");   		
   		printf("b.Video format change (to CBYCRY) ..\n");   		   		
   		printf("c.TV in NTSC system ..\n");   		
   		printf("d.TV in PAL system ..\n");   		   		

		printf("X.Exit ..\n");
		printf("\n***********************************\n");
    		printf("Select : \n");  
    		select = getchar();
		getchar();
		//select = 0x33;
    	switch(select)
    	{
    		case 0x31:
    			ioctl(fd,VIDEO_DISPLAY_ON);
    			break;
    		
    		case 0x32:
    			ioctl(fd,VIDEO_DISPLAY_OFF);
    			break;
	#if 0    			
			case 0x33:
				//printf("Input Brightness Value : ");
				//scanf("%s", inputstring);
				//brightness = atoi(inputstring);
		
				//printf("%d\n", brightness);
				if(brightness==0) brightness = 1;
				if(brightness<0) brightness = 2000;
				ioctl(fd,IOCTL_LCD_BRIGHTNESS,&brightness);
		
				sleep(5);
				brightness-=200;
				break;
			
			case 0x34:
	    			ioctl(fd,LCD_RGB565_2_RGB555);
				printf("RGB565_2_RGB555..\n");
				break;
	
			case 0x35:
				ioctl(fd,LCD_RGB555_2_RGB565);
		    		printf("RGB555_2_RGB565..\n");
				break;
			case 0x36:
	    			ioctl(fd,LCD_ENABLE_INT);
				printf("Enable int..\n");
				break;
	
			case 0x37:
				ioctl(fd,LCD_DISABLE_INT);
		    		printf("Disable int..\n");
				break;	
	#endif				
    		case 0x38:
    			ioctl(fd,VIDEO_DISPLAY_LCD);
    			break;
    		
    		case 0x39:
    			ioctl(fd,VIDEO_DISPLAY_TV);
    			break;

    		case 'a':
    			ioctl(fd,VIDEO_FORMAT_CHANGE, DISPLAY_MODE_RGB565);
    			break;

    		case 'b':
    			ioctl(fd,VIDEO_FORMAT_CHANGE, DISPLAY_MODE_CBYCRY);
    			break;

    		case 'c':
    			ioctl(fd,VIDEO_TV_SYSTEM, 0);	// TV in NTSC system
    			break;

    		case 'd':
    			ioctl(fd,VIDEO_TV_SYSTEM, 1);	// TV in PAL system
    			break;

	   		case 0x58: //'x' -- exit the program    		
    		case 0x78: //'X'    			
 				  printf("Exit now.\n");
    			goto end; 
    			
    		default :
    			printf("Please select the right command number... \n");
    			break;	
    	}	
	}

end:
	/* Close LCD */
	//ioctl(fd, VIDEO_DISPLAY_OFF);	
	close(fd);	
	return 0;
}
