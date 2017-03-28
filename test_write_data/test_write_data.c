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
// this has to be commented out when compiling on 168.192.0.15 for some damn reason
//#define UINT16_T
/* comment it out in this machine and I get:

../main.h:12:22: error: conflicting types for ‘uint16_t’
 typedef unsigned int uint16_t;
                      ^
In file included from /usr/lib/gcc/x86_64-linux-gnu/5/include/stdint.h:9:0,
                 from /usr/include/curses.h:63,
                 from test_write_data.c:15:
/usr/include/stdint.h:49:28: note: previous declaration of ‘uint16_t’ was here
 typedef unsigned short int uint16_t;

wtf?

*/
#include "../main.h"

#define BAUDRATE B19200
#define MODEMDEVICE "/dev/ttyS0"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define LEN 200
void set_defaults(void);
// really cranking
#define TIME_DELAY 2000
// readable
//#define TIME_DELAY 300000
UCHAR current_param;
uint16_t temp_int;
UCHAR parse_state;

int set_interface_attribs (int fd, int speed, int parity);
void set_blocking (int fd, int should_block);
void do_read(WINDOW *win, int fd);

//******************************************************************************************//
//****************************************** main ******************************************//
//******************************************************************************************//
int main(int argc, char *argv[])
{
	int fd, res, res2;
	int type = 0;
	int iters;
	int i,j;
	UCHAR ch;
	struct termios oldtio,newtio;
	WINDOW *menu_win;
	useconds_t tdelay = TIME_DELAY;
    UCHAR data = 2;
    UCHAR data1 = 0;
	uint16_t data2 = 0;
    UCHAR code = RT_TRIP;
    UCHAR read_buff[10];
	//PROMPT_STRUCT prompts[30];
//	UCHAR no_prompts;
	int done;
	uint16_t rpm;
	UCHAR key;
	UCHAR wkey;
	char param_string[10];

	memset(read_buff,0,10);
	initscr();			/* Start curses mode 		*/
	clear();
	noecho();
	nodelay(stdscr,TRUE);
	raw();				/* Line buffering disabled	*/
	cbreak();	/* Line buffering disabled. pass on everything */
	menu_win = newwin(25, 50, 10,10);
	keypad(menu_win, TRUE);
	nodelay(menu_win, TRUE);
	box(menu_win,0,0);

	mvwprintw(menu_win, 2, 2, "RT_TRIP ");
	mvwprintw(menu_win, 3, 2,"RT_TIME ");
	mvwprintw(menu_win, 4, 2,"RT_AIRT ");
	mvwprintw(menu_win, 5, 2,"RT_O2 ");
	mvwprintw(menu_win, 6, 2,"RT_MAP ");
	mvwprintw(menu_win, 7, 2,"RT_OILT ");
	mvwprintw(menu_win, 8, 2,"RT_OILP ");
	mvwprintw(menu_win, 9, 2,"RT_ENGT ");
	mvwprintw(menu_win, 10, 2,"RT_MPH ");
	mvwprintw(menu_win, 11, 2,"RT_RPM ");
	wrefresh(menu_win);
	if(argc > 1)
	{
//		printf("write: ");
//		mvwprintw(menu_win, 2, 2,
		iters = atoi(argv[1])*10;
//		printf("iters: %d\n",iters);
		mvwprintw(menu_win,17,4,"interations: %d",iters);
		type = 1;
		if(argc > 2)
		{
			data2 = atoi(argv[2]);
//			printf("rpm starting at: %d\n",data2);
			mvwprintw(menu_win,18,4,"rpm starting at: %d",data2);
		}
		if(argc > 3)
		{
			data = atoi(argv[3]);
//			printf("others starting at: %d\n",data);
			mvwprintw(menu_win,19,4,"others starting at: %d",data);
		}
		if(argc > 4)
		{
			tdelay = atoi(argv[4])*1000;
			mvwprintw(menu_win,20,4,"time delay: %dK",atoi(argv[4]));
		}
	}
	else
//		printf("read\n");

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

// read
	if(type == 0) 
	{
		do_read(menu_win, fd);
	}	// end of else

// write
	else if(argc > 1)
	{
		for(i = 0;i < iters;i++)
		{
			code++;
			ch = code;
		    write(fd,&code,1);
//			printf("%x\n",ch);
			if(code == RT_RPM)
			{
				if(data2 & 0x8000)
				{
					ch = RT_HIGH3;
					res = write(fd,&ch,1);
//					printf("%x ",ch);
					data1 = (UCHAR)(data2);
					ch = data1;
					res = write(fd,&ch,1);
//					printf("%x ",ch);
					data1 = (UCHAR)(data2>>8);
					data1 &= 0x7f;
					ch = data1;
					res = write(fd,&ch,1);
//					printf("%x\n",ch);
					usleep(tdelay);
				}
				else if(data2 & 0x0080)
				{
//					printf("RT_HIGH2 ");
					ch = RT_HIGH2;
					res = write(fd,&ch,1);
//					printf("%x ",ch);
					data1 = (UCHAR)data2;
					data1 &= 0x7f;
					ch = data1;
					res = write(fd,&ch,1);
//					printf("%x ",ch);
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
//					printf("%x ",ch);
					data1 = (UCHAR)(data2);
					ch = data1;
					res = write(fd,&ch,1);
//					printf("%x ",ch);
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
				mvwprintw(menu_win, code-0xF4, 10, param_string);
				wrefresh(menu_win);
				code = RT_TRIP-1;
	//				code = RT_OILP;
			}
			else
			{
				if(data > 0x7f)
				{
//					printf("RT_HIGH0 ");
					ch = RT_HIGH0;
					res = write(fd,&ch,1);
//					printf("%x ",ch);
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
//					printf("%x ",ch);
					ch = data;
					res = write(fd,&ch,1);
//					printf("%x\n",ch);
					usleep(tdelay);
				}
				data++;
//				printf("%d\n",data);
				sprintf(param_string,"%4u",data);
				mvwprintw(menu_win, code-0xF4, 10, param_string);
				wrefresh(menu_win);
			}
			res = read(fd,read_buff,5);
			mvwprintw(menu_win, 13, 7, "bytes read: %d",res);
			mvwprintw(menu_win, 14, 8, "               ");
			for(j = 0;j < res;j++)
				mvwprintw(menu_win, 14, 10+(j*3), "%x",read_buff[j]);
			if(res == 2)
			{
				rpm = read_buff[0];
				rpm <<= 8;
				rpm |= read_buff[1];
				mvwprintw(menu_win, 15, 8, "rpm:  %d  ",rpm);
			}
			else if(res == 1)
			{
				mvwprintw(menu_win, 16, 8, "others:  %d  ",read_buff[0]);
			}
			wrefresh(menu_win);
			if(code == RT_TRIP-1)
				usleep(tdelay*2);
			key = wgetch(menu_win);	
			if(key != 0xff)
			{
				switch(key)
				{
					case '0':
						mvwprintw(menu_win, 22, 8, "zero  ");
						wkey = KP_0;
						break;
					case '1':
						mvwprintw(menu_win, 22, 8, "one   ");
						wkey = KP_1;
						break;
					case '2':
						mvwprintw(menu_win, 22, 8, "two   ");
						wkey = KP_2;
						break;
					case '3':
						mvwprintw(menu_win, 22, 8, "three ");
						wkey = KP_3;
						break;
					case '4':
						mvwprintw(menu_win, 22, 8, "four  ");
						wkey = KP_4;
						break;
					case '5':
						mvwprintw(menu_win, 22, 8, "five  ");
						wkey = KP_5;
						break;	
					case '6':
						mvwprintw(menu_win, 22, 8, "six   ");
						wkey = KP_6;
						break;	
					case '7':
						mvwprintw(menu_win, 22, 8, "seven ");
						wkey = KP_7;
						break;	
					case '8':
						mvwprintw(menu_win, 22, 8, "eight ");
						wkey = KP_8;
						break;	
					case '9':
						mvwprintw(menu_win, 22, 8, "nine  ");
						wkey = KP_9;
						break;	
					case '*':
						mvwprintw(menu_win, 22, 8, "ast   ");
						wkey = KP_AST;
						break;	
					case '#':
						mvwprintw(menu_win, 22, 8, "pound ");
						wkey = KP_POUND;
						break;	
					case 'A':
					case 'a':
						mvwprintw(menu_win, 22, 8, "A     ");
						wkey = KP_A;
						break;	
					case 'B':
					case 'b':
						mvwprintw(menu_win, 22, 8, "B     ");
						wkey = KP_B;
						break;	
					case 'C':
					case 'c':
						mvwprintw(menu_win, 22, 8, "C     ");
						wkey = KP_C;
						break;	
					case 'D':
					case 'd':
						mvwprintw(menu_win, 22, 8, "D     ");
						wkey = KP_D;
						break;	
					default:
						mvwprintw(menu_win, 22, 8, "?     ");
						wkey = 0xff;
						break;
				}
				if(wkey != 0xff)
					write(fd,&wkey,1);
			}			
		}	// end of for(...
	}	// end of else

//	while (STOP==FALSE) 
//	{       /* loop for input */
//		res = read(fd,buf,20);
//		printf("%d:%s ",res,buf);
//	}
	delwin(menu_win);
	clrtoeol();
	refresh();
	endwin();

	tcsetattr(fd,TCSANOW,&oldtio);
	close(fd);
//	printf("\nbytes read: %d\n",res2);
	printf("\n");
	exit(0);
}

void set_defaults(void)
{
	temp_int = 0;
	parse_state = IDLE;
}

//******************************************************************************************//
//**************************************** do_read *****************************************//
//******************************************************************************************//
void do_read(WINDOW *win, int fd)
{
	int done;
	int res;
	UCHAR ch;
	UCHAR xbyte;
	char param_string[10];
	uint16_t xword;
	UCHAR txword;
	UCHAR temp;
	UCHAR wkey;
	
	while(TRUE)
	{	
		done = 0;
		res = read(fd,&ch,1);
//			printf("parse_state:%x %x\n",parse_state,ch);
		switch(ch)
		{
			case KP_0:
				mvwprintw(win, 22, 8, "zero  ");
				wkey = KP_0;
				break;
			case KP_1:
				mvwprintw(win, 22, 8, "one   ");
				wkey = KP_1;
				break;
			case KP_2:
				mvwprintw(win, 22, 8, "two   ");
				wkey = KP_2;
				break;
			case KP_3:
				mvwprintw(win, 22, 8, "three ");
				wkey = KP_3;
				break;
			case KP_4:
				mvwprintw(win, 22, 8, "four  ");
				wkey = KP_4;
				break;
			case KP_5:
				mvwprintw(win, 22, 8, "five  ");
				wkey = KP_5;
				break;	
			case KP_6:
				mvwprintw(win, 22, 8, "six   ");
				wkey = KP_6;
				break;	
			case KP_7:
				mvwprintw(win, 22, 8, "seven ");
				wkey = KP_7;
				break;	
			case KP_8:
				mvwprintw(win, 22, 8, "eight ");
				wkey = KP_8;
				break;	
			case KP_9:
				mvwprintw(win, 22, 8, "nine  ");
				wkey = KP_9;
				break;	
			case KP_AST:
				mvwprintw(win, 22, 8, "ast   ");
				wkey = KP_AST;
				break;	
			case KP_POUND:
				mvwprintw(win, 22, 8, "pound ");
				wkey = KP_POUND;
				break;	
			case KP_A:
				mvwprintw(win, 22, 8, "A     ");
				wkey = KP_A;
				break;	
			case KP_B:
				mvwprintw(win, 22, 8, "B     ");
				wkey = KP_B;
				break;	
			case KP_C:
				mvwprintw(win, 22, 8, "C     ");
				wkey = KP_C;
				break;	
			case KP_D:
				mvwprintw(win, 22, 8, "D     ");
				wkey = KP_D;
				break;	
			default:

				switch(parse_state)
				{
					case IDLE:
						if(ch <= RT_RPM && ch >= RT_TRIP)
						{
							current_param = ch;
		//						printf("current:");
							parse_state = CHECK_HIGHBIT;
						}
						else
						{
		//						printf("opps\n");
							set_defaults();
						}
						break;
					case CHECK_HIGHBIT:
						switch(ch)
						{
							case RT_LOW:
								parse_state = SEND_UCHAR0;
								break;
							case RT_HIGH0:		// if just a UCHAR is sent with high bit set
								parse_state = SEND_UCHAR1;
								break;
							case RT_HIGH1:		// if UINT with neither bit 7 or 15 set
								parse_state = GET_CH0;
								break;	
							case RT_HIGH2:		// if a UINT is sent with bit 7 set
								parse_state = GET_CH1;
								break;
							case RT_HIGH3:		// if a UINT is sent with bit 15 set
								parse_state = GET_CH2;
								break;
							default:
								set_defaults();
								break;
						}
						break;
					case GET_CH0:
						parse_state = SEND_UINT0;
						temp_int = ch;
						break;
					case GET_CH1:
						temp_int = ch;
						parse_state = SEND_UINT1;
						break;
					case GET_CH2:
						temp_int = ch;
						parse_state = SEND_UINT2;
						break;
					case SEND_UCHAR0:
						xbyte = ch;
						sprintf(param_string,"%4d",xbyte);
		//					printf("uchar0:%s\n",param_string);
						done = 1;
						break;
					case SEND_UCHAR1:
						xbyte = ch | 0x80;
						sprintf(param_string,"%4d",xbyte);
		//					printf("uchar1:%s\n",param_string);
						done = 1;
						break;
					case SEND_UINT0:
						xword = (uint16_t)temp_int;
						temp_int = (uint16_t)ch;
						temp_int <<= 8;
						temp_int &= 0xff00;
						xword |= temp_int;
						sprintf(param_string,"%4u",xword);
		//					printf("uint0:%s\n",param_string);
	
						done = 1;
						break;
					case SEND_UINT1:
						xword = (uint16_t)temp_int;
						temp_int = (uint16_t)ch;
						temp_int <<= 8;
						temp_int &= 0xff00;
						xword |= temp_int;
						xword |= 0x0080;
						sprintf(param_string,"%4u",xword);
		//					printf("uint1:%s\n",param_string);
						done = 1;
						break;
					case SEND_UINT2:
						xword = (uint16_t)temp_int;
						temp_int = (uint16_t)ch;
						temp_int <<= 8;
						temp_int &= 0xff00;
						xword |= temp_int;
						xword |= 0x8000;
						sprintf(param_string,"%4u",xword);
		//					printf("uint2:%s\n",param_string);
						done = 1;
						break;
					default:
						printf("%s\n","default ");
						set_defaults();
						break;
				}	// end of inner switch
				if(done)
				{
		//				printf("%5s\n",param_string);
					mvwprintw(win, current_param-0xF4, 10, param_string);
					wrefresh(win);

					temp = ~current_param;

					if(temp == 0)
					{
						txword = (uint8_t)(xword>>8);
						write(fd,&txword,1);
						txword = (uint8_t)(xword);
						write(fd,&txword,1);
					}
					else
						write(fd,&xbyte,1);

		/*
					for(i = 0;i < no_prompts;i++)
					{
						if(prompts[i].type == RT_LABEL && temp == prompts[i].pnum)
						{
		//						GDispStringAt(prompts[i].row,prompts[i].col+10,param_string);
						}
					}
		*/
					set_defaults();
				}	// end of done
		}	// end of outer switch
	}	// end of while(1)	
}

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

