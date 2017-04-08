	// test_write_data.c - used to test the data protocol between the AVR and PIC24 where the
// data string is sent with FF,FE,FD.. and the next 3 bytes are shifted so they don't
// have the high bits set - displays in ncurses window

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>
#include <ncurses.h>
#include "../sfr_helper.h"
#include "../main.h"
#include "../t6963.h"

#define BAUDRATE B19200
#define MODEMDEVICE "/dev/ttyS0"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define LEN 200
#define DISP_OFFSET 15
void set_defaults(void);
// really cranking
#define TIME_DELAY 2000
// readable
//#define TIME_DELAY 300000

int set_interface_attribs (int fd, int speed, int parity);
void set_blocking (int fd, int should_block);
//void display_labels(void);
//******************************************************************************************//
//****************************************** main ******************************************//
//******************************************************************************************//
int main(int argc, char *argv[])
{
	int fd, res, res2;
	int type = 0;
	int iters;
	int i,j;
	UCHAR alnum_strlen = 0;
	int write_alnum = 10;
	UCHAR ch;
	struct termios oldtio,newtio;
	WINDOW *menu_win;
	useconds_t tdelay = TIME_DELAY;
    UCHAR data = 2;
    UCHAR data1 = 0;
	UINT data2 = 0;
    UCHAR code = RT_RPM-1;
    UCHAR read_buff[10];
	int done;
	UINT rpm;
	UCHAR key;
	UCHAR wkey;
	int ret_code;
	char param_string[10];
	int display_offset = 0;
	char aux_array[NUM_ENTRY_SIZE];

	if((argc > 2) && (argv[1][0] == 'w'))
		type = 1;
	else if((argc == 2) && (argv[1][0] == 'r'))
		type = 0;
	else
	{
		printf("usage: test_data w [no iters][starting rpm][others][delay]\n");
		printf("or test_data r\n");
		return 1;
	}
	memset(aux_array,0,NUM_ENTRY_SIZE);
	memset(labels,0,NUM_LABELS*MAX_LABEL_LEN);
	memset(menu_structs,0,NUM_MENU_STRUCTS*sizeof(MENU_STRUCT));
	memset(rt_params,0,NUM_RT_PARAMS*sizeof(RT_PARAM));
#if 0
	burn_eeprom();
	char temp_label[MAX_LABEL_LEN];
	for(i = 0;i < no_labels;i++)
	{
		printf("%s\n",labels[i]);
	}

	printf("rt_params:\n");
	for(i = 0;i < no_rtparams;i++)
	{
		printf("%d\t",rt_params[i].row);
		printf("%d\t",rt_params[i].col);
		printf("%d\t",rt_params[i].shown);
		printf("%d\n",rt_params[i].type);
	}
	printf("menu structs: %d\n",no_menu_structs);
	for(i = 0;i < no_menu_structs;i++)
	{
		printf("%d\t",menu_structs[i].pnum);
		printf("%d\t",menu_structs[i].row);
		printf("%d\t",menu_structs[i].col);
		printf("%x\t",menu_structs[i].menu_choice);
		printf("%d\t",menu_structs[i].ch_type);
		printf("%d\n",menu_structs[i].type);
	}

	return 0;
#endif
	burn_eeprom();
	memset(read_buff,0,10);
	initscr();			/* Start curses mode 		*/
	clear();
	noecho();
	nodelay(stdscr,TRUE);
	raw();				/* Line buffering disabled	*/
	cbreak();	/* Line buffering disabled. pass on everything */
	menu_win = newwin(47, 40, 0,0);
	keypad(menu_win, TRUE);
	nodelay(menu_win, TRUE);
	box(menu_win,0,0);
	set_win(menu_win);

	if(type == 0)
		display_offset = DISP_OFFSET;

	if(type < 2)
	{
		mvwprintw(menu_win, display_offset+2, 2,"RPM ");
		mvwprintw(menu_win, display_offset+3, 2,"TRIP ");
		mvwprintw(menu_win, display_offset+4, 2,"TIME ");
		mvwprintw(menu_win, display_offset+5, 2,"AIRT ");
		mvwprintw(menu_win, display_offset+6, 2,"O2 ");
		mvwprintw(menu_win, display_offset+7, 2,"MAP ");
		mvwprintw(menu_win, display_offset+8, 2,"OILT ");
		mvwprintw(menu_win, display_offset+9, 2,"OILP ");
		mvwprintw(menu_win, display_offset+10,2,"ENGT ");
		mvwprintw(menu_win, display_offset+11,2,"MPH ");
		wrefresh(menu_win);
	}
	if(type == 1)
	{
//		printf("write: ");
		iters = atoi(argv[2])*10;
//		printf("iters: %d\n",iters);
		mvwprintw(menu_win,display_offset+17,4,"interations: %d",iters);
		if(argc > 2)
		{
			data2 = atoi(argv[3]);
//			printf("rpm starting at: %d\n",data2);
			mvwprintw(menu_win,display_offset+18,4,"rpm starting at: %d",data2);
		}
		if(argc > 3)
		{
			data = atoi(argv[4]);
//			printf("others starting at: %d\n",data);
			mvwprintw(menu_win,display_offset+19,4,"others starting at: %d",data);
		}
		if(argc > 4)
		{
			tdelay = atoi(argv[5])*1000;
			mvwprintw(menu_win,display_offset+20,4,"time delay: %dK",atoi(argv[4]));
		}
	}

	memset(&newtio, 0, sizeof newtio);

	fd = open (MODEMDEVICE, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd <0) {perror(MODEMDEVICE);
		exit(-1); }

	if(tcgetattr(fd,&oldtio) != 0) /* save current port settings */
	{
		printf("Error from tcgetattr: %d ",errno);
		close(fd);
		exit(1);
	}

	set_interface_attribs (fd, BAUDRATE, 0);
	set_blocking (fd, 0);

	res2 = 0;
	set_defaults();

// read	- simulate the AVR
	if(type == 0)
	{
//		printf("read\n");
		ret_code = do_read(menu_win, fd,display_offset);
		printf("do_read: %d\n",ret_code);
	}	// end of else

// write - simulate the PIC24
	else if(type == 1)
	{
		for(i = 0;i < iters;i++)
		{
			if(++code > RT_MPH)
			{
/*
				if(--write_alnum < 1)
				{
					write_alnum = 10;
					code = RT_AUX;
				}
				else
*/
				code = RT_RPM;
			}
			ch = code;
		    write(fd,&code,1);
//			printf("%x\n",ch);
/*
			if(code == RT_AUX)
			{

				memset(aux_array,0,NUM_ENTRY_SIZE);
				usleep(tdelay*2);
				read(fd,&alnum_strlen,1);
				usleep(tdelay/2);
				res = read(fd,aux_array,NUM_ENTRY_SIZE);

			}
*/
			if(code == RT_RPM)
			{
				if(data2 & 0x8000)
				{
					ch = RT_HIGH3;
					res = write(fd,&ch,1);
//					printf("%d %x ",res,ch);
					data1 = (UCHAR)(data2);
					ch = data1;
//					printf("%d %x ",res,ch);
					data1 = (UCHAR)(data2>>8);
					data1 &= 0x7f;
					ch = data1;
					res = write(fd,&ch,1);
//					printf("%d %x ",res,ch);
					usleep(tdelay);
				}
				else if(data2 & 0x0080)
				{
//					printf("RT_HIGH2 ");
					ch = RT_HIGH2;
					res = write(fd,&ch,1);
//					printf("%d %x ",res,ch);
					data1 = (UCHAR)data2;
					data1 &= 0x7f;
					ch = data1;
					res = write(fd,&ch,1);
//					printf("%d %x ",res,ch);
					data1 = (UCHAR)(data2>>8);
					ch = data1;
					res = write(fd,&ch,1);
//					printf("%x\n",ch);
					usleep(tdelay);
				}
				else
				{
//					printf("RT_HIGH1 ");
					ch = RT_HIGH1;
					res = write(fd,&ch,1);
//					printf("%d %x ",res,ch);
					data1 = (UCHAR)(data2);
					ch = data1;
					res = write(fd,&ch,1);
//					printf("%d %x ",res,ch);
					data1 = (UCHAR)(data2>>8);
					ch = data1;
					res = write(fd,&ch,1);
//					printf("%x\n",ch);
					usleep(tdelay);
				}
				if(data2 > 6000)
					data2 = 0;
				data2++;
//				printf("%d\n",data2);
				sprintf(param_string,"%4u",data2);
				mvwprintw(menu_win, display_offset+code, 10, param_string);
				wrefresh(menu_win);
			}
			else
			{
				if(data > 0x7f)
				{
//					printf("RT_HIGH0 ");
					ch = RT_HIGH0;
					res = write(fd,&ch,1);
//					printf("%d %x ",res,ch);
					data1 = data & 0x7f;
					ch = data1;
					res = write(fd,&ch,1);
//					printf("%x\n",ch);
					usleep(tdelay);
				}
				else
				{
//					printf("RT_LOW ");
					ch = RT_LOW;
					res = write(fd,&ch,1);
//					printf("%d %x ",res,ch);
					ch = data;
					res = write(fd,&ch,1);
//					printf("%d %x\n",res,ch);
					usleep(tdelay);
				}
				data++;
//				printf("data = %d\n",data);
				sprintf(param_string,"%4u",data);
				mvwprintw(menu_win, display_offset+code, 10, param_string);
				wrefresh(menu_win);
			}
			res = read(fd,read_buff,5);
#if 1
			mvwprintw(menu_win, display_offset+13, 4, "bytes read: %d",res);
			mvwprintw(menu_win, display_offset+14, 4, "                       ");
			for(j = 0;j < res;j++)
				mvwprintw(menu_win, display_offset+14, 10+(j*3), "%x",read_buff[j]);
			if(res == 2)
			{
				rpm = read_buff[0];
				rpm <<= 8;
				rpm |= read_buff[1];
				mvwprintw(menu_win, display_offset+15, 4, "rpm:  %d  ",rpm);
			}
			else if(res == 1)
			{
				mvwprintw(menu_win, display_offset+16, 4, "others:  %d  ",read_buff[0]);
			}
			mvwprintw(menu_win, display_offset+21, 4, "iterations left: %d   ",iters-i);
			mvwprintw(menu_win, display_offset+23, 4,"                         ");
			for(j = 0;j < NUM_ENTRY_SIZE;j++)
				mvwprintw(menu_win, display_offset+23, 4+j, "%c",aux_array[j]);
			mvwprintw(menu_win, display_offset+24, 4,"alnum strlen: %d",alnum_strlen);
			wrefresh(menu_win);

#endif
// see if one of the keys from the "keypad" is pressed
#if 1
			key = wgetch(menu_win);
			if(key != 0xff)
			{
				switch(key)
				{
					case '0':
						mvwprintw(menu_win, display_offset+22, 8, "zero  ");
						wkey = KP_0;
						break;
					case '1':
						mvwprintw(menu_win, display_offset+22, 8, "one   ");
						wkey = KP_1;
						break;
					case '2':
						mvwprintw(menu_win, display_offset+22, 8, "two   ");
						wkey = KP_2;
						break;
					case '3':
						mvwprintw(menu_win,display_offset+22, 8, "three ");
						wkey = KP_3;
						break;
					case '4':
						mvwprintw(menu_win, display_offset+22, 8, "four  ");
						wkey = KP_4;
						break;
					case '5':
						mvwprintw(menu_win, display_offset+22, 8, "five  ");
						wkey = KP_5;
						break;
					case '6':
						mvwprintw(menu_win, display_offset+22, 8, "six   ");
						wkey = KP_6;
						break;
					case '7':
						mvwprintw(menu_win, display_offset+22, 8, "seven ");
						wkey = KP_7;
						break;
					case '8':
						mvwprintw(menu_win, display_offset+22, 8, "eight ");
						wkey = KP_8;
						break;
					case '9':
						mvwprintw(menu_win, display_offset+22, 8, "nine  ");
						wkey = KP_9;
						break;
					case '*':
						mvwprintw(menu_win, display_offset+22, 8, "ast   ");
						wkey = KP_AST;
						break;
					case '#':
						mvwprintw(menu_win, display_offset+22, 8, "pound ");
						wkey = KP_POUND;
						break;
					case 'A':
					case 'a':
						mvwprintw(menu_win, display_offset+22, 8, "A     ");
						wkey = KP_A;
						break;
					case 'B':
					case 'b':
						mvwprintw(menu_win, display_offset+22, 8, "B     ");
						wkey = KP_B;
						break;
					case 'C':
					case 'c':
						mvwprintw(menu_win, display_offset+22, 8, "C     ");
						wkey = KP_C;
						break;
					case 'D':
					case 'd':
						mvwprintw(menu_win, display_offset+22, 8, "D     ");
						wkey = KP_D;
						break;
// use 'z' as a shortcut to '#' and 'y' as a shcortcut to '#'
					case 'Y':
					case 'y':
						mvwprintw(menu_win, display_offset+22, 8, "pound ");
						wkey = KP_POUND;
						break;
					case 'Z':
					case 'z':
						mvwprintw(menu_win, display_offset+22, 8, "ast   ");
						wkey = KP_AST;
						break;
					default:
						mvwprintw(menu_win, display_offset+22, 8, "?     ");
						wkey = 0xff;
						break;
				}
				if(wkey != 0xff)
					write(fd,&wkey,1);
#endif
			}
		}	// end of for(...
	}	// end of else

	delwin(menu_win);
	clrtoeol();
	refresh();
	endwin();

	tcsetattr(fd,TCSANOW,&oldtio);
	close(fd);
	exit(0);
}

void set_defaults(void)
{
	temp_UINT = 0;
	parse_state = IDLE;
}
//******************************************************************************************//
//**************************************** display_labels **********************************//
//******************************************************************************************//
// displays only the labels of the current rt_layout
#if 0
void display_labels(void)
{
	int i;
#if 0
	for(i = 0;i < no_prompts;i++)
	{
		if(prompts[i].type == RT_LABEL)
		{
//			eeprom_read_block(temp, eepromString+prompts[i].offset,prompts[i].len+1);
			GDispStringAt(prompts[i].row,prompts[i].col,prompts[i].label);
		}
	}
#endif
}
#endif
//******************************************************************************************//
//*********************************** set_interface_attribs ********************************//
//******************************************************************************************//
int set_interface_attribs (int fd, int speed, int parity)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                printf ("error %d from tcgetattr", errno);
				perror(" ");
                return -1;
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                printf ("error %d from tcsetattr", errno);
				perror(" ");
                return -1;
        }
        return 0;
}

void set_blocking (int fd, int should_block)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                printf ("error %d from tggetattr", errno);
                perror(" ");
                return;
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
                printf ("error %d setting term attributes", errno);
}

