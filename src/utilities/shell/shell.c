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
	printf("%c(ver. 0.08)\n", 200, 200, 200);
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
	printf("%clsram       : List files in the RAM FAT32 disk image\n");
	printf("%ccatram <f>  : Display contents of a file from the RAM FAT32 disk image\n");
	printf("%cwriteram <> : Write to a temporary file (in RAM)\n");
	printf("%clsata       : List detected ATA drives\n");
	printf("%clsfat       : List files on the real hard drive (drive 0)\n");
	printf("%ccatfat <f>  : Display contents of a file from the real hard drive (drive 0)\n");
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

// recursive directory listing for lsfat
void lsfat_list_dir(void* disk_img, struct fat32_bpb* bpb, uint32 cluster, int depth, int max_depth, int indent) 
{
    if (depth > max_depth) return;
    uint32 byts_per_sec = bpb->BytsPerSec;
    uint32 sec_per_clus = bpb->SecPerClus;
    uint32 rsvd_sec_cnt = bpb->RsvdSecCnt;
    uint32 num_fats = bpb->NumFATs;
    uint32 fatsz = bpb->FATSz32;
    uint32 first_data_sec = rsvd_sec_cnt + (num_fats * fatsz);
    uint32 cur_cluster = cluster;

    while (cur_cluster < 0x0FFFFFF8) 
	{
        uint32 dir_sec = first_data_sec + ((cur_cluster - 2) * sec_per_clus);
        uint32 dir_offset = dir_sec * byts_per_sec;
        struct fat32_dir_entry* entries = (struct fat32_dir_entry*)((char*)disk_img + dir_offset);
        int entry_count = (byts_per_sec * sec_per_clus) / sizeof(struct fat32_dir_entry);

        for (int i = 0; i < entry_count; i++) 
		{
            if (entries[i].Name[0] == 0x00) break;
            if ((entries[i].Attr & 0x0F) == 0x0F) continue;
            if (entries[i].Name[0] == 0xE5) continue;
            char name[12];
            for (int j = 0; j < 11; j++) name[j] = entries[i].Name[j];
            name[11] = '\0';

            // print indentation
            for (int k = 0; k < indent; k++) printf(" ");
            if (entries[i].Attr & 0x10) 
			{
                printf("%c%s <DIR>\n", 255, 255, 255, name);
                // skip "." and ".." entries
                if (!(name[0] == '.' && (name[1] == ' ' || name[1] == '.'))) 
				{
                    uint32 subdir_cluster = ((uint32)entries[i].FstClusHI << 16) | entries[i].FstClusLO;
                    if (subdir_cluster >= 2) 
					{
                        lsfat_list_dir(disk_img, bpb, subdir_cluster, depth + 1, max_depth, indent + 2);
                    }
                }
            } 
			else 
			{
                printf("%c%s\n", 255, 255, 255, name);
            }
        }
        cur_cluster = fat32_next_cluster(disk_img, bpb, cur_cluster);
    }
}

void lsram(string input) 
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
    // Parse optional depth parameter from input string
    int max_depth = 1;
    uint8 i = 0;
    while (input[i] && input[i] != ' ') i++;
    while (input[i] && input[i] == ' ') i++;
    if (input[i]) {
        // Parse integer
        int val = 0;
        while (input[i] >= '0' && input[i] <= '9') 
		{
            val = val * 10 + (input[i] - '0');
            i++;
        }
        if (val > 0) max_depth = val;
    }
    printf("%cFAT32 directory tree (depth: %d):\n\n", 255, 255, 255, max_depth);
    lsfat_list_dir(fat32_disk_img, &bpb, bpb.RootClus, 1, max_depth, 0);
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

void catram(string ch) 
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
    printf("%c%s\n", 255, 255, 255, buf);
}

void writeram(string ch)
{
    if (!fat32_disk_img) {
        printf("%cFAT32 disk image not loaded!\n", 255, 0, 0);
        return;
    }
    struct fat32_bpb bpb;
    if (fat32_read_bpb(fat32_disk_img, &bpb) != 0) {
        printf("%cFailed to read FAT32 BPB\n", 255, 0, 0);
        return;
    }
    // Parse filename and data from input
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    if (!ch[i]) {
        printf("%cUsage: writefat <filename> <data>\n", 255, 255, 255);
        return;
    }
    char arg[64];
    uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 63) {
        arg[j++] = ch[i++];
    }
    arg[j] = '\0';
    if (strlength(arg) < 1) {
        printf("%cUsage: writefat <filename> <data>\n", 255, 255, 255);
        return;
    }
    char fatname[12];
    to_fat32_83(arg, fatname);
    while (ch[i] && ch[i] == ' ') i++;
    if (!ch[i]) {
        printf("%cUsage: writefat <filename> <data>\n", 255, 255, 255);
        return;
    }
    char data[512];
    j = 0;
    while (ch[i] && j < 511) {
        data[j++] = ch[i++];
    }
    data[j] = '\0';
    int res = fat32_write_file(fat32_disk_img, &bpb, fatname, data, j);
    if (res < 0) {
        printf("%cFailed to write file.\n", 255, 0, 0);
    } else {
        printf("%cFile written successfully.\n", 0, 255, 0);
    }
}

void lsata() {
    for (int d = 0; d < 4; d++) {
        uint16 id[256];
        int res = ata_identify(d, id);
        if (res == 0) {
            // Model string is words 27-46 (40 chars)
            char model[41];
            for (int i = 0; i < 20; i++) {
                model[i*2] = (id[27+i] >> 8) & 0xFF;
                model[i*2+1] = id[27+i] & 0xFF;
            }
            model[40] = '\0';
            // Size in sectors: words 60-61 (little endian)
            uint32 sectors = id[60] | (id[61] << 16);
            uint32 mb = (sectors / 2048); // 512 bytes/sector
            printf("%cDrive %d: %s (%d MB)\n", 255, 255, 255, d, model, mb);
        } else {
            printf("%cDrive %d: not present\n", 255, 255, 255, d);
        }
    }
}

void catfat(string ch) 
{
    struct fat32_bpb bpb;
    if (fat32_read_bpb_sector(0, &bpb) != 0) 
	{
        printf("%cFailed to read FAT32 BPB from drive 0\n", 255, 0, 0);
        return;
    }

    // get filename argument (e.g. test.txt)
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
    int n = fat32_read_file_sector(0, &bpb, fatname, buf, 512);
    if (n < 0) 
	{
        printf("%cFile not found or error reading file.\n", 255, 0, 0);
        return;
    }

    buf[n] = '\0';
    printf("%c%s\n", 255, 255, 255, buf);
}

/// COMMAND LIST ///

// Helper: check for '>' and split command and filename
int parse_redirection(const char* input, char* cmd, char* filename) {
    int i = 0, j = 0, found = 0;
    while (input[i]) {
        if (input[i] == '>') {
            found = 1;
            break;
        }
        cmd[j++] = input[i++];
    }
    cmd[j] = '\0';
    if (!found) return 0;
    i++; // skip '>'
    while (input[i] == ' ') i++;
    j = 0;
    while (input[i] && input[i] != ' ' && j < 63) filename[j++] = input[i++];
    filename[j] = '\0';
    return 1;
}

// Modified echo to support output to buffer
void echo_to_buf(string ch, char* outbuf, int outbufsize) {
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    if (!ch[i]) {
        if (outbuf) outbuf[0] = '\0';
        else printf("%c\n", 255, 255, 255);
        return;
    }
    if (outbuf) {
        int j = 0;
        while (ch[i] && j < outbufsize - 2) outbuf[j++] = ch[i++];
        outbuf[j++] = '\n';
        outbuf[j] = '\0';
    } else {
        printf("%c%s\n", 255, 255, 255, &ch[i]);
    }
}

// Modified calc to support output to buffer
void calc_to_buf(string str, char* outbuf, int outbufsize) {
    uint8 i = 0;
    while (str[i] && str[i] != ' ') i++;
    while (str[i] && str[i] == ' ') i++;
    if (!str[i]) {
        // Manual formatting for usage message
        char* usage = "Usage: calc <expression>\nExample: calc 2.5+3.7\n";
        int j = 0;
        while (usage[j] && j < outbufsize - 1) { outbuf[j] = usage[j]; j++; }
        outbuf[j] = '\0';
        return;
    }
    char expression[200];
    uint8 j = 0;
    while (str[i] && j < 199) expression[j++] = str[i++];
    expression[j] = '\0';
    int32_t result = math_get_current_equation(expression);
    int32_t int_part = result / FIXED_POINT_FACTOR;
    int32_t decimal_part = result % FIXED_POINT_FACTOR;
    if (decimal_part < 0) decimal_part = -decimal_part;
    j = 0;
    char* prefix = "Result: ";
    int k = 0;
    while (prefix[k] && j < outbufsize - 1) outbuf[j++] = prefix[k++];
    // Convert int_part to string
    char intbuf[16];
    int_to_ascii(int_part, intbuf);
    k = 0;
    while (intbuf[k] && j < outbufsize - 1) outbuf[j++] = intbuf[k++];
    if (decimal_part == 0) {
        if (j < outbufsize - 1) outbuf[j++] = '\n';
        outbuf[j] = '\0';
    } else {
        if (j < outbufsize - 1) outbuf[j++] = '.';
        // Convert decimal_part to 3 digits
        int d1 = (decimal_part / 100) % 10;
        int d2 = (decimal_part / 10) % 10;
        int d3 = decimal_part % 10;
        if (j < outbufsize - 1) outbuf[j++] = '0' + d1;
        if (j < outbufsize - 1) outbuf[j++] = '0' + d2;
        if (j < outbufsize - 1) outbuf[j++] = '0' + d3;
        if (j < outbufsize - 1) outbuf[j++] = '\n';
        outbuf[j] = '\0';
    }
}

void lsfat(string input) {
    struct fat32_bpb bpb;
    if (fat32_read_bpb_sector(0, &bpb) != 0) {
        printf("%cFailed to read FAT32 BPB from drive 0\n", 255, 0, 0);
        return;
    }
    printf("%cFAT32 files on drive 0:\n\n", 255, 255, 255);
    if (fat32_list_root_sector(0, &bpb) != 0) {
        printf("%cFailed to list root directory\n", 255, 0, 0);
        return;
    }
}

void launch_shell(int n)
{
	string ch = (string) malloc(200); // util.h
	string data[64];
	printf("%c%s", 255, 255, 0, "! ");  // yellow prompt
	do
	{
		ch = readStr();
		printf("\n");
		char cmd[200], filename[64];
		int is_redirect = parse_redirection(ch, cmd, filename);
		char outbuf[512];
		if (is_redirect && fat32_disk_img) {
			struct fat32_bpb bpb;
			fat32_read_bpb(fat32_disk_img, &bpb);
			char fatname[12];
			to_fat32_83(filename, fatname);
			outbuf[0] = '\0';
			if (cmdEql(cmd, "echo")) {
				echo_to_buf(cmd, outbuf, 512);
			} else if (cmdEql(cmd, "calc")) {
				calc_to_buf(cmd, outbuf, 512);
			} else {
				// fallback: not supported for redirection
				char* msg = "Redirection not supported for this command.\n";
				int k = 0;
				while (msg[k] && k < 511) { outbuf[k] = msg[k]; k++; }
				outbuf[k] = '\0';
			}
			int res = fat32_write_file(fat32_disk_img, &bpb, fatname, outbuf, strlength(outbuf));
			if (res < 0) printf("%cFailed to write file.\n", 255, 0, 0);
			else printf("%cFile written successfully.\n", 0, 255, 0);
		} else {
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
			else if(cmdEql(ch,"lsram"))
			{
				printf("%c", 255, 255, 255);
				lsram(ch);
			}
			else if(cmdEql(ch,"catram"))
			{
				catram(ch);
			}
			else if(cmdEql(ch,"writeram"))
			{
				writeram(ch);
			}
			else if(cmdEql(ch,"lsata"))
			{
				lsata();
			}
			else if(cmdEql(ch,"lsfat"))
			{
				lsfat(ch);
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
		}
		if (!cmdEql(ch,"exit")) 
		{
			printf("%c%s", 255, 255, 0, "! ");  // yellow prompt
		}
	} 
	while (!cmdEql(ch,"exit"));
}
