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
		printf(mbi, "\n%s\n", argSrch(ch, 1, 1));

	}
	else
	{
		printf(mbi, "\n");
	}
}

void joke_spam(multiboot_info_t *mbi) 
{
	for (int i = 1; i <= 100; i++) 
	{
		printf(mbi, " EYN-OS\n", 12, 0);
	}
	printf(mbi, "\n");
}

void ver(multiboot_info_t *mbi) {
	printf(mbi, "\n #########   ###     ###   ###      ###              ######      ######", 13, 0);
    printf(mbi, "\n ###           ### ###     ######   ###            ###    ###  ###", 13, 0);
    printf(mbi, "\n #########       ###       ###  ### ###   ######   ###    ###    ######", 13, 0);
    printf(mbi, "\n ###             ###       ###    #####            ###    ###        ###", 13, 0);
    printf(mbi, "\n #########       ###       ###      ###              ######     ######", 13, 0);
	printf(mbi, "\n (ver. 0.05)\n\n", 7, 0);
}

void help(multiboot_info_t *mbi)
{
	printf(mbi, "\n cmd       : Launch a new recursive Shell");
	printf(mbi, "\n clear     : Clears the screen");
	printf(mbi, "\n echo      : Reprints a given text");
	printf(mbi, "\n exit      : Quits the current shell");
	printf(mbi, "\n ver       : Shows the current system version");
	printf(mbi, "\n spam      : Spam 'EYN-OS' to the shell");
	printf(mbi, "\n calc      : Simple 8-Bit calculator\n\n");
}

void launch_shell(int n, multiboot_info_t *mbi)
{
	string ch = (string) malloc(200); // util.h
	string data[64];
	string prompt = "! ";
	do
	{
			printf(mbi, prompt);
		    ch = readStr(mbi);
		    if(cmdEql(ch,"cmd"))
		    {
		            printf(mbi, "\nNew recursive shell opened.\n");
					launch_shell(n+1, mbi);
		    }
		    else if(cmdEql(ch,"clear"))
		    {
		            clearScreen(mbi);
		    }
		    else if(cmdEql(ch,"echo"))
		    {
		    	echo(mbi, ch);
		    }
		    else if(cmdEql(ch,"help"))
		    {
		    	help(mbi);
		    }
		    else if(cmdEql(ch,"spam"))
		    {
		    	joke_spam(mbi);
		    }
			else if(cmdEql(ch,"ver"))
		    {
				ver(mbi);
		    }
			else if(cmdEql(ch,"draw"))
			{
				drawRect(mbi, 10, 10, 500, 200, 255, 255, 255);
			}
		    else
		    {
				printf(mbi, "\n%s isn't a valid command\n", ch);
				if(check_string(ch) && !cmdEql(ch,"exit")) 
				{
				}
				else 
				{
					printf(mbi, "\n");
				}
			}
	} while (!cmdEql(ch,"exit"));
}
