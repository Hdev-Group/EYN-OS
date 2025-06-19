#include "../../../include/string.h"
#include "../../../include/system.h"
#include "../../../include/shell.h"
#include "../../../include/util.h"
#include "../../../include/kb.h"
#include "../../../include/math.h"
#include "../../../include/multiboot.h"
#include "../../../include/vga.h"
#include "../../../include/fat32.h"
#include <stdint.h>

extern void* fat32_disk_img;
extern multiboot_info_t *g_mbi;

uint32_t __stack_chk_fail_local() // shutting the compiler up
{
    return 0;
}

/// COMMANDS ///

void echo(string ch)
{
	// Skip past "echo" and any spaces
	uint8 i = 0;
	while (ch[i] && ch[i] != ' ') i++;
	while (ch[i] && ch[i] == ' ') i++;

	if (!ch[i]) {
		// No arguments provided
		printf("%c\n", 255, 255, 255);
		return;
	}

	// Print the rest of the string on a new line
	printf("%c%s\n", 255, 255, 255, &ch[i]);
}

void joke_spam() 
{
	for (int i = 1; i <= 100; i++) 
	{
		printf("%c EYN-OS\n", 255, 0, 255);
	}
	printf("%c\n", 255, 255, 255);
}

void ver() 
{
	printf("%c#######  ##    ##  ###     ##          ######    #####\n", 255, 0, 255);
    printf("%c###       ##  ##   ####    ##         ##    ##  ##\n");
    printf("%c#######     ##     ##  ##  ##  #####  ##    ##   #####\n");
    printf("%c###         ##     ##    ####         ##    ##       ##\n");
    printf("%c#######     ##     ##      ##          ######    #####\n");
	printf("%c(ver. 0.07)\n", 200, 200, 200);
}

void help()
{
	printf("%chelp        : Display this message\n", 255, 255, 255);
	printf("%cclear       : Clears the screen\n");
	printf("%cecho <expr> : Reprints a given text (if no expr, prints nothing)\n");
	printf("%cexit        : Exits the kernel (shutdown)\n");
	printf("%cver         : Shows the current system version\n");
	printf("%cspam        : Spam 'EYN-OS' to the shell (100 times)\n");
	printf("%ccalc <expr> : 32-bit fixed-point calculator (if no expr, prints calc help)\n");
	printf("%cdraw <expr> : Draw an example rectangle (if no expr, prints draw help)\n");
	printf("%clsfat       : List files in the FAT32 disk image\n");
	printf("%ccatfat <f>  : Display contents of a file from the FAT32 disk image\n");
}

void calc(string str) 
{
    // ignore "calc" and spaces
    uint8 i = 0;
    while (str[i] && str[i] != ' ') i++;
    while (str[i] && str[i] == ' ') i++;
    
    if (!str[i]) 
	{
        printf("%cUsage: calc <expression>\n", 255, 255, 255);
        printf("%cExample: calc 2.5+3.7\n");
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
    int32_t result = math_get_current_equation(expression);
    
    // convert fixed-point to decimal display
    int32_t int_part = result / FIXED_POINT_FACTOR;
    int32_t decimal_part = result % FIXED_POINT_FACTOR;
    if (decimal_part < 0) decimal_part = -decimal_part;
    
    if (decimal_part == 0) 
	{
        // if no decimal part, just print the integer
        printf("%cResult: %d\n", 255, 255, 255, int_part);
    } 
	else 
	{
        // print with decimal places
        char decimal_str[4];
        decimal_str[0] = '0' + (decimal_part / 100);
        decimal_str[1] = '0' + ((decimal_part / 10) % 10);
        decimal_str[2] = '0' + (decimal_part % 10);
        decimal_str[3] = '\0';
        printf("%cResult: %d.%s\n", 255, 255, 255, int_part, decimal_str);
    }
}

void draw_cmd_handler(string ch) 
{
    // default values
    int x = 10, y = 10, w = 500, h = 200, r = 255, g = 255, b = 255;

    // find the argument part after 'draw'
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    if (!ch[i]) 
	{
        // No parameters provided, print help
        printf("%cUsage: draw <x> <y> <width> <height> <r> <g> <b>\n", 255, 255, 255);
        printf("%cExample: draw 10 20 100 50 255 0 0\n");
        printf("%cIf you provide fewer than 7 parameters, defaults will be used for the rest.\n");
        return;
    }

    // Copy the argument part
    char argstr[128];
    uint8 j = 0;
    while (ch[i] && j < 127) 
	{
        argstr[j++] = ch[i++];
    }
    argstr[j] = '\0';
    // Now parse up to 7 integers separated by whitespace
    int vals[7] = {x, y, w, h, r, g, b};
    int val_idx = 0;
    char numbuf[16];
    int ni = 0;

    for (uint8 k = 0; argstr[k] && val_idx < 7; k++) 
	{
        if ((argstr[k] >= '0' && argstr[k] <= '9') || argstr[k] == '-') 
		{
            numbuf[ni++] = argstr[k];
        } else if (argstr[k] == ' ' || argstr[k] == '\t') 
		{
            if (ni > 0) 
			{
                numbuf[ni] = '\0';
                vals[val_idx++] = str_to_int(numbuf);
                ni = 0;
            }
        }
    }

    if (ni > 0 && val_idx < 7) 
	{
        numbuf[ni] = '\0';
        vals[val_idx++] = str_to_int(numbuf);
    }

    x = vals[0]; y = vals[1]; w = vals[2]; h = vals[3]; r = vals[4]; g = vals[5]; b = vals[6];
    drawRect(x, y, w, h, r, g, b);
	printf("%cShape drawn.\n", 255, 255, 255);
}

void lsfat() 
{
    if (!fat32_disk_img) {
        printf("%cFAT32 disk image not loaded!\n", 255, 0, 0);
        return;
    }

    struct fat32_bpb bpb;

    if (fat32_read_bpb(fat32_disk_img, &bpb) != 0) 
	{
        printf("%cFailed to read FAT32 BPB\n", 255, 0, 0);
        return;
    }

    printf("%cFAT32 root directory entries:\n\n", 255, 255, 255);
    fat32_list_root(fat32_disk_img, &bpb);
}

void to_fat32_83(const char* input, char* output)
{
    // convert input like "test.txt" to "TEST    TXT" (cancerous)
    int i = 0, j = 0;
    // copy name part (up to dot or 8 chars)
    while (input[i] && input[i] != '.' && j < 8) 
	{
        char c = input[i];
        if (c >= 'a' && c <= 'z') c -= 32; // to upper
        output[j++] = c;
        i++;
    }

    // pad with spaces
    while (j < 8) output[j++] = ' ';
    // if dot, skip it
    if (input[i] == '.') i++;
    int ext = 0;
    // copy extension (up to 3 chars)
    while (input[i] && ext < 3) 
	{
        char c = input[i];
        if (c >= 'a' && c <= 'z') c -= 32;
        output[j++] = c;
        i++; ext++;
    }

    // pad extension with spaces
    while (ext < 3) 
	{
		output[j++] = ' '; ext++; 
	}
    output[j] = '\0';
}

void catfat(string ch) 
{
    if (!fat32_disk_img) 
	{
        printf("%cFAT32 disk image not loaded!\n", 255, 0, 0);
        return;
    }

    struct fat32_bpb bpb;
    if (fat32_read_bpb(fat32_disk_img, &bpb) != 0) 
	{
        printf("%cFailed to read FAT32 BPB\n", 255, 0, 0);
        return;
    }

    // get filename argument (e.g. test.txt)
    // Inline argument extraction: skip command and spaces, then copy next word
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    if (!ch[i]) 
	{
        printf("%cUsage: catfat <filename>\nExample: catfat test.txt\n", 255, 255, 255);
        return;
    }
    char arg[64];
    uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 63) {
        arg[j++] = ch[i++];
    }
    arg[j] = '\0';
    if (strlength(arg) < 1) 
	{
        printf("%cUsage: catfat <filename>\nExample: catfat test.txt\n", 255, 255, 255);
        return;
    }

    char fatname[12];
    to_fat32_83(arg, fatname);
    char buf[513];
	printf("%c", 255, 255, 255);
    int n = fat32_read_file(fat32_disk_img, &bpb, fatname, buf, 512);
    if (n < 0) 
	{
        printf("%cFile not found or error reading file.\n", 255, 0, 0);
        return;
    }

    buf[n] = '\0';
    printf("%c%s", 255, 255, 255, buf);
}

/// COMMAND LIST ///

void launch_shell(int n)
{
	string ch = (string) malloc(200); // util.h
	string data[64];
	printf("%c%s", 255, 255, 0, "! ");  // yellow prompt
	do
	{
		ch = readStr();
		printf("\n");
		if(cmdEql(ch,"cmd"))
		{
			printf("%c\nNew recursive shell opened.\n", 0, 255, 0);  // green
			launch_shell(n+1);
		}
		else if(cmdEql(ch,"clear"))
		{
			clearScreen();
		}
		else if(cmdEql(ch,"echo"))
		{
			echo(ch);
		}
		else if(cmdEql(ch,"help"))
		{
			help();
		}
		else if(cmdEql(ch,"spam"))
		{
			joke_spam();
		}
		else if(cmdEql(ch,"ver"))
		{
			ver();
		}
		else if(cmdEql(ch,"draw"))
		{
			draw_cmd_handler(ch);
		}
		else if(cmdEql(ch,"calc"))
		{
			calc(ch);
		}
		else if(cmdEql(ch,"lsfat"))
		{
			printf("%c", 255, 255, 255);
			lsfat();
		}
		else if(cmdEql(ch,"catfat"))
		{
			catfat(ch);
		}
		else if(cmdEql(ch,"exit"))
		{
			printf("%c\nShutting down...\n", 255, 0, 0);  // red
			Shutdown();
		}
		else
		{
			// check if input is empty or space (THIS CODE IS HORRIBLE IM SORRY)
			uint8 empty = 1;

			for (uint8 ci = 0; ch[ci]; ci++) 
			{
				if (ch[ci] != ' ' && ch[ci] != '\t' && ch[ci] != '\n' && ch[ci] != '\r') 
				{
					empty = 0;
					break;
				}
			}

			if (empty) 
			{
				printf("%c", 255, 255, 255);
			} 
			else 
			{
				printf("%c%s isn't a valid command\n", 255, 0, 0, ch);  // print error in red
			}
		}
		
		if (!cmdEql(ch,"exit")) 
		{
			printf("%c%s", 255, 255, 0, "! ");  // yellow prompt
		}
	} 
	while (!cmdEql(ch,"exit"));
}
