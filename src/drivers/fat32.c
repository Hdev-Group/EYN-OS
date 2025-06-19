#include "../../include/fat32.h"
#include "../../include/vga.h"
#include "../../include/multiboot.h"
#include "../../include/util.h"

extern multiboot_info_t *g_mbi;

// Read BPB from disk image
int fat32_read_bpb(void* disk_img, struct fat32_bpb* bpb) {
    if (!disk_img || !bpb) return -1;
    // BPB is at offset 0
    memory_copy((char*)disk_img, (char*)bpb, sizeof(struct fat32_bpb));
    return 0;
}

// List root directory entries (simple, no subdirs, no LFN)
int fat32_list_root(void* disk_img, struct fat32_bpb* bpb) {
    if (!disk_img || !bpb) return -1;
    // Calculate root directory start
    uint32 byts_per_sec = bpb->BytsPerSec;
    uint32 sec_per_clus = bpb->SecPerClus;
    uint32 rsvd_sec_cnt = bpb->RsvdSecCnt;
    uint32 num_fats = bpb->NumFATs;
    uint32 fatsz = bpb->FATSz32;
    uint32 root_clus = bpb->RootClus;

    // First data sector
    uint32 first_data_sec = rsvd_sec_cnt + (num_fats * fatsz);
    // Root dir sector
    uint32 root_dir_sec = first_data_sec + ((root_clus - 2) * sec_per_clus);
    uint32 root_dir_offset = root_dir_sec * byts_per_sec;

    // List first 16 entries
    struct fat32_dir_entry* entries = (struct fat32_dir_entry*)((char*)disk_img + root_dir_offset);
    for (int i = 0; i < 16; i++) {
        if (entries[i].Name[0] == 0x00) break; // No more entries
        if ((entries[i].Attr & 0x0F) == 0x0F) continue; // Skip LFN
        if (entries[i].Name[0] == 0xE5) continue; // Deleted
        // Print name
        char name[12];
        for (int j = 0; j < 11; j++) name[j] = entries[i].Name[j];
        name[11] = '\0';
        if (entries[i].Attr & 0x10) { // Directory
            printf("%c%s <DIR>\n", 255, 255, 255, name);
        } else {
            printf("%c%s\n", 255, 255, 255, name);
        }
    }
    return 0;
}

int fat32_read_file(void* disk_img, struct fat32_bpb* bpb, const char* filename, char* buf, int bufsize) {
    if (!disk_img || !bpb || !filename || !buf) return -1;
    uint32 byts_per_sec = bpb->BytsPerSec;
    uint32 sec_per_clus = bpb->SecPerClus;
    uint32 rsvd_sec_cnt = bpb->RsvdSecCnt;
    uint32 num_fats = bpb->NumFATs;
    uint32 fatsz = bpb->FATSz32;
    uint32 root_clus = bpb->RootClus;

    // First data sector
    uint32 first_data_sec = rsvd_sec_cnt + (num_fats * fatsz);
    // Root dir sector
    uint32 root_dir_sec = first_data_sec + ((root_clus - 2) * sec_per_clus);
    uint32 root_dir_offset = root_dir_sec * byts_per_sec;

    // Find file in first 16 entries
    struct fat32_dir_entry* entries = (struct fat32_dir_entry*)((char*)disk_img + root_dir_offset);
    for (int i = 0; i < 16; i++) {
        if (entries[i].Name[0] == 0x00) break; // No more entries
        if ((entries[i].Attr & 0x0F) == 0x0F) continue; // Skip LFN
        if (entries[i].Name[0] == 0xE5) continue; // Deleted
        // Compare 8.3 name
        int match = 1;
        for (int j = 0; j < 11; j++) {
            if (entries[i].Name[j] != filename[j]) {
                match = 0;
                break;
            }
        }
        if (match) {
            // Found file
            uint32 file_size = entries[i].FileSize;
            if (file_size > (uint32)bufsize) file_size = bufsize;
            uint32 first_clus = ((uint32)entries[i].FstClusHI << 16) | entries[i].FstClusLO;
            if (first_clus < 2) return -1;
            // Read file data (no FAT walking, just first cluster)
            uint32 data_sec = first_data_sec + ((first_clus - 2) * sec_per_clus);
            uint32 data_offset = data_sec * byts_per_sec;
            memory_copy((char*)disk_img + data_offset, buf, file_size);
            return file_size;
        }
    }
    return -1; // Not found
} 