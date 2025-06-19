#include "../../include/fat32.h"
#include "../../include/vga.h"
#include "../../include/multiboot.h"
#include "../../include/util.h"
#include "../../include/system.h"

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

// Helper: get next cluster in chain
uint32 fat32_next_cluster(void* disk_img, struct fat32_bpb* bpb, uint32 cluster) {
    // FAT starts after reserved sectors
    uint32 fat_offset = bpb->RsvdSecCnt * bpb->BytsPerSec;
    uint32* fat = (uint32*)((char*)disk_img + fat_offset);
    return fat[cluster] & 0x0FFFFFFF; // 28 bits for FAT32
}

// List all entries in a directory given its starting cluster
int fat32_list_dir(void* disk_img, struct fat32_bpb* bpb, uint32 start_cluster) {
    if (!disk_img || !bpb || start_cluster < 2) return -1;
    uint32 byts_per_sec = bpb->BytsPerSec;
    uint32 sec_per_clus = bpb->SecPerClus;
    uint32 rsvd_sec_cnt = bpb->RsvdSecCnt;
    uint32 num_fats = bpb->NumFATs;
    uint32 fatsz = bpb->FATSz32;
    uint32 first_data_sec = rsvd_sec_cnt + (num_fats * fatsz);

    uint32 cluster = start_cluster;
    while (cluster < 0x0FFFFFF8) { // FAT32 end-of-chain
        uint32 dir_sec = first_data_sec + ((cluster - 2) * sec_per_clus);
        uint32 dir_offset = dir_sec * byts_per_sec;
        struct fat32_dir_entry* entries = (struct fat32_dir_entry*)((char*)disk_img + dir_offset);
        for (int i = 0; i < (byts_per_sec * sec_per_clus) / sizeof(struct fat32_dir_entry); i++) {
            if (entries[i].Name[0] == 0x00) break; // No more entries
            if ((entries[i].Attr & 0x0F) == 0x0F) continue; // Skip LFN
            if (entries[i].Name[0] == 0xE5) continue; // Deleted
            char name[12];
            for (int j = 0; j < 11; j++) name[j] = entries[i].Name[j];
            name[11] = '\0';
            if (entries[i].Attr & 0x10) {
                printf("%c%s <DIR>\n", 255, 255, 255, name);
            } else {
                printf("%c%s\n", 255, 255, 255, name);
            }
        }
        cluster = fat32_next_cluster(disk_img, bpb, cluster);
    }
    return 0;
}

// Find a directory entry by name in a directory, returns cluster or -1 if not found
int fat32_find_entry(void* disk_img, struct fat32_bpb* bpb, uint32 dir_cluster, const char* name, struct fat32_dir_entry* out_entry) {
    if (!disk_img || !bpb || dir_cluster < 2 || !name) return -1;
    uint32 byts_per_sec = bpb->BytsPerSec;
    uint32 sec_per_clus = bpb->SecPerClus;
    uint32 rsvd_sec_cnt = bpb->RsvdSecCnt;
    uint32 num_fats = bpb->NumFATs;
    uint32 fatsz = bpb->FATSz32;
    uint32 first_data_sec = rsvd_sec_cnt + (num_fats * fatsz);

    uint32 cluster = dir_cluster;
    while (cluster < 0x0FFFFFF8) {
        uint32 dir_sec = first_data_sec + ((cluster - 2) * sec_per_clus);
        uint32 dir_offset = dir_sec * byts_per_sec;
        struct fat32_dir_entry* entries = (struct fat32_dir_entry*)((char*)disk_img + dir_offset);
        for (int i = 0; i < (byts_per_sec * sec_per_clus) / sizeof(struct fat32_dir_entry); i++) {
            if (entries[i].Name[0] == 0x00) break;
            if ((entries[i].Attr & 0x0F) == 0x0F) continue;
            if (entries[i].Name[0] == 0xE5) continue;
            int match = 1;
            for (int j = 0; j < 11; j++) {
                if (entries[i].Name[j] != name[j]) {
                    match = 0;
                    break;
                }
            }
            if (match) {
                if (out_entry) *out_entry = entries[i];
                uint32 entry_cluster = ((uint32)entries[i].FstClusHI << 16) | entries[i].FstClusLO;
                return entry_cluster;
            }
        }
        cluster = fat32_next_cluster(disk_img, bpb, cluster);
    }
    return -1;
}

int fat32_write_file(void* disk_img, struct fat32_bpb* bpb, const char* filename, const char* buf, int bufsize) {
    if (!disk_img || !bpb || !filename || !buf || bufsize <= 0) return -1;
    uint32 byts_per_sec = bpb->BytsPerSec;
    uint32 sec_per_clus = bpb->SecPerClus;
    uint32 rsvd_sec_cnt = bpb->RsvdSecCnt;
    uint32 num_fats = bpb->NumFATs;
    uint32 fatsz = bpb->FATSz32;
    uint32 root_clus = bpb->RootClus;
    uint32 first_data_sec = rsvd_sec_cnt + (num_fats * fatsz);
    uint32 root_dir_sec = first_data_sec + ((root_clus - 2) * sec_per_clus);
    uint32 root_dir_offset = root_dir_sec * byts_per_sec;
    struct fat32_dir_entry* entries = (struct fat32_dir_entry*)((char*)disk_img + root_dir_offset);
    int entry_count = (byts_per_sec * sec_per_clus) / sizeof(struct fat32_dir_entry);

    // 1. Check if file already exists
    for (int i = 0; i < entry_count; i++) {
        if (entries[i].Name[0] == 0x00) break;
        if ((entries[i].Attr & 0x0F) == 0x0F) continue;
        if (entries[i].Name[0] == 0xE5) continue;
        int match = 1;
        for (int j = 0; j < 11; j++) {
            if (entries[i].Name[j] != filename[j]) {
                match = 0;
                break;
            }
        }
        if (match) return -2; // File exists
    }

    // 2. Find a free directory entry
    int free_entry = -1;
    for (int i = 0; i < entry_count; i++) {
        if (entries[i].Name[0] == 0x00 || entries[i].Name[0] == 0xE5) {
            free_entry = i;
            break;
        }
    }
    if (free_entry == -1) return -3; // No free dir entry

    // 3. Find a free cluster in the FAT
    uint32 fat_offset = bpb->RsvdSecCnt * bpb->BytsPerSec;
    uint32* fat = (uint32*)((char*)disk_img + fat_offset);
    uint32 total_clusters = (bpb->TotSec32 - first_data_sec) / sec_per_clus;
    uint32 first_free_clus = 2;
    for (; first_free_clus < total_clusters + 2; first_free_clus++) {
        if ((fat[first_free_clus] & 0x0FFFFFFF) == 0) break;
    }
    if (first_free_clus >= total_clusters + 2) return -4; // No free cluster

    // 4. Write data to the cluster
    uint32 data_sec = first_data_sec + ((first_free_clus - 2) * sec_per_clus);
    uint32 data_offset = data_sec * byts_per_sec;
    int max_bytes = byts_per_sec * sec_per_clus;
    int write_bytes = bufsize > max_bytes ? max_bytes : bufsize;
    memory_copy((char*)buf, (char*)disk_img + data_offset, write_bytes);
    if (write_bytes < max_bytes) memory_set((uint8*)((char*)disk_img + data_offset + write_bytes), 0, max_bytes - write_bytes);

    // 5. Update FAT (mark cluster as end-of-chain)
    fat[first_free_clus] = 0x0FFFFFF8;
    // Mirror to other FATs if present
    for (uint32 f = 1; f < num_fats; f++) {
        uint32* fat2 = (uint32*)((char*)disk_img + fat_offset + f * fatsz * byts_per_sec);
        fat2[first_free_clus] = 0x0FFFFFF8;
    }

    // 6. Fill directory entry
    struct fat32_dir_entry* entry = &entries[free_entry];
    for (int j = 0; j < 11; j++) entry->Name[j] = filename[j];
    entry->Attr = 0x20; // Archive, normal file
    entry->NTRes = 0;
    entry->CrtTimeTenth = 0;
    entry->CrtTime = 0;
    entry->CrtDate = 0;
    entry->LstAccDate = 0;
    entry->WrtTime = 0;
    entry->WrtDate = 0;
    entry->FstClusHI = (first_free_clus >> 16) & 0xFFFF;
    entry->FstClusLO = first_free_clus & 0xFFFF;
    entry->FileSize = write_bytes;

    return write_bytes;
}

// Sector-buffered FAT32 functions for real disk access
int fat32_read_bpb_sector(uint8 drive, struct fat32_bpb* bpb) {
    if (!bpb) return -1;
    uint8 sector[512];
    if (ata_read_sector(drive, 0, sector) != 0) return -1;
    memory_copy((char*)sector, (char*)bpb, sizeof(struct fat32_bpb));
    return 0;
}

int fat32_list_root_sector(uint8 drive, struct fat32_bpb* bpb) {
    if (!bpb) return -1;
    uint32 byts_per_sec = bpb->BytsPerSec;
    uint32 sec_per_clus = bpb->SecPerClus;
    uint32 rsvd_sec_cnt = bpb->RsvdSecCnt;
    uint32 num_fats = bpb->NumFATs;
    uint32 fatsz = bpb->FATSz32;
    uint32 root_clus = bpb->RootClus;
    uint32 first_data_sec = rsvd_sec_cnt + (num_fats * fatsz);
    uint32 root_dir_sec = first_data_sec + ((root_clus - 2) * sec_per_clus);
    
    printf("%cCluster size: %d sectors, Root cluster: %d, Start sector: %d\n", 255, 255, 255, sec_per_clus, root_clus, root_dir_sec);
    
    // Read all sectors in the root directory cluster
    uint8 sector[512];
    int total_entries = 0;
    for (uint32 sec = 0; sec < sec_per_clus; sec++) {
        printf("%cReading sector %d...\n", 255, 255, 255, root_dir_sec + sec);
        if (ata_read_sector(drive, root_dir_sec + sec, sector) != 0) {
            printf("%cFailed to read root directory sector %d\n", 255, 0, 0, sec);
            return -1;
        }
        struct fat32_dir_entry* entries = (struct fat32_dir_entry*)sector;
        int entries_in_sector = 0;
        // Process all 16 entries per sector (512 bytes / 32 bytes per entry)
        for (int i = 0; i < 16; i++) {
            if (entries[i].Name[0] == 0x00) {
                printf("%cEnd of directory found at entry %d in sector %d\n", 255, 255, 255, i, sec);
                return 0; // End of directory
            }
            if ((entries[i].Attr & 0x0F) == 0x0F) continue; // Skip LFN
            if (entries[i].Name[0] == 0xE5) continue; // Skip deleted
            char name[12];
            for (int j = 0; j < 11; j++) name[j] = entries[i].Name[j];
            name[11] = '\0';
            if (entries[i].Attr & 0x10) {
                printf("%c%s <DIR>\n", 255, 255, 255, name);
            } else {
                printf("%c%s\n", 255, 255, 255, name);
            }
            entries_in_sector++;
            total_entries++;
        }
        printf("%cFound %d entries in sector %d\n", 255, 255, 255, entries_in_sector, sec);
    }
    printf("%cTotal entries found: %d\n", 255, 255, 255, total_entries);
    return 0;
}

int fat32_read_file_sector(uint8 drive, struct fat32_bpb* bpb, const char* filename, char* buf, int bufsize) {
    if (!bpb || !filename || !buf) return -1;
    uint32 byts_per_sec = bpb->BytsPerSec;
    uint32 sec_per_clus = bpb->SecPerClus;
    uint32 rsvd_sec_cnt = bpb->RsvdSecCnt;
    uint32 num_fats = bpb->NumFATs;
    uint32 fatsz = bpb->FATSz32;
    uint32 root_clus = bpb->RootClus;
    uint32 first_data_sec = rsvd_sec_cnt + (num_fats * fatsz);
    uint32 root_dir_sec = first_data_sec + ((root_clus - 2) * sec_per_clus);
    
    // Search for file in root directory
    uint8 sector[512];
    for (uint32 sec = 0; sec < sec_per_clus; sec++) {
        if (ata_read_sector(drive, root_dir_sec + sec, sector) != 0) return -1;
        struct fat32_dir_entry* entries = (struct fat32_dir_entry*)sector;
        for (int i = 0; i < 16; i++) {
            if (entries[i].Name[0] == 0x00) return -1; // End of directory, file not found
            if ((entries[i].Attr & 0x0F) == 0x0F) continue; // Skip LFN
            if (entries[i].Name[0] == 0xE5) continue; // Skip deleted
            
            // Compare filename
            int match = 1;
            for (int j = 0; j < 11; j++) {
                if (entries[i].Name[j] != filename[j]) {
                    match = 0;
                    break;
                }
            }
            
            if (match) {
                // Found the file
                uint32 file_size = entries[i].FileSize;
                uint32 first_clus = ((uint32)entries[i].FstClusHI << 16) | entries[i].FstClusLO;
                if (first_clus < 2) return -1;
                
                printf("%cFile found: size=%d bytes, first cluster=%d\n", 255, 255, 255, file_size, first_clus);
                
                // For now, just read the first cluster (simplified)
                uint32 data_sec = first_data_sec + ((first_clus - 2) * sec_per_clus);
                uint32 bytes_to_read = file_size;
                if (bytes_to_read > bufsize) bytes_to_read = bufsize;
                if (bytes_to_read > byts_per_sec * sec_per_clus) bytes_to_read = byts_per_sec * sec_per_clus;
                
                // Read the first sector of the file
                if (ata_read_sector(drive, data_sec, (uint8*)buf) != 0) return -1;
                
                return bytes_to_read;
            }
        }
    }
    return -1; // File not found
} 