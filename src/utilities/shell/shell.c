#include "../../../include/string.h"
#include "../../../include/system.h"
#include "../../../include/shell.h"
#include "../../../include/screen.h"
#include "../../../include/util.h"
#include "../../../include/kb.h"
#include "../../../include/math.h"
#include <stdint.h>

uint32_t __stack_chk_fail_local(){
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
	printf("\n 0 : black");
	printf_colored("\n 1 : blue",1,0);   // screen.h
	printf_colored("\n 2 : green",2,0);
	printf_colored("\n 3 : cyan",3,0);
	printf_colored("\n 4 : red",4,0);
	printf_colored("\n 5 : purple",5,0);
	printf_colored("\n 6 : orange",6,0);
	printf_colored("\n 7 : grey",7,0);
	printf_colored("\n 8 : dark grey",8,0);
	printf_colored("\n 9 : blue light",9,0);
	printf_colored("\n 10 : green light",10,0);
	printf_colored("\n 11 : blue lighter",11,0);
	printf_colored("\n 12 : red light",12,0);
	printf_colored("\n 13 : rose",13,0);
	printf_colored("\n 14 : yellow",14,0);
	printf_colored("\n 15 : white",15,0);

	printf("\n\n  Text color ? : ");
	int text_color = str_to_int(readStr());
	printf("\n\n  Background color ? : ");
	int bg_color = str_to_int(readStr());
	set_screen_color(text_color,bg_color);
	clearScreen();
}

void joke_spam() {
	for (int i = 1; i <= 100; i++) {
		printf_colored(" EYN-OS\n", 12, 0);
	}
	printf("\n");
}

void ver() {
	printf_colored("\n #########   ###     ###   ###      ###              ######      ######", 13, 0);
    printf_colored("\n ###           ### ###     ######   ###            ###    ###  ###", 13, 0);
    printf_colored("\n #########       ###       ###  ### ###   ######   ###    ###    ######", 13, 0);
    printf_colored("\n ###             ###       ###    #####            ###    ###        ###", 13, 0);
    printf_colored("\n #########       ###       ###      ###              ######     ######", 13, 0);
	printf_colored("\n (ver. 0.02)\n\n", 7, 0);
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
	printf("\n calc      : Simple 8-Bit calculator\n\n")
}

void maths()
{
	printf("\n Math symbols:  1 - Add  2 - Minus  3 - Divide  4 - Times\n");

	printf_colored("\n Math symbol", 7, 0);
	printf_colored("! ", 11, 0);
	string symbol = readStr();
	int intsymbol=str_to_int(symbol);

	printf_colored("\n First number", 7, 0);
	printf_colored("! ", 11, 0);
	string strfint = readStr();
	int fint=str_to_int(strfint);

	printf_colored("\n Second number", 7, 0);
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

void launch_shell(int n)
{
	string ch = (string) malloc(200); // util.h
	string data[64];
	int counter = 0;
	do
	{
			printf_colored("! ", 14, 0);
		    ch = readStr();
		    if(cmdEql(ch,"cmd"))
		    {
		            printf("\nNew recursive shell opened.\n");
					launch_shell(n+1);
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
				ver();
		    }
			else if(cmdEql(ch,"calc"))
			{
				maths();
			}
		    else
		    {
				printf("\n'");
				printf(ch);
		        printf("' isn't a valid command\n");
		    }
	} while (!cmdEql(ch,"exit"));
}
