#include "../../../include/string.h"
#include "../../../include/system.h"
#include "../../../include/shell.h"
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

void echo(multiboot_info_t *mbi, string ch)
{
	if (argSrch(ch, 1, 1) != 0)
	{
		printf(mbi, "%c\n%s\n", 255, 255, 255, argSrch(ch, 1, 1));

	}
	else
	{
		printf(mbi, "%c\n", 255, 255, 255);
	}
}

void joke_spam(multiboot_info_t *mbi) 
{
	for (int i = 1; i <= 100; i++) 
	{
		printf(mbi, "%c EYN-OS\n", 255, 0, 255);
	}
	printf(mbi, "%c\n", 255, 255, 255);
}

void ver(multiboot_info_t *mbi) {
	printf(mbi, "%c\n #########   ###     ###   ###      ###              ######      ######", 255, 0, 255);
    printf(mbi, "%c\n ###           ### ###     ######   ###            ###    ###  ###", 255, 0, 255);
    printf(mbi, "%c\n #########       ###       ###  ### ###   ######   ###    ###    ######", 255, 0, 255);
    printf(mbi, "%c\n ###             ###       ###    #####            ###    ###        ###", 255, 0, 255);
    printf(mbi, "%c\n #########       ###       ###      ###              ######     ######", 255, 0, 255);
	printf(mbi, "%c\n (ver. 0.05)\n\n", 200, 200, 200);
}

void help(multiboot_info_t *mbi)
{
	printf(mbi, "%c\n cmd       : Launch a new recursive Shell", 255, 255, 255);
	printf(mbi, "%c\n clear     : Clears the screen", 255, 255, 255);
	printf(mbi, "%c\n echo      : Reprints a given text", 255, 255, 255);
	printf(mbi, "%c\n exit      : Quits the current shell", 255, 255, 255);
	printf(mbi, "%c\n ver       : Shows the current system version", 255, 255, 255);
	printf(mbi, "%c\n spam      : Spam 'EYN-OS' to the shell", 255, 255, 255);
	printf(mbi, "%c\n calc      : Simple 8-Bit calculator\n\n", 255, 255, 255);
}

void launch_shell(int n, multiboot_info_t *mbi)
{
	string ch = (string) malloc(200); // util.h
	string data[64];
	string prompt = "! ";
	printf(mbi, "%c%s", 255, 255, 255, prompt);  // white prompt
	do
	{
		ch = readStr(mbi);
		if(cmdEql(ch,"cmd"))
		{
			printf(mbi, "%c\nNew recursive shell opened.\n", 255, 255, 255);
			launch_shell(n+1, mbi);
			printf(mbi, "%c%s", 255, 255, 255, prompt);
		}
		else if(cmdEql(ch,"clear"))
		{
			clearScreen(mbi);
			printf(mbi, "%c%s", 255, 255, 255, prompt);
		}
		else if(cmdEql(ch,"echo"))
		{
			echo(mbi, ch);
			printf(mbi, "%c%s", 255, 255, 255, prompt);
		}
		else if(cmdEql(ch,"help"))
		{
			help(mbi);
			printf(mbi, "%c%s", 255, 255, 255, prompt);
		}
		else if(cmdEql(ch,"spam"))
		{
			joke_spam(mbi);
			printf(mbi, "%c%s", 255, 255, 255, prompt);
		}
		else if(cmdEql(ch,"ver"))
		{
			ver(mbi);
			printf(mbi, "%c%s", 255, 255, 255, prompt);
		}
		else if(cmdEql(ch,"draw"))
		{
			drawRect(mbi, 10, 10, 500, 200, 255, 255, 255);
			printf(mbi, "%c%s", 255, 255, 255, prompt);
		}
		else
		{
			printf(mbi, "%c\n", 255, 0, 0);  // Start red color
			printf(mbi, "%c%s", 255, 0, 0, ch);  // Print command in red
			printf(mbi, "%c isn't a valid command\n", 255, 0, 0);  // Print rest of message in red
			if(check_string(ch) && !cmdEql(ch,"exit")) 
			{
			}
			else 
			{
				printf(mbi, "%c\n", 255, 255, 255);
			}
			printf(mbi, "%c%s", 255, 255, 255, prompt);
		}
	} while (!cmdEql(ch,"exit"));
}
