#include "../../../include/string.h"
#include "../../../include/system.h"
#include "../../../include/shell.h"
#include "../../../include/tty.h"
#include "../../../include/util.h"
#include "../../../include/kb.h"
#include "../../../include/math.h"
#include "../../../include/multiboot.h"
#include "../../../include/vga.h"
#include <stdint.h>

uint32_t __stack_chk_fail_local()
{
    return 0;
}

void echo()
{
	printf_colored("\n! ", 11, 0);
	string str = readStr();
	printf("\n ");
	printf(str);
	printf("\n");
}

void set_background_color()
{
	printf("\n Color codes : ");
	printf("\n 0  : black");
	printf_colored("\n 1  : blue",1,0); // tty.h
	printf_colored("\n 2  : green",2,0);
	printf_colored("\n 3  : cyan",3,0);
	printf_colored("\n 4  : red",4,0);
	printf_colored("\n 5  : purple",5,0);
	printf_colored("\n 6  : orange",6,0);
	printf_colored("\n 7  : grey",7,0);
	printf_colored("\n 8  : dark grey",8,0);
	printf_colored("\n 9  : blue light",9,0);
	printf_colored("\n 10 : green light",10,0);
	printf_colored("\n 11 : blue lighter",11,0);
	printf_colored("\n 12 : red light",12,0);
	printf_colored("\n 13 : rose",13,0);
	printf_colored("\n 14 : yellow",14,0);
	printf_colored("\n 15 : white",15,0);

	printf_colored("\n\n Text color",7,0);
	printf_colored("! ", 11, 0);
	int text_color = str_to_int(readStr());

	printf_colored("\n\n Background color",7,0);
	printf_colored("! ", 11, 0);
	int bg_color = str_to_int(readStr());

	set_screen_color(text_color,bg_color);
	clearScreen();
}

void joke_spam() {
	for (int i = 1; i <= 100; i++) 
	{
		printf_colored(" EYN-OS\n", 12, 0);
	}
	printf("\n");
}

void ver(multiboot_info_t *mbi) {
	printf_colored("\n #########   ###     ###   ###      ###              ######      ######", 13, 0);
    printf_colored("\n ###           ### ###     ######   ###            ###    ###  ###", 13, 0);
    printf_colored("\n #########       ###       ###  ### ###   ######   ###    ###    ######", 13, 0);
    printf_colored("\n ###             ###       ###    #####            ###    ###        ###", 13, 0);
    printf_colored("\n #########       ###       ###      ###              ######     ######", 13, 0);
	printf_colored("\n (ver. 0.04)\n\n", 7, 0);
}

void help()
{
	printf("\n cmd       : Launch a new recursive Shell");
	printf("\n clear     : Clears the screen");
	printf("\n echo      : Reprints a given text");
	printf("\n exit      : Quits the current shell");
	printf("\n color     : Changes the colors of the terminal");
	printf("\n ver       : Shows the current system version");
	printf("\n spam      : Spam 'EYN-OS' to the shell");
	printf("\n calc      : Simple 8-Bit calculator\n\n");
}

void maths()
{
	printf("\n Math symbols:\n 1 : Add\n 2 : Minus\n 3 : Divide\n 4 : Multiply\n");

	printf("\n Math symbol");
	printf_colored("! ", 11, 0);
	string symbol = readStr();
	int intsymbol=str_to_int(symbol);

	printf("\n First number");
	printf_colored("! ", 11, 0);
	string strfint = readStr();
	int fint=str_to_int(strfint);

	printf("\n Second number");
	printf_colored("! ", 11, 0);
	string strsint = readStr();
	int sint=str_to_int(strsint);
	printf("\n ");

	if(intsymbol == 1)
	{
		printf(int_to_string(math_add(fint,sint)));
	} 
	else if(intsymbol == 2)
	{
		printf(int_to_string(math_remove(fint,sint)));
	} 
	else if(intsymbol == 4)
	{
		printf(int_to_string(math_epi(fint,sint)));
	} 
	else if(intsymbol == 3)
	{
		printf(int_to_string(math_dia(fint,sint)));
	}
	else
	{
		printf(" You used an invalid symbol.");
	}
	printf("\n\n");
}

void launch_shell(int n, multiboot_info_t *mbi)
{
	string ch = (string) malloc(200); // util.h
	string data[64];
	int gpu_mode = 0;
	do
	{
			printf_colored("! ", 14, 0);
		    ch = readStr();
		    if(cmdEql(ch,"cmd"))
		    {
		            printf("\nNew recursive shell opened.\n");
					launch_shell(n+1, mbi);
		    }
		    else if(cmdEql(ch,"clear"))
		    {
		            clearScreen();
		    }
		    else if(cmdEql(ch,"echo"))
		    {
		    	echo();
		    }
		    else if(cmdEql(ch,"help"))
		    {
		    	help();
		    }
		    else if(cmdEql(ch,"spam"))
		    {
		    	joke_spam();
		    }
		    else if(cmdEql(ch,"color"))
		    {
		    	set_background_color();
		    }
			else if(cmdEql(ch,"ver"))
		    {
				ver(mbi);
		    }
			else if(cmdEql(ch,"calc"))
			{
				maths();
			}
			else if(cmdEql(ch,"draw"))
			{
				gpu_mode = 1;
			}
		    else
		    {
				if(check_string(ch) && !cmdEql(ch,"exit")) 
				{
					printf_gay("\n%s isn't a valid command\n", ch);
				}
				else 
				{
					printf("\n");
				}
			}
	} while (!cmdEql(ch,"exit"));
}
