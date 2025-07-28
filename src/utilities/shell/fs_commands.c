#include "../../../include/fs_commands.h"
#include "../../../include/shell_command_info.h"
#include "../../../include/types.h"
#include "../../../include/vga.h"
#include "../../../include/util.h"
#include "../../../include/fat32.h"
#include "../../../include/eynfs.h"
#include "../../../include/string.h"
#include "../../../include/write_editor.h"
#include "../../../include/kb.h"
#include "../../../include/system.h"
#include <stdint.h>

// EYNFS integration: assume superblock at LBA 2048 on drive 0
#define EYNFS_SUPERBLOCK_LBA 2048
#define EYNFS_DRIVE 0

// Global variable for current drive (default 0)
extern uint8_t g_current_drive;

// Helper function prototypes (if not already declared)
void to_fat32_83(const char* input, char* output);
int parse_redirection(const char* input, char* cmd, char* filename);
uint32 str_to_uint(const char* s);

extern void* fat32_disk_img;
extern void poll_keyboard_for_ctrl_c();

#define MAX_ENTRIES (EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t))

extern char shell_current_path[128]; // from shell.c

// Helper: resolve relative/absolute path to absolute
void resolve_path(const char* input, const char* cwd, char* out, size_t outsz) {
    if (!input || !input[0]) {
        strncpy(out, cwd, outsz-1); out[outsz-1] = '\0';
        return;
    }
    if (input[0] == '/') {
        strncpy(out, input, outsz-1); out[outsz-1] = '\0';
        return;
    }
    // Join cwd and input
    char tmp[256];
    size_t cwdlen = strlen(cwd);
    if (cwdlen > 1 && cwd[cwdlen-1] == '/') cwdlen--;
    strncpy(tmp, cwd, cwdlen); tmp[cwdlen] = '\0';
    if (cwdlen > 0 && tmp[cwdlen-1] != '/') strncat(tmp, "/", sizeof(tmp)-strlen(tmp)-1);
    strncat(tmp, input, sizeof(tmp)-strlen(tmp)-1);
    // Normalize: handle ., .., //
    char* parts[64]; int nparts = 0;
    char* tok; char* save;
    for (tok = strtok_r(tmp, "/", &save); tok; tok = strtok_r(NULL, "/", &save)) {
        if (strcmp(tok, ".") == 0) continue;
        if (strcmp(tok, "..") == 0) { if (nparts > 0) nparts--; continue; }
        parts[nparts++] = tok;
    }
    out[0] = '/'; out[1] = '\0';
    for (int i = 0; i < nparts; ++i) {
        strncat(out, parts[i], outsz-strlen(out)-1);
        if (i < nparts-1) strncat(out, "/", outsz-strlen(out)-1);
    }
    // Do NOT add a trailing slash
}

// Helper: get last path component (filename) from a path
static const char* get_basename(const char* path) {
    const char* last = path;
    for (const char* p = path; *p; ++p) {
        if (*p == '/') last = p + 1;
    }
    return last;
}

// cd command
void cd(string input) {
    uint8 disk = g_current_drive;
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(disk, EYNFS_SUPERBLOCK_LBA, &sb) != 0 || sb.magic != EYNFS_MAGIC) {
        printf("%cNo supported filesystem found.\n", 255, 0, 0);
        return;
    }
    uint8 i = 0;
    while (input[i] && input[i] != ' ') i++;
    while (input[i] && input[i] == ' ') i++;
    if (!input[i]) {
        printf("%cUsage: cd <directory>\n", 255, 255, 255);
        return;
    }
    char arg[128]; uint8 j = 0;
    while (input[i] && input[i] != ' ' && j < 127) arg[j++] = input[i++];
    arg[j] = '\0';
    char abspath[128];
    resolve_path(arg, shell_current_path, abspath, sizeof(abspath));
    eynfs_dir_entry_t entry;
    if (strcmp(abspath, "/") == 0) {
        strncpy(shell_current_path, "/", sizeof(shell_current_path)-1);
        shell_current_path[sizeof(shell_current_path)-1] = '\0';
        return;
    }
    if (eynfs_traverse_path(disk, &sb, abspath, &entry, NULL, NULL) == 0 && entry.type == EYNFS_TYPE_DIR) {
        strncpy(shell_current_path, abspath, sizeof(shell_current_path)-1);
        shell_current_path[sizeof(shell_current_path)-1] = '\0';
    } else {
        printf("%cDirectory not found: %s\n", 255, 0, 0, abspath);
    }
}

// Helper: recursive ls with depth, indentation, and color
void eynfs_ls_depth(uint8 disk, uint32_t dir_block, int depth, int max_depth, int indent) {
    eynfs_dir_entry_t entries[MAX_ENTRIES];
    int count = eynfs_read_dir_table(disk, dir_block, entries, MAX_ENTRIES);
    for (int i = 0; i < count; ++i) {
        if (entries[i].name[0] == '\0') continue;
        // Print indentation in blue
        for (int d = 0; d < indent; ++d);
        if (indent > 0) printf("%c || ", 120, 120, 255);
        if (entries[i].type == EYNFS_TYPE_DIR) {
            printf("%c%s/\n", 120, 120, 255, entries[i].name);
            if (depth < max_depth) {
                eynfs_ls_depth(disk, entries[i].first_block, depth+1, max_depth, indent+1);
            }
        } else {
            printf("%c%s\n", 255, 255, 255, entries[i].name);
        }
    }
}

// Refactor ls to detect filesystem and dispatch
void ls(string input) {
    uint8 disk = g_current_drive;
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(disk, EYNFS_SUPERBLOCK_LBA, &sb) == 0 && sb.magic == EYNFS_MAGIC) {
        // EYNFS implementation (already present)
        int max_depth = 0;
        uint8 i = 0;
        while (input[i] && input[i] != ' ') i++;
        while (input[i] && input[i] == ' ') i++;
        if (input[i]) {
            max_depth = str_to_uint(&input[i]);
            if (max_depth > 10) max_depth = 10;
        }
        char abspath[128];
        resolve_path("", shell_current_path, abspath, sizeof(abspath));
        eynfs_dir_entry_t entry;
        uint32_t dir_block = sb.root_dir_block;
        if (strcmp(abspath, "/") != 0) {
            if (eynfs_traverse_path(disk, &sb, abspath, &entry, NULL, NULL) == 0 && entry.type == EYNFS_TYPE_DIR) {
                dir_block = entry.first_block;
            } else {
                printf("%cDirectory not found: %s\n", 255, 0, 0, abspath);
                return;
            }
        }
        eynfs_ls_depth(disk, dir_block, 0, max_depth, 0);
        return;
    }
    // FAT32 fallback
    uint32 partition_lba_start = fat32_get_partition_lba_start(disk);
    struct fat32_bpb bpb;
    if (fat32_read_bpb_sector(disk, partition_lba_start, &bpb) == 0) {
        int max_depth = 1;
        uint8 i = 0;
        while (input[i] && input[i] != ' ') i++;
        while (input[i] && input[i] == ' ') i++;
        if (input[i]) {
            int val = 0;
            while (input[i] >= '0' && input[i] <= '9') {
                val = val * 10 + (input[i] - '0');
                i++;
            }
            if (val > 0) max_depth = val;
        }
        printf("%cFAT32 directory tree (depth: %d):\n\n", 255, 255, 255, max_depth);
        uint32 byts_per_sec = bpb.BytsPerSec;
        uint32 sec_per_clus = bpb.SecPerClus;
        uint32 rsvd_sec_cnt = bpb.RsvdSecCnt;
        uint32 num_fats = bpb.NumFATs;
        uint32 fatsz = bpb.FATSz32;
        uint32 root_clus = bpb.RootClus;
        uint32 first_data_sec = rsvd_sec_cnt + (num_fats * fatsz);
        uint32 cluster = root_clus;
        extern volatile int g_user_interrupt;
        g_user_interrupt = 0;
        int depth = 0;
        // Only support depth 1 for now (can be extended)
        while (cluster < 0x0FFFFFF8) {
            uint32 cluster_first_sec = first_data_sec + ((cluster - 2) * sec_per_clus);
            for (uint32 sec = 0; sec < sec_per_clus; sec++) {
                struct fat32_dir_entry* entries = (struct fat32_dir_entry*)((char*)fat32_disk_img + (cluster_first_sec + sec) * byts_per_sec);
                int entry_count = byts_per_sec / sizeof(struct fat32_dir_entry);
                for (int i = 0; i < entry_count; i++) {
                    if (entries[i].Name[0] == 0x00) break;
                    if ((entries[i].Attr & 0x0F) == 0x0F) continue;
                    if (entries[i].Name[0] == 0xE5) continue;
                    char name[12];
                    for (int j = 0; j < 11; j++) name[j] = entries[i].Name[j];
                    name[11] = '\0';
                    if (entries[i].Attr & 0x10) {
                        printf("%c%s/\n", 120, 120, 255, name);
                    } else {
                        printf("%c%s\n", 255, 255, 255, name);
                    }
                    poll_keyboard_for_ctrl_c();
                    sleep(30);
                    if (g_user_interrupt) {
                        printf("\n^C [Interrupted by user]\n", 255, 0, 0);
                        g_user_interrupt = 0;
                        return;
                    }
                }
            }
            cluster = fat32_next_cluster(fat32_disk_img, &bpb, cluster);
        }
        return;
    }
    printf("%cNo supported filesystem found on drive %d.\n", 255, 0, 0, disk);
}

// cat implementation
void cat(string ch) {
    uint8 disk = g_current_drive;
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    if (!ch[i]) {
        printf("%cUsage: read <filename>\n", 255, 255, 255);
        return;
    }
    char arg[128]; uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 127) arg[j++] = ch[i++];
    arg[j] = '\0';
    char abspath[128];
    resolve_path(arg, shell_current_path, abspath, sizeof(abspath));
    // Try EYNFS first
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(disk, EYNFS_SUPERBLOCK_LBA, &sb) == 0 && sb.magic == EYNFS_MAGIC) {
        eynfs_dir_entry_t entry;
        if (eynfs_traverse_path(disk, &sb, abspath, &entry, NULL, NULL) != 0 || entry.type != EYNFS_TYPE_FILE) {
            printf("%cFile not found: %s\n", 255, 0, 0, abspath);
            return;
        }
        const int chunk_size = 8;
        uint32_t bytes_left = entry.size;
        uint32_t offset = 0;
        char buf[chunk_size + 1];
        while (bytes_left > 0) {
            int to_read = (bytes_left < chunk_size) ? bytes_left : chunk_size;
            int n = eynfs_read_file(disk, &sb, &entry, buf, to_read, offset);
            if (n < 0) {
                printf("%cFailed to read file.\n", 255, 0, 0);
                return;
            }
            buf[n] = '\0';
            printf("%c%s", 255, 255, 255, buf);
            offset += n;
            bytes_left -= n;
            if (n == 0) break;
        }
        printf("\n");
        return;
    }
    // FAT32 fallback
    uint32 partition_lba_start = fat32_get_partition_lba_start(disk);
    struct fat32_bpb bpb;
    if (fat32_read_bpb_sector(disk, partition_lba_start, &bpb) == 0) {
        char fatname[12];
        to_fat32_83(arg, fatname);
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
        extern volatile int g_user_interrupt;
        g_user_interrupt = 0;
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
                        sleep(30);
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
        return;
    }
    printf("%cNo supported filesystem found.\n", 255, 0, 0);
}

// del implementation
void del(string ch) {
    uint8 disk = g_current_drive;
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    if (!ch[i]) {
        printf("%cUsage: del <filename>\n", 255, 255, 255);
        printf("%cDeletes the specified file from the filesystem.\n", 255, 255, 255);
        return;
    }
    char arg[128]; uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 127) arg[j++] = ch[i++];
    arg[j] = '\0';
    char abspath[128];
    resolve_path(arg, shell_current_path, abspath, sizeof(abspath));
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(disk, EYNFS_SUPERBLOCK_LBA, &sb) == 0 && sb.magic == EYNFS_MAGIC) {
        eynfs_dir_entry_t entry;
        uint32_t parent_block, entry_idx;
        if (eynfs_traverse_path(disk, &sb, abspath, &entry, &parent_block, &entry_idx) == 0 && entry.type == EYNFS_TYPE_FILE) {
            if (eynfs_delete_entry(disk, &sb, parent_block, entry.name) == 0) {
                printf("%cFile '%s' deleted successfully.\n", 0, 255, 0, abspath);
            } else {
                printf("%cFailed to delete file '%s'.\n", 255, 0, 0, abspath);
            }
        } else {
            printf("%cFile not found: %s\n", 255, 0, 0, abspath);
        }
        return;
    }
    printf("%cNo supported filesystem found.\n", 255, 0, 0);
}

// write implementation
void write_cmd(string ch) {
    uint8 disk = g_current_drive;
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    if (!ch[i]) {
        printf("%cUsage: write <filename>\n", 255, 255, 255);
        printf("%cOpens a text editor for the specified file.\n", 255, 255, 255);
        return;
    }
    char arg[128]; uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 127) arg[j++] = ch[i++];
    arg[j] = '\0';
    char abspath[128];
    resolve_path(arg, shell_current_path, abspath, sizeof(abspath));
    // Pass the full path to write_editor for subdirectory support
    write_editor(abspath, disk);
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

// writefat implementation
void writefat(string ch)
{
    uint32 partition_lba_start = fat32_get_partition_lba_start(0);
    struct fat32_bpb bpb;
    if (fat32_read_bpb_sector(0, partition_lba_start, &bpb) != 0) {
        printf("%cFailed to read FAT32 BPB from drive 0\n", 255, 0, 0);
        return;
    }
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

// lsram implementation
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
    int max_depth = 1;
    uint8 i = 0;
    while (input[i] && input[i] != ' ') i++;
    while (input[i] && input[i] == ' ') i++;
    if (input[i]) {
        int val = 0;
        while (input[i] >= '0' && input[i] <= '9') 
        {
            val = val * 10 + (input[i] - '0');
            i++;
        }
        if (val > 0) max_depth = val;
    }
    printf("%cFAT32 directory tree (depth: %d):\n\n", 255, 255, 255, max_depth);
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
                sleep(30);
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

// writeram implementation
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

int write_output_to_file(const char* buf, int len, const char* filename, uint8_t disk) {
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(disk, EYNFS_SUPERBLOCK_LBA, &sb) != 0 || sb.magic != EYNFS_MAGIC) {
        printf("No supported filesystem found.\n");
        return -1;
    }
    eynfs_dir_entry_t entry;
    uint32_t entry_idx;
    // Try to find the file, or create it if it doesn't exist
    if (eynfs_find_in_dir(disk, &sb, sb.root_dir_block, filename, &entry, &entry_idx) != 0) {
        if (eynfs_create_entry(disk, &sb, sb.root_dir_block, filename, EYNFS_TYPE_FILE) != 0) {
            printf("Failed to create file: %s\n", filename);
            return -1;
        }
        if (eynfs_find_in_dir(disk, &sb, sb.root_dir_block, filename, &entry, &entry_idx) != 0) {
            printf("Failed to find file after creation: %s\n", filename);
            return -1;
        }
    }
    int written = eynfs_write_file(disk, &sb, &entry, buf, len, sb.root_dir_block, entry_idx);
    return (written == len) ? 0 : -1;
}

// makedir implementation
void makedir(string ch) {
    uint8 disk = g_current_drive;
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    if (!ch[i]) {
        printf("%cUsage: makedir <directory>\n", 255, 255, 255);
        printf("%cCreates a new directory at the specified path.\n", 255, 255, 255);
        return;
    }
    char arg[128]; uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 127) arg[j++] = ch[i++];
    arg[j] = '\0';
    char abspath[128];
    resolve_path(arg, shell_current_path, abspath, sizeof(abspath));
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(disk, EYNFS_SUPERBLOCK_LBA, &sb) != 0 || sb.magic != EYNFS_MAGIC) {
        printf("%cNo supported filesystem found.\n", 255, 0, 0);
        return;
    }
    // Find parent directory and base name
    char parent_path[128];
    strcpy(parent_path, abspath);
    char* last_slash = strrchr(parent_path, '/');
    if (!last_slash || last_slash == parent_path) {
        strcpy(parent_path, "/");
    } else {
        *last_slash = '\0';
    }
    eynfs_dir_entry_t parent_entry;
    if (eynfs_traverse_path(disk, &sb, parent_path, &parent_entry, NULL, NULL) != 0 || parent_entry.type != EYNFS_TYPE_DIR) {
        printf("%cParent directory not found: %s\n", 255, 0, 0, parent_path);
        return;
    }
    const char* dirname = get_basename(abspath);
    if (eynfs_create_entry(disk, &sb, parent_entry.first_block, dirname, EYNFS_TYPE_DIR) == 0) {
        printf("%cDirectory '%s' created successfully.\n", 0, 255, 0, abspath);
    } else {
        printf("%cFailed to create directory '%s'.\n", 255, 0, 0, abspath);
    }
}

// deldir implementation
void deldir(string ch) {
    uint8 disk = g_current_drive;
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    if (!ch[i]) {
        printf("%cUsage: deldir <directory>\n", 255, 255, 255);
        printf("%cRemoves the specified directory (must be empty).\n", 255, 255, 255);
        return;
    }
    char arg[128]; uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 127) arg[j++] = ch[i++];
    arg[j] = '\0';
    char abspath[128];
    resolve_path(arg, shell_current_path, abspath, sizeof(abspath));
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(disk, EYNFS_SUPERBLOCK_LBA, &sb) != 0 || sb.magic != EYNFS_MAGIC) {
        printf("%cNo supported filesystem found.\n", 255, 0, 0);
        return;
    }
    eynfs_dir_entry_t entry;
    uint32_t parent_block, entry_idx;
    if (eynfs_traverse_path(disk, &sb, abspath, &entry, &parent_block, &entry_idx) != 0 || entry.type != EYNFS_TYPE_DIR) {
        printf("%cDirectory not found: %s\n", 255, 0, 0, abspath);
        return;
    }
    // Check if directory is empty
    eynfs_dir_entry_t entries[MAX_ENTRIES];
    int count = eynfs_read_dir_table(disk, entry.first_block, entries, MAX_ENTRIES);
    int empty = 1;
    for (int k = 0; k < count; ++k) {
        if (entries[k].name[0] != '\0') { empty = 0; break; }
    }
    if (!empty) {
        printf("%cDirectory not empty: %s\n", 255, 0, 0, abspath);
        return;
    }
    if (eynfs_delete_entry(disk, &sb, parent_block, entry.name) == 0) {
        printf("%cDirectory '%s' deleted successfully.\n", 0, 255, 0, abspath);
    } else {
        printf("%cFailed to delete directory '%s'.\n", 255, 0, 0, abspath);
    }
}

// Copy command implementation
void copy_cmd(string ch) {
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    
    if (!ch[i]) {
        printf("%cUsage: copy <source> <destination>\n", 255, 255, 255);
        printf("%cExample: copy file1.txt file2.txt\n", 255, 255, 255);
        return;
    }
    
    // Parse source filename
    char source[128] = {0};
    uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 127) {
        source[j++] = ch[i++];
    }
    source[j] = '\0';
    
    while (ch[i] && ch[i] == ' ') i++;
    
    if (!ch[i]) {
        printf("%cError: Destination filename required.\n", 255, 0, 0);
        return;
    }
    
    // Parse destination filename
    char dest[128] = {0};
    j = 0;
    while (ch[i] && ch[i] != ' ' && j < 127) {
        dest[j++] = ch[i++];
    }
    dest[j] = '\0';
    
    // Resolve paths
    char source_path[256], dest_path[256];
    resolve_path(source, shell_current_path, source_path, sizeof(source_path));
    resolve_path(dest, shell_current_path, dest_path, sizeof(dest_path));
    
    // Read source file
    eynfs_superblock_t sb;
    uint8_t disk = g_current_drive;
    if (eynfs_read_superblock(disk, EYNFS_SUPERBLOCK_LBA, &sb) != 0 || sb.magic != EYNFS_MAGIC) {
        printf("%cError: No supported filesystem found.\n", 255, 0, 0);
        return;
    }
    
    eynfs_dir_entry_t source_entry;
    uint32_t parent_block, entry_idx;
    if (eynfs_traverse_path(disk, &sb, source_path, &source_entry, &parent_block, &entry_idx) != 0) {
        printf("%cError: Source file not found: %s\n", 255, 0, 0, source_path);
        return;
    }
    
    if (source_entry.type != EYNFS_TYPE_FILE) {
        printf("%cError: Source is not a file: %s\n", 255, 0, 0, source_path);
        return;
    }
    
    // Read file content
    uint8_t* file_content = (uint8_t*)my_malloc(source_entry.size + 1);
    if (!file_content) {
        printf("%cError: Memory allocation failed.\n", 255, 0, 0);
        return;
    }
    
    int bytes_read = eynfs_read_file(disk, &sb, &source_entry, file_content, source_entry.size, 0);
    if (bytes_read <= 0) {
        printf("%cError: Failed to read source file.\n", 255, 0, 0);
        my_free(file_content);
        return;
    }
    
    // Create destination file
    eynfs_dir_entry_t dest_entry = source_entry;
    strncpy(dest_entry.name, dest, EYNFS_NAME_MAX - 1);
    dest_entry.name[EYNFS_NAME_MAX - 1] = '\0';
    
    // Find parent directory for destination
    char dest_dir[256];
    char* last_slash = strrchr(dest_path, '/');
    if (last_slash) {
        strncpy(dest_dir, dest_path, last_slash - dest_path);
        dest_dir[last_slash - dest_path] = '\0';
        strcpy(dest_entry.name, last_slash + 1);
    } else {
        strcpy(dest_dir, "/");
    }
    
    uint32_t dest_parent_block, dest_entry_idx;
    eynfs_dir_entry_t dest_parent;
    if (eynfs_traverse_path(disk, &sb, dest_dir, &dest_parent, &dest_parent_block, &dest_entry_idx) != 0) {
        printf("%cError: Destination directory not found: %s\n", 255, 0, 0, dest_dir);
        my_free(file_content);
        return;
    }
    
    // Create the destination file
    if (eynfs_create_entry(disk, &sb, dest_parent_block, dest_entry.name, EYNFS_TYPE_FILE) != 0) {
        printf("%cError: Failed to create destination file.\n", 255, 0, 0);
        my_free(file_content);
        return;
    }
    
    // Find the created entry and write content
    if (eynfs_find_in_dir(disk, &sb, dest_parent_block, dest_entry.name, &dest_entry, &dest_entry_idx) != 0) {
        printf("%cError: Failed to locate created file.\n", 255, 0, 0);
        my_free(file_content);
        return;
    }
    
    if (eynfs_write_file(disk, &sb, &dest_entry, file_content, bytes_read, dest_parent_block, dest_entry_idx) != bytes_read) {
        printf("%cError: Failed to write destination file.\n", 255, 0, 0);
        my_free(file_content);
        return;
    }
    
    printf("%cFile copied: %s -> %s (%d bytes)\n", 0, 255, 0, source_path, dest_path, bytes_read);
    my_free(file_content);
}

// Move command implementation
void move_cmd(string ch) {
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    
    if (!ch[i]) {
        printf("%cUsage: move <source> <destination>\n", 255, 255, 255);
        printf("%cExample: move file1.txt /backup/file1.txt\n", 255, 255, 255);
        return;
    }
    
    // Parse source filename
    char source[128] = {0};
    uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 127) {
        source[j++] = ch[i++];
    }
    source[j] = '\0';
    
    while (ch[i] && ch[i] == ' ') i++;
    
    if (!ch[i]) {
        printf("%cError: Destination filename required.\n", 255, 0, 0);
        return;
    }
    
    // Parse destination filename
    char dest[128] = {0};
    j = 0;
    while (ch[i] && ch[i] != ' ' && j < 127) {
        dest[j++] = ch[i++];
    }
    dest[j] = '\0';
    
    // Resolve paths
    char source_path[256], dest_path[256];
    resolve_path(source, shell_current_path, source_path, sizeof(source_path));
    resolve_path(dest, shell_current_path, dest_path, sizeof(dest_path));
    
    // First copy the file
    char temp_cmd[512];
    snprintf(temp_cmd, sizeof(temp_cmd), "copy %s %s", source_path, dest_path);
    
    // Parse the copy command manually
    uint8 k = 0;
    while (temp_cmd[k] && temp_cmd[k] != ' ') k++;
    while (temp_cmd[k] && temp_cmd[k] == ' ') k++;
    
    char source_file[128] = {0};
    j = 0;
    while (temp_cmd[k] && temp_cmd[k] != ' ' && j < 127) {
        source_file[j++] = temp_cmd[k++];
    }
    source_file[j] = '\0';
    
    while (temp_cmd[k] && temp_cmd[k] == ' ') k++;
    
    char dest_file[128] = {0};
    j = 0;
    while (temp_cmd[k] && temp_cmd[k] != ' ' && j < 127) {
        dest_file[j++] = temp_cmd[k++];
    }
    dest_file[j] = '\0';
    
    // Resolve paths for copy
    char source_path_copy[256], dest_path_copy[256];
    resolve_path(source_file, shell_current_path, source_path_copy, sizeof(source_path_copy));
    resolve_path(dest_file, shell_current_path, dest_path_copy, sizeof(dest_path_copy));
    
    // Read source file
    eynfs_superblock_t sb;
    uint8_t disk = g_current_drive;
    if (eynfs_read_superblock(disk, EYNFS_SUPERBLOCK_LBA, &sb) != 0 || sb.magic != EYNFS_MAGIC) {
        printf("%cError: No supported filesystem found.\n", 255, 0, 0);
        return;
    }
    
    eynfs_dir_entry_t source_entry;
    uint32_t parent_block, entry_idx;
    if (eynfs_traverse_path(disk, &sb, source_path_copy, &source_entry, &parent_block, &entry_idx) != 0) {
        printf("%cError: Source file not found: %s\n", 255, 0, 0, source_path_copy);
        return;
    }
    
    if (source_entry.type != EYNFS_TYPE_FILE) {
        printf("%cError: Source is not a file: %s\n", 255, 0, 0, source_path_copy);
        return;
    }
    
    // Read file content
    uint8_t* file_content = (uint8_t*)my_malloc(source_entry.size + 1);
    if (!file_content) {
        printf("%cError: Memory allocation failed.\n", 255, 0, 0);
        return;
    }
    
    int bytes_read = eynfs_read_file(disk, &sb, &source_entry, file_content, source_entry.size, 0);
    if (bytes_read <= 0) {
        printf("%cError: Failed to read source file.\n", 255, 0, 0);
        my_free(file_content);
        return;
    }
    
    // Create destination file
    eynfs_dir_entry_t dest_entry = source_entry;
    strncpy(dest_entry.name, dest_file, EYNFS_NAME_MAX - 1);
    dest_entry.name[EYNFS_NAME_MAX - 1] = '\0';
    
    // Find parent directory for destination
    char dest_dir[256];
    char* last_slash = strrchr(dest_path_copy, '/');
    if (last_slash) {
        strncpy(dest_dir, dest_path_copy, last_slash - dest_path_copy);
        dest_dir[last_slash - dest_path_copy] = '\0';
        strcpy(dest_entry.name, last_slash + 1);
    } else {
        strcpy(dest_dir, "/");
    }
    
    uint32_t dest_parent_block, dest_entry_idx;
    eynfs_dir_entry_t dest_parent;
    if (eynfs_traverse_path(disk, &sb, dest_dir, &dest_parent, &dest_parent_block, &dest_entry_idx) != 0) {
        printf("%cError: Destination directory not found: %s\n", 255, 0, 0, dest_dir);
        my_free(file_content);
        return;
    }
    
    // Create the destination file
    if (eynfs_create_entry(disk, &sb, dest_parent_block, dest_entry.name, EYNFS_TYPE_FILE) != 0) {
        printf("%cError: Failed to create destination file.\n", 255, 0, 0);
        my_free(file_content);
        return;
    }
    
    // Find the created entry and write content
    if (eynfs_find_in_dir(disk, &sb, dest_parent_block, dest_entry.name, &dest_entry, &dest_entry_idx) != 0) {
        printf("%cError: Failed to locate created file.\n", 255, 0, 0);
        my_free(file_content);
        return;
    }
    
    if (eynfs_write_file(disk, &sb, &dest_entry, file_content, bytes_read, dest_parent_block, dest_entry_idx) != bytes_read) {
        printf("%cError: Failed to write destination file.\n", 255, 0, 0);
        my_free(file_content);
        return;
    }
    
    // Now delete the source file
    if (eynfs_delete_entry(disk, &sb, parent_block, source_entry.name) != 0) {
        printf("%cWarning: File copied but failed to delete source: %s\n", 255, 255, 0, source_path_copy);
    } else {
        printf("%cFile moved: %s -> %s (%d bytes)\n", 0, 255, 0, source_path_copy, dest_path_copy, bytes_read);
    }
    
    my_free(file_content);
}

REGISTER_SHELL_COMMAND(ls, "ls", "List files in the root directory of the selected drive.\nUsage: ls", "ls");
REGISTER_SHELL_COMMAND(read, "read", "Display contents of a file.\nUsage: read <filename>", "read myfile.txt");
REGISTER_SHELL_COMMAND(del, "del", "Delete a file from the filesystem.\nUsage: del <filename>", "del myfile.txt");
REGISTER_SHELL_COMMAND(write, "write", "Open nano-like text editor for a file.\nUsage: write <filename>", "write myfile.txt");
REGISTER_SHELL_COMMAND(size, "size", "Show the size of a file in bytes.\nUsage: size <filename>", "size myfile.txt");
REGISTER_SHELL_COMMAND(cd, "cd", "Change the current directory.\nUsage: cd <directory>", "cd myfolder");
REGISTER_SHELL_COMMAND(makedir, "makedir", "Create a new directory.\nUsage: makedir <directory>", "makedir myfolder");
REGISTER_SHELL_COMMAND(deldir, "deldir", "Delete an empty directory.\nUsage: deldir <directory>", "deldir myfolder");