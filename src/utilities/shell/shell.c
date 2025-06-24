#include "../../../include/string.h"
#include "../../../include/system.h"
#include "../../../include/shell.h"
#include "../../../include/util.h"
#include "../../../include/kb.h"
#include "../../../include/math.h"
#include "../../../include/multiboot.h"
#include "../../../include/vga.h"
#include "../../../include/fat32.h"
#include "../../../include/eynfs.h"
#include <stdint.h>

extern void* fat32_disk_img;
extern multiboot_info_t *g_mbi;
extern void poll_keyboard_for_ctrl_c();

void writefat(string ch);
void fdisk_list();
void fdisk_create_partition(uint32 start_lba, uint32 size, uint8 type);
void fdisk_cmd_handler(string ch);
void format_cmd_handler(string ch);

// EYNFS integration: assume superblock at LBA 2048 on drive 0
#define EYNFS_SUPERBLOCK_LBA 2048
#define EYNFS_DRIVE 0

// Global variable for current drive (default 0)
static uint8_t g_current_drive = 0;

// Command to switch current drive
void drive_cmd(string ch);

// Function prototypes for helpers used in this file
void to_fat32_83(const char* input, char* output);
int parse_redirection(const char* input, char* cmd, char* filename);
void echo_to_buf(string ch, char* outbuf, int outbufsize);
void calc_to_buf(string str, char* outbuf, int outbufsize);
uint32 str_to_uint(const char* s);
static int load_eynfs_superblock(eynfs_superblock_t *sb);

// Unified ls command: detects EYNFS or FAT32 and lists files
void ls(string input) {
    uint8 disk = g_current_drive;
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(disk, EYNFS_SUPERBLOCK_LBA, &sb) == 0 && sb.magic == EYNFS_MAGIC) {
        eynfs_dir_entry_t entries[EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t)];
        int count = eynfs_read_dir_table(disk, sb.root_dir_block, entries, EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t));
        if (count < 0) {
            printf("%cFailed to read EYNFS root directory\n", 255, 0, 0);
            return;
        }
        printf("%cEYNFS root directory (drive %d):\n", 255, 255, 255, disk);
        for (int i = 0; i < count; ++i) {
            if (entries[i].name[0] == '\0') continue;
            if (entries[i].type == EYNFS_TYPE_DIR)
                printf("%c%s <DIR>\n", 255, 255, 255, entries[i].name);
            else
                printf("%c%s\n", 255, 255, 255, entries[i].name);
        }
        return;
    }
    uint32 partition_lba_start = fat32_get_partition_lba_start(disk);
    struct fat32_bpb bpb;
    if (fat32_read_bpb_sector(disk, partition_lba_start, &bpb) == 0) {
        printf("%cFAT32 files on drive %d (Partition starts at LBA %d):\n\n", 255, 255, 255, disk, partition_lba_start);
        uint32 byts_per_sec = bpb.BytsPerSec;
        uint32 sec_per_clus = bpb.SecPerClus;
        uint32 rsvd_sec_cnt = bpb.RsvdSecCnt;
        uint32 num_fats = bpb.NumFATs;
        uint32 fatsz = bpb.FATSz32;
        uint32 root_clus = bpb.RootClus;
        uint32 first_data_sec = rsvd_sec_cnt + (num_fats * fatsz);
        uint32 cluster = root_clus;
        uint8 sector[512];
        uint8 fat[512];
        extern volatile int g_user_interrupt;
        g_user_interrupt = 0;
        while (cluster < 0x0FFFFFF8) {
            uint32 cluster_first_sec = first_data_sec + ((cluster - 2) * sec_per_clus);
            for (uint32 sec = 0; sec < sec_per_clus; sec++) {
                if (ata_read_sector(disk, partition_lba_start + cluster_first_sec + sec, sector) != 0) {
                    printf("%cFailed to read root directory sector %d\n", 255, 0, 0, sec);
                    return;
                }
                struct fat32_dir_entry* entries = (struct fat32_dir_entry*)sector;
                for (int i = 0; i < 16; i++) {
                    if (entries[i].Name[0] == 0x00) return;
                    if ((entries[i].Attr & 0x0F) == 0x0F) continue;
                    if (entries[i].Name[0] == 0xE5) continue;
                    char name[12];
                    for (int j = 0; j < 11; j++) name[j] = entries[i].Name[j];
                    name[11] = '\0';
                    if (entries[i].Attr & 0x10) {
                        printf("%c%s <DIR>\n", 255, 255, 255, name);
                    } else {
                        printf("%c%s\n", 255, 255, 255, name);
                    }
                }
            }
            uint32 fat_sector = rsvd_sec_cnt + ((cluster * 4) / 512);
            if (ata_read_sector(disk, fat_sector, fat) != 0) break;
            uint32* fat32 = (uint32*)fat;
            uint32 fat_index = (cluster % (512 / 4));
            uint32 next_cluster = fat32[fat_index] & 0x0FFFFFFF;
            cluster = next_cluster;
        }
        return;
    }
    printf("%cNo supported filesystem found on drive %d.\n", 255, 0, 0, disk);
}

// Unified cat command: detects EYNFS or FAT32 and displays file contents
void cat(string ch) {
    uint8 disk = g_current_drive;
    // Parse filename
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    if (!ch[i]) {
        printf("%cUsage: cat <filename>\n", 255, 255, 255);
        return;
    }
    char arg[64];
    uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 63) arg[j++] = ch[i++];
    arg[j] = '\0';
    if (strlength(arg) < 1) {
        printf("%cUsage: cat <filename>\n", 255, 255, 255);
        return;
    }
    // Try EYNFS first
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(disk, EYNFS_SUPERBLOCK_LBA, &sb) == 0 && sb.magic == EYNFS_MAGIC) {
        eynfs_dir_entry_t entry;
        uint32_t entry_idx;
        if (eynfs_find_in_dir(disk, &sb, sb.root_dir_block, arg, &entry, &entry_idx) != 0 || entry.type != EYNFS_TYPE_FILE) {
            printf("%cFile not found in EYNFS root directory.\n", 255, 0, 0);
            return;
        }
        char buf[EYNFS_BLOCK_SIZE+1];
        int n = eynfs_read_file(disk, &sb, &entry, buf, EYNFS_BLOCK_SIZE);
        if (n < 0) {
            printf("%cFailed to read file.\n", 255, 0, 0);
            return;
        }
        buf[n] = '\0';
        printf("%c%s\n", 255, 255, 255, buf);
        return;
    }
    // Try FAT32
    uint32 partition_lba_start = fat32_get_partition_lba_start(disk);
    struct fat32_bpb bpb;
    if (fat32_read_bpb_sector(disk, partition_lba_start, &bpb) == 0) {
        char fatname[12];
        to_fat32_83(arg, fatname);
        // Find the file entry in the root directory (scan all clusters)
        uint32 byts_per_sec = bpb.BytsPerSec;
        uint32 sec_per_clus = bpb.SecPerClus;
        uint32 rsvd_sec_cnt = bpb.RsvdSecCnt;
        uint32 num_fats = bpb.NumFATs;
        uint32 fatsz = bpb.FATSz32;
        uint32 root_clus = bpb.RootClus;
        uint32 first_data_sec = rsvd_sec_cnt + (num_fats * fatsz);
        struct fat32_dir_entry entry;
        int found = 0;
        uint8 sector[512];
        uint8 fat[512];
        uint32 cluster = root_clus;
        while (cluster < 0x0FFFFFF8 && !found) {
            uint32 cluster_first_sec = first_data_sec + ((cluster - 2) * sec_per_clus);
            for (uint32 sec = 0; sec < sec_per_clus; sec++) {
                if (ata_read_sector(disk, partition_lba_start + cluster_first_sec + sec, sector) != 0) break;
                struct fat32_dir_entry* entries = (struct fat32_dir_entry*)sector;
                for (int i = 0; i < 16; i++) {
                    if (entries[i].Name[0] == 0x00) break;
                    if ((entries[i].Attr & 0x0F) == 0x0F) continue;
                    if (entries[i].Name[0] == 0xE5) continue;
                    int match = 1;
                    for (int j = 0; j < 11; j++) {
                        if (entries[i].Name[j] != fatname[j]) {
                            match = 0;
                            break;
                        }
                    }
                    if (match) {
                        entry = entries[i];
                        found = 1;
                        break;
                    }
                }
                if (found) break;
            }
            // Walk to next cluster in root dir
            uint32 fat_sector = rsvd_sec_cnt + ((cluster * 4) / 512);
            if (ata_read_sector(disk, fat_sector, fat) != 0) break;
            uint32* fat32 = (uint32*)fat;
            uint32 fat_index = (cluster % (512 / 4));
            uint32 next_cluster = fat32[fat_index] & 0x0FFFFFFF;
            cluster = next_cluster;
        }
        if (!found) {
            printf("%cFile not found or error reading file.\n", 255, 0, 0);
            return;
        }
        uint32 file_size = entry.FileSize;
        uint32 first_clus = ((uint32)entry.FstClusHI << 16) | entry.FstClusLO;
        if (first_clus < 2) {
            printf("%cInvalid first cluster.\n", 255, 0, 0);
            return;
        }
        // Walk the FAT chain and print each cluster, slow scroll with interrupt
        uint32 cluster2 = first_clus;
        uint32 bytes_read = 0;
        extern volatile int g_user_interrupt;
        g_user_interrupt = 0;
        while (cluster2 < 0x0FFFFFF8 && bytes_read < file_size) {
            uint32 data_sec = first_data_sec + ((cluster2 - 2) * sec_per_clus);
            char* data_ptr = (char*)fat32_disk_img + data_sec * byts_per_sec;
            for (uint32 s = 0; s < sec_per_clus && bytes_read < file_size; s++) {
                char* sec_ptr = data_ptr + s * byts_per_sec;
                uint32 to_copy = byts_per_sec;
                if (bytes_read + to_copy > file_size) to_copy = file_size - bytes_read;
                for (uint32 k = 0; k < to_copy; k++) {
                    putchar(sec_ptr[k]);
                    poll_keyboard_for_ctrl_c();
                    if (sec_ptr[k] == '\n') {
                        sleep(30); // ~1/5 second, adjust as needed
                        if (g_user_interrupt) {
                            printf("\n^C [Interrupted by user]\n", 255, 0, 0);
                            g_user_interrupt = 0;
                            return;
                        }
                    }
                }
                bytes_read += to_copy;
            }
            // Read the correct FAT sector for this cluster
            cluster2 = fat32_next_cluster_sector(disk, partition_lba_start, &bpb, cluster2);
        }
        printf("\n");
        return;
    }
    printf("%cNo supported filesystem found.\n", 255, 0, 0);
}

// Unified write command: detects EYNFS or FAT32 and writes file contents
void write(string ch) {
    uint8 disk = g_current_drive;
    // Parse filename and data
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    if (!ch[i]) {
        printf("%cUsage: write <filename> <data>\n", 255, 255, 255);
        return;
    }
    char arg[64];
    uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 63) arg[j++] = ch[i++];
    arg[j] = '\0';
    if (strlength(arg) < 1) {
        printf("%cUsage: write <filename> <data>\n", 255, 255, 255);
        return;
    }
    while (ch[i] && ch[i] == ' ') i++;
    if (!ch[i]) {
        printf("%cUsage: write <filename> <data>\n", 255, 255, 255);
        return;
    }
    // Try EYNFS first
    eynfs_superblock_t sb;
    if (load_eynfs_superblock(&sb) == 0 && sb.magic == EYNFS_MAGIC) {
        char data[EYNFS_BLOCK_SIZE];
        j = 0;
        while (ch[i] && j < EYNFS_BLOCK_SIZE-1) data[j++] = ch[i++];
        data[j] = '\0';
        // Try to find the file
        eynfs_dir_entry_t entry;
        uint32_t entry_idx;
        if (eynfs_find_in_dir(disk, &sb, sb.root_dir_block, arg, &entry, &entry_idx) != 0) {
            // Not found, create it
            if (eynfs_create_entry(disk, &sb, sb.root_dir_block, arg, EYNFS_TYPE_FILE) != 0) {
                printf("%cFailed to create file.\n", 255, 0, 0);
                return;
            }
            // Find again
            if (eynfs_find_in_dir(disk, &sb, sb.root_dir_block, arg, &entry, &entry_idx) != 0) {
                printf("%cFailed to find file after creation.\n", 255, 0, 0);
                return;
            }
        }
        int n = eynfs_write_file(disk, &sb, &entry, data, strlength(data), sb.root_dir_block, entry_idx);
        if (n < 0) {
            printf("%cFailed to write file.\n", 255, 0, 0);
            return;
        }
        printf("%cFile written successfully.\n", 0, 255, 0);
        return;
    }
    // Try FAT32
    uint32 partition_lba_start = fat32_get_partition_lba_start(disk);
    struct fat32_bpb bpb;
    if (fat32_read_bpb_sector(disk, partition_lba_start, &bpb) == 0) {
        char fatname[12];
        to_fat32_83(arg, fatname);
        char data[512];
        j = 0;
        while (ch[i] && j < 511) data[j++] = ch[i++];
        data[j] = '\0';
        int res = fat32_write_file_sector(disk, partition_lba_start, &bpb, fatname, data, j);
        if (res < 0) {
            printf("%cFailed to write file to disk. Error %d\n", 255, 0, 0, res);
        } else {
            printf("%cFile written successfully to disk.\n", 0, 255, 0);
        }
        return;
    }
    printf("%cNo supported filesystem found.\n", 255, 0, 0);
}

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
	printf("%c(ver. 0.10)\n", 200, 200, 200);
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
	printf("%cdraw <>     : Draw an example rectangle (if no expr, prints draw help)\n");
	printf("%clsata       : List detected ATA drives\n");
	printf("%cfdisk       : List partition table (use 'fdisk create' to create partitions)\n");
	printf("%cformat <>   : Format partition n (1-4) as FAT32 or EYNFS\n");
	printf("%ccat <file>  : Display contents of a file\n");
	printf("%cls          : List files in the root directory (of drive selected)\n");
	printf("%cwrite <>    : Write to a file in EYNFS root\n");
    printf("%cdrive <n>   : Change between different drives (from lsata)\n");
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
    // Custom implementation with scrolling and Ctrl+C for root dir only
    uint32 byts_per_sec = bpb.BytsPerSec;
    uint32 sec_per_clus = bpb.SecPerClus;
    uint32 rsvd_sec_cnt = bpb.RsvdSecCnt;
    uint32 num_fats = bpb.NumFATs;
    uint32 fatsz = bpb.FATSz32;
    uint32 root_clus = bpb.RootClus;
    uint32 first_data_sec = rsvd_sec_cnt + (num_fats * fatsz);
    uint32 cluster = root_clus;
    uint8 sector[512];
    extern volatile int g_user_interrupt;
    g_user_interrupt = 0;
    while (cluster < 0x0FFFFFF8) {
        uint32 cluster_first_sec = first_data_sec + ((cluster - 2) * sec_per_clus);
        for (uint32 sec = 0; sec < sec_per_clus; sec++) {
            struct fat32_dir_entry* entries = (struct fat32_dir_entry*)((char*)fat32_disk_img + (cluster_first_sec + sec) * byts_per_sec);
            int entry_count = byts_per_sec / sizeof(struct fat32_dir_entry);
            for (int i = 0; i < entry_count; i++) {
                if (entries[i].Name[0] == 0x00) return;
                if ((entries[i].Attr & 0x0F) == 0x0F) continue;
                if (entries[i].Name[0] == 0xE5) continue;
                char name[12];
                for (int j = 0; j < 11; j++) name[j] = entries[i].Name[j];
                name[11] = '\0';
                if (entries[i].Attr & 0x10) {
                    printf("%c%s <DIR>\n", 255, 255, 255, name);
                } else {
                    printf("%c%s\n", 255, 255, 255, name);
                }
                poll_keyboard_for_ctrl_c();
                sleep(30); // ~1/5 second
                if (g_user_interrupt) {
                    printf("\n^C [Interrupted by user]\n", 255, 0, 0);
                    g_user_interrupt = 0;
                    return;
                }
            }
        }
        cluster = fat32_next_cluster(fat32_disk_img, &bpb, cluster);
    }
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
        printf("%cUsage: catram <filename>\nExample: catram test.txt\n", 255, 255, 255);
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
        printf("%cUsage: catram <filename>\nExample: catram test.txt\n", 255, 255, 255);
        return;
    }

    char fatname[12];
    to_fat32_83(arg, fatname);

    // Find the file entry in the root directory (scan all clusters)
    uint32 byts_per_sec = bpb.BytsPerSec;
    uint32 sec_per_clus = bpb.SecPerClus;
    uint32 rsvd_sec_cnt = bpb.RsvdSecCnt;
    uint32 num_fats = bpb.NumFATs;
    uint32 fatsz = bpb.FATSz32;
    uint32 root_clus = bpb.RootClus;
    uint32 first_data_sec = rsvd_sec_cnt + (num_fats * fatsz);

    struct fat32_dir_entry entry;
    int found = 0;
    uint32 cluster = root_clus;
    while (cluster < 0x0FFFFFF8 && !found) {
        uint32 cluster_first_sec = first_data_sec + ((cluster - 2) * sec_per_clus);
        for (uint32 sec = 0; sec < sec_per_clus; sec++) {
            struct fat32_dir_entry* entries = (struct fat32_dir_entry*)((char*)fat32_disk_img + (cluster_first_sec + sec) * byts_per_sec);
            int entry_count = byts_per_sec / sizeof(struct fat32_dir_entry);
            for (int i = 0; i < entry_count; i++) {
                if (entries[i].Name[0] == 0x00) break;
                if ((entries[i].Attr & 0x0F) == 0x0F) continue;
                if (entries[i].Name[0] == 0xE5) continue;
                int match = 1;
                for (int j = 0; j < 11; j++) {
                    if (entries[i].Name[j] != fatname[j]) {
                        match = 0;
                        break;
                    }
                }
                if (match) {
                    entry = entries[i];
                    found = 1;
                    break;
                }
            }
            if (found) break;
        }
        cluster = fat32_next_cluster(fat32_disk_img, &bpb, cluster);
    }
    if (!found) {
        printf("%cFile not found or error reading file.\n", 255, 0, 0);
        return;
    }

    uint32 file_size = entry.FileSize;
    uint32 first_clus = ((uint32)entry.FstClusHI << 16) | entry.FstClusLO;
    if (first_clus < 2) {
        printf("%cInvalid first cluster.\n", 255, 0, 0);
        return;
    }

    // Walk the FAT chain and print each cluster, slow scroll with interrupt
    uint32 cluster2 = first_clus;
    uint32 bytes_read = 0;
    extern volatile int g_user_interrupt;
    g_user_interrupt = 0;
    while (cluster2 < 0x0FFFFFF8 && bytes_read < file_size) {
        uint32 data_sec = first_data_sec + ((cluster2 - 2) * sec_per_clus);
        char* data_ptr = (char*)fat32_disk_img + data_sec * byts_per_sec;
        for (uint32 s = 0; s < sec_per_clus && bytes_read < file_size; s++) {
            char* sec_ptr = data_ptr + s * byts_per_sec;
            uint32 to_copy = byts_per_sec;
            if (bytes_read + to_copy > file_size) to_copy = file_size - bytes_read;
            for (uint32 k = 0; k < to_copy; k++) {
                putchar(sec_ptr[k]);
                poll_keyboard_for_ctrl_c();
                if (sec_ptr[k] == '\n') {
                    sleep(30); // ~1/5 second
                    if (g_user_interrupt) {
                        printf("\n^C [Interrupted by user]\n", 255, 0, 0);
                        g_user_interrupt = 0;
                        return;
                    }
                }
            }
            bytes_read += to_copy;
        }
        cluster2 = fat32_next_cluster(fat32_disk_img, &bpb, cluster2);
    }
    printf("\n");
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
    for (int d = 0; d < 8; d++) { // Show up to 8 possible drives
        uint16 id[256];
        int res = ata_identify(d, id);
        if (res == 0) {
            char model[41];
            for (int i = 0; i < 20; i++) {
                model[i*2] = (id[27+i] >> 8) & 0xFF;
                model[i*2+1] = id[27+i] & 0xFF;
            }
            model[40] = '\0';
            uint32 sectors = id[60] | (id[61] << 16);
            uint32 mb = (sectors / 2048);
            printf("%cDrive %d: %s (%d MB)\n", 255, 255, 255, d, model, mb);
        } else {
            printf("%cDrive %d: not present or not responding\n", 255, 255, 255, d);
        }
    }
}

void launch_shell(int n)
{
	string ch = (string) my_malloc(200); // util.h
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
			else if(cmdEql(ch,"cat"))
			{
				cat(ch);
			}
			else if(cmdEql(ch,"fdisk"))
			{
				fdisk_cmd_handler(ch);
			}
			else if(cmdEql(ch,"format"))
			{
				format_cmd_handler(ch);
			}
			else if(cmdEql(ch,"ls"))
			{
				ls(ch);
			}
			else if(cmdEql(ch,"write"))
			{
				write(ch);
			}
			else if(cmdEql(ch,"exit"))
			{
				printf("%c\nShutting down...\n", 255, 0, 0);  // red
				Shutdown();
			}
			else if(cmdEql(ch,"drive")) { drive_cmd(ch); }
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

void writefat(string ch)
{
    uint32 partition_lba_start = fat32_get_partition_lba_start(0);
    struct fat32_bpb bpb;
    if (fat32_read_bpb_sector(0, partition_lba_start, &bpb) != 0) {
        printf("%cFailed to read FAT32 BPB from drive 0\n", 255, 0, 0);
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
    int res = fat32_write_file_sector(0, partition_lba_start, &bpb, fatname, data, j);
    if (res < 0) {
        printf("%cFailed to write file to disk. Error %d\n", 255, 0, 0, res);
    } else {
        printf("%cFile written successfully to disk.\n", 0, 255, 0);
    }
}

void fdisk_list() {
    uint8 mbr[512];
    if (ata_read_sector(0, 0, mbr) != 0) {
        printf("%cFailed to read MBR from drive 0\n", 255, 0, 0);
        return;
    }

    uint16_t signature = (mbr[511] << 8) | mbr[510];
    if (signature != 0xAA55) {
        printf("%cWarning: MBR signature is invalid (found %d, expected 43605).\n", 255, 165, 0, signature);
    } else {
        printf("%cMBR signature is valid.\n", 0, 255, 0);
    }

    printf("%cPartition Table (drive 0):\n", 255, 255, 0);
    for (int i = 0; i < 4; i++) {
        uint8* entry = mbr + 0x1BE + i * 16;
        uint8 boot = entry[0];
        uint8 type = entry[4];
        uint32 start_lba = entry[8] | (entry[9]<<8) | (entry[10]<<16) | (entry[11]<<24);
        uint32 size = entry[12] | (entry[13]<<8) | (entry[14]<<16) | (entry[15]<<24);
        printf("%cPart %d: Boot=%d Type=%d Start=%d Size=%d\n", 255,255,255, i+1, boot, type, start_lba, size);
    }
}

void fdisk_create_partition(uint32 start_lba, uint32 size, uint8 type) {
    uint8 mbr[512];
    if (ata_read_sector(0, 0, mbr) != 0) {
        printf("%cFailed to read MBR from drive 0\n", 255, 0, 0);
        return;
    }
    // Find a free partition entry
    int free_idx = -1;
    for (int i = 0; i < 4; i++) {
        uint8* entry = mbr + 0x1BE + i * 16;
        uint8 ptype = entry[4];
        uint32 pstart = entry[8] | (entry[9]<<8) | (entry[10]<<16) | (entry[11]<<24);
        uint32 psize = entry[12] | (entry[13]<<8) | (entry[14]<<16) | (entry[15]<<24);
        if (ptype == 0 && pstart == 0 && psize == 0) {
            free_idx = i;
            break;
        }
    }
    if (free_idx == -1) {
        printf("%cNo free partition entry available (max 4).\n", 255, 0, 0);
        return;
    }
    // Fill the entry
    uint8* entry = mbr + 0x1BE + free_idx * 16;
    entry[0] = 0x00; // Not bootable
    entry[1] = 0; entry[2] = 0; entry[3] = 0; // CHS not used
    entry[4] = type;
    entry[5] = 0; entry[6] = 0; entry[7] = 0; // CHS not used
    entry[8]  = (start_lba & 0xFF);
    entry[9]  = (start_lba >> 8) & 0xFF;
    entry[10] = (start_lba >> 16) & 0xFF;
    entry[11] = (start_lba >> 24) & 0xFF;
    entry[12] = (size & 0xFF);
    entry[13] = (size >> 8) & 0xFF;
    entry[14] = (size >> 16) & 0xFF;
    entry[15] = (size >> 24) & 0xFF;

    // Set MBR signature
    mbr[510] = 0x55;
    mbr[511] = 0xAA;

    // Write MBR back
    if (ata_write_sector(0, 0, mbr) != 0) {
        printf("%cFailed to write MBR to drive 0\n", 255, 0, 0);
        return;
    }
    printf("%cPartition created: entry %d, type=%d, start=%d, size=%d\n", 0,255,0, free_idx+1, type, start_lba, size);
}

// Helper function to format a partition as EYNFS
int eynfs_format_partition(uint8 drive, uint8 part_num) {
    printf("%cStarting EYNFS format for partition %d...\n", 255, 255, 0, part_num);
    
    // Read MBR to get partition info
    uint8 mbr[512];
    if (ata_read_sector(drive, 0, mbr) != 0) {
        printf("%cFailed to read MBR\n", 255, 0, 0);
        return -1;
    }
    
    uint32 start_lba = 0;
    uint32 size = 0;
    
    // Check for MBR signature
    if (mbr[510] == 0x55 && mbr[511] == 0xAA) {
        // Valid MBR exists, try to read partition info
        uint8* entry = mbr + 0x1BE + (part_num - 1) * 16;
        start_lba = entry[8] | (entry[9]<<8) | (entry[10]<<16) | (entry[11]<<24);
        size = entry[12] | (entry[13]<<8) | (entry[14]<<16) | (entry[15]<<24);
        
        if (start_lba == 0 || size == 0) {
            start_lba = 0;
            size = 1024000; // Assume 500MB disk (1024000 sectors)
        }
    } else {
        // No valid MBR, treat as superfloppy
        start_lba = 0;
        size = 1024000; // Assume 500MB disk (1024000 sectors)
    }
    
    printf("%cUsing start_lba=%d, size=%d\n", 255, 255, 0, start_lba, size);
    
    // ERASE THE DISK: Zero out the first 2048 sectors to remove old FAT32 data
    printf("%cErasing disk...\n", 255, 255, 0);
    uint8 zero_sector[EYNFS_BLOCK_SIZE] = {0};
    for (uint32 i = 0; i < 2048; i++) {
        int write_result = ata_write_sector(drive, start_lba + i, zero_sector);
        if (write_result != 0) {
            printf("%cFailed to erase sector %d\n", 255, 0, 0, i);
            return -7;
        }
        // Add a small delay every 10 sectors to prevent overwhelming the disk controller
        if (i % 10 == 0) {
            sleep(1); // Small delay
        }
    }
    
    printf("%cWriting EYNFS structures...\n", 255, 255, 0);
    
    // EYNFS layout: superblock at start_lba + 2048
    uint32 eynfs_superblock_lba = start_lba + 2048;
    uint32 eynfs_bitmap_lba = start_lba + 2049;
    uint32 eynfs_nametable_lba = start_lba + 2050;
    uint32 eynfs_rootdir_lba = start_lba + 2051;
    
    // Write superblock
    eynfs_superblock_t sb = {0};
    sb.magic = EYNFS_MAGIC;
    sb.version = EYNFS_VERSION;
    sb.block_size = EYNFS_BLOCK_SIZE;
    sb.total_blocks = size;
    sb.root_dir_block = eynfs_rootdir_lba;
    sb.free_block_map = eynfs_bitmap_lba;
    sb.name_table_block = eynfs_nametable_lba;
    if (eynfs_write_superblock(drive, eynfs_superblock_lba, &sb) != 0) {
        printf("%cFailed to write superblock\n", 255, 0, 0);
        return -3;
    }
    
    // Write zeroed free block bitmap
    uint8 bitmap[EYNFS_BLOCK_SIZE] = {0};
    // Mark reserved blocks as used (superblock, bitmap, nametable, rootdir)
    for (int i = 0; i < 4; i++) bitmap[i/8] |= (1 << (i%8));
    if (ata_write_sector(drive, eynfs_bitmap_lba, bitmap) != 0) {
        printf("%cFailed to write bitmap\n", 255, 0, 0);
        return -4;
    }
    
    // Write empty name table
    uint8 name_table[EYNFS_BLOCK_SIZE] = {0};
    if (ata_write_sector(drive, eynfs_nametable_lba, name_table) != 0) {
        printf("%cFailed to write name table\n", 255, 0, 0);
        return -5;
    }
    
    // Write empty root directory block
    uint8 root_dir[EYNFS_BLOCK_SIZE] = {0};
    if (ata_write_sector(drive, eynfs_rootdir_lba, root_dir) != 0) {
        printf("%cFailed to write root directory\n", 255, 0, 0);
        return -6;
    }
    
    printf("%cEYNFS format completed successfully\n", 0, 255, 0);
    return 0;
}

void format_cmd_handler(string ch) {
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;

    if (!ch[i]) {
        printf("%cUsage: format <partition_num (1-4)> [filesystem_type]\n", 255, 255, 255);
        printf("%cFilesystem types: fat32, eynfs\n", 255, 255, 255);
        printf("%cExample: format 1 fat32\n", 255, 255, 255);
        printf("%cExample: format 2 eynfs\n", 255, 255, 255);
        return;
    }

    // Parse partition number (first token)
    char part_str[16];
    uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 15) part_str[j++] = ch[i++];
    part_str[j] = '\0';
    
    uint8 part_num = (uint8)str_to_int(part_str);
    if (part_num < 1 || part_num > 4) {
        printf("%cInvalid partition number. Must be 1-4.\n", 255, 0, 0);
        return;
    }

    // Parse filesystem type (second token)
    while (ch[i] && ch[i] == ' ') i++;
    
    int format_eynfs = 0;
    if (ch[i]) {
        char fs_type[16];
        j = 0;
        while (ch[i] && ch[i] != ' ' && j < 15) fs_type[j++] = ch[i++];
        fs_type[j] = '\0';
        if (strEql(fs_type, "eynfs")) {
            format_eynfs = 1;
        } else if (!strEql(fs_type, "fat32")) {
            printf("%cUnknown filesystem type: %s\n", 255, 0, 0, fs_type);
            printf("%cSupported types: fat32, eynfs\n", 255, 255, 255);
            return;
        }
    }

    printf("%cThis will erase all data on partition %d. Are you sure? (y/n): ", 255, 165, 0, part_num);
    
    string confirm = readStr();
    printf("\n");

    if (strEql(confirm, "y") || strEql(confirm, "Y")) {
        printf("%cFormatting partition %d as %s...\n", 255, 255, 255, part_num, format_eynfs ? "EYNFS" : "FAT32");
        
        int res;
        if (format_eynfs) {
            res = eynfs_format_partition(0, part_num);
        } else {
            res = fat32_format_partition(0, part_num);
        }
        
        if (res == 0) {
            printf("%cPartition %d formatted successfully as %s.\n", 0, 255, 0, part_num, format_eynfs ? "EYNFS" : "FAT32");
        } else {
            printf("%cFailed to format partition %d. Error %d\n", 255, 0, 0, part_num, res);
        }
    } else {
        printf("%cFormat cancelled.\n", 255, 255, 255);
    }
}

void fdisk_cmd_handler(string ch) {
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;

    if (!ch[i]) {
        fdisk_list();
        return;
    }

    char arg[32];
    uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 31) arg[j++] = ch[i++];
    arg[j] = '\0';

    if (strEql(arg, "create")) {
        // Parse start_lba, size, type
        while (ch[i] && ch[i] == ' ') i++;
        if (!ch[i]) {
            printf("%cUsage: fdisk create <start_lba> <size> <type>\n", 255,255,255);
            return;
        }
        uint32 start_lba = str_to_uint(ch + i);
        while (ch[i] && ch[i] != ' ') i++; 
        while (ch[i] && ch[i] == ' ') i++;
        if (!ch[i]) {
            printf("%cUsage: fdisk create <start_lba> <size> <type>\n", 255,255,255);
            return;
        }
        uint32 size = str_to_uint(ch + i);
        while (ch[i] && ch[i] != ' ') i++;
        while (ch[i] && ch[i] == ' ') i++;
        if (!ch[i]) {
            printf("%cUsage: fdisk create <start_lba> <size> <type>\n", 255,255,255);
            return;
        }
        uint8 type = 0;
        if (ch[i] == '0' && (ch[i+1] == 'x' || ch[i+1] == 'X')) {
            i += 2;
            while (ch[i]) {
                char c = ch[i];
                if (c >= '0' && c <= '9') type = (type << 4) | (c - '0');
                else if (c >= 'a' && c <= 'f') type = (type << 4) | (c - 'a' + 10);
                else if (c >= 'A' && c <= 'F') type = (type << 4) | (c - 'A' + 10);
                else break;
                i++;
            }
        } else {
            type = (uint8)str_to_uint(ch + i);
        }
        fdisk_create_partition(start_lba, size, type);
    } else {
        printf("%cUnknown fdisk subcommand: %s\n", 255, 0, 0, arg);
    }
}

// Add the missing static helper for EYNFS superblock
static int load_eynfs_superblock(eynfs_superblock_t *sb) {
    return eynfs_read_superblock(g_current_drive, EYNFS_SUPERBLOCK_LBA, sb);
}

// Add the drive_cmd definition if missing
void drive_cmd(string ch) {
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    if (ch[i] >= '0' && ch[i] <= '9') {
        uint32_t drive = 0;
        while (ch[i] >= '0' && ch[i] <= '9') {
            drive = drive * 10 + (ch[i] - '0');
            i++;
        }
        g_current_drive = (uint8_t)drive;
        printf("%cSwitched to drive %d\n", 0, 255, 0, g_current_drive);
    } else {
        printf("%cUsage: drive <n>\n", 255, 255, 255);
    }
}
