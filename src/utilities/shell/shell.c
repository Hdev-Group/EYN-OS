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
	printf(mbi, "%c\n #######  ##    ##  ###     ##          ######    #####", 255, 0, 255);
    printf(mbi, "%c\n ###       ##  ##   ####    ##         ##    ##  ##", 255, 0, 255);
    printf(mbi, "%c\n #######     ##     ##  ##  ##  #####  ##    ##   #####", 255, 0, 255);
    printf(mbi, "%c\n ###         ##     ##    ####         ##    ##       ##", 255, 0, 255);
    printf(mbi, "%c\n #######     ##     ##      ##          ######    #####", 255, 0, 255);
	printf(mbi, "%c\n (ver. 0.06)\n\n", 200, 200, 200);
}

void help(multiboot_info_t *mbi)
{
	printf(mbi, "%c\n cmd         : Launch a new recursive Shell", 255, 255, 255);
	printf(mbi, "%c\n clear       : Clears the screen", 255, 255, 255);
	printf(mbi, "%c\n echo <expr> : Reprints a given text (if no expr, prints nothing)", 255, 255, 255);
	printf(mbi, "%c\n exit        : Exits the kernel (shutdown)", 255, 255, 255);
	printf(mbi, "%c\n ver         : Shows the current system version", 255, 255, 255);
	printf(mbi, "%c\n spam        : Spam 'EYN-OS' to the shell (100 times)", 255, 255, 255);
	printf(mbi, "%c\n calc <expr> : 32-bit fixed-point calculator (if no expr, prints calc help)\n\n", 255, 255, 255);
}

void calc(string str, multiboot_info_t *mbi) {
    // ignore "calc" and spaces
    uint8 i = 0;
    while (str[i] && str[i] != ' ') i++;
    while (str[i] && str[i] == ' ') i++;
    
    if (!str[i]) {
        printf(mbi, "%c\nUsage: calc <expression>\n", 255, 255, 255);
        printf(mbi, "%c\nExample: calc 2.5+3.7\n", 255, 255, 255);
        return;
    }
    
    // ONLY use static memory for expression
    char expression[200];
    uint8 j = 0;
    
    // copy expression
    while (str[i] && j < 199) {
        expression[j++] = str[i++];
    }
    expression[j] = '\0';
    
    // calculate result
    int32_t result = math_get_current_equation(expression, mbi);
    
    // convert fixed-point to decimal display
    int32_t int_part = result / FIXED_POINT_FACTOR;
    int32_t decimal_part = result % FIXED_POINT_FACTOR;
    if (decimal_part < 0) decimal_part = -decimal_part;
    
    if (decimal_part == 0) {
        // if no decimal part, just print the integer
        printf(mbi, "%c\nResult: %d\n", 255, 255, 255, int_part);
    } else {
        // print with decimal places
        char decimal_str[4];
        decimal_str[0] = '0' + (decimal_part / 100);
        decimal_str[1] = '0' + ((decimal_part / 10) % 10);
        decimal_str[2] = '0' + (decimal_part % 10);
        decimal_str[3] = '\0';
        printf(mbi, "%c\nResult: %d.%s\n", 255, 255, 255, int_part, decimal_str);
    }
}

void launch_shell(int n, multiboot_info_t *mbi)
{
	string ch = (string) malloc(200); // util.h
	string data[64];
	string prompt = "! ";
	printf(mbi, "%c%s", 255, 255, 0, prompt);  // yellow prompt
	do
	{
		ch = readStr(mbi);
		if(cmdEql(ch,"cmd"))
		{
			printf(mbi, "%c\nNew recursive shell opened.\n", 0, 255, 0);  // green
			launch_shell(n+1, mbi);
			printf(mbi, "%c%s", 255, 255, 0, prompt);  // yellow prompt
		}
		else if(cmdEql(ch,"clear"))
		{
			clearScreen(mbi);
			printf(mbi, "%c%s", 255, 255, 0, prompt);  // yellow prompt
		}
		else if(cmdEql(ch,"echo"))
		{
			echo(mbi, ch);
			printf(mbi, "%c%s", 255, 255, 0, prompt);  // yellow prompt
		}
		else if(cmdEql(ch,"help"))
		{
			help(mbi);
			printf(mbi, "%c%s", 255, 255, 0, prompt);  // yellow prompt
		}
		else if(cmdEql(ch,"spam"))
		{
			joke_spam(mbi);
			printf(mbi, "%c%s", 255, 255, 0, prompt);  // yellow prompt
		}
		else if(cmdEql(ch,"ver"))
		{
			ver(mbi);
			printf(mbi, "%c%s", 255, 255, 0, prompt);  // yellow prompt
		}
		else if(cmdEql(ch,"draw"))
		{
			drawRect(mbi, 10, 10, 500, 200, 255, 255, 255);
			printf(mbi, "%c%s", 255, 255, 0, prompt);  // yellow prompt
		}
		else if(cmdEql(ch,"calc"))
		{
			calc(ch, mbi);
			printf(mbi, "%c%s", 255, 255, 0, prompt);  // yellow prompt
		}
		else if(cmdEql(ch,"exit"))
		{
			printf(mbi, "%c\nShutting down...\n", 255, 0, 0);  // red
			Shutdown();
		}
		else
		{
			printf(mbi, "%c\n", 255, 0, 0);
			printf(mbi, "%c%s isn't a valid command\n", 255, 0, 0, ch);  // print error in red
			printf(mbi, "%c%s", 255, 255, 0, prompt);  // yellow prompt
		}
	} while (!cmdEql(ch,"exit"));
}
