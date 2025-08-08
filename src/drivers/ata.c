#include <types.h>
#include <system.h>
#include <vga.h>

#define ATA_PRIMARY_IO 0x1F0
#define ATA_SECONDARY_IO 0x170
#define ATA_PRIMARY_CTRL 0x3F6
#define ATA_SECONDARY_CTRL 0x376

// SATA ports (common on Dell Optiplex 755)
#define SATA_PRIMARY_IO 0x1F0
#define SATA_SECONDARY_IO 0x170
#define SATA_PRIMARY_CTRL 0x3F6
#define SATA_SECONDARY_CTRL 0x376

#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07
#define ATA_REG_ALTSTATUS  0x206

// Enhanced commands for SATA compatibility
#define ATA_CMD_READ_PIO   0x20
#define ATA_CMD_WRITE_PIO  0x30
#define ATA_CMD_IDENTIFY   0xEC
#define ATA_CMD_IDENTIFY_PACKET 0xA1
#define ATA_CMD_SET_FEATURES 0xEF
#define ATA_CMD_SLEEP      0xE6
#define ATA_CMD_STANDBY    0xE2
#define ATA_CMD_IDLE       0xE3

#define ATA_SR_BSY     0x80
#define ATA_SR_DRDY    0x40
#define ATA_SR_DF      0x20
#define ATA_SR_DSC     0x10
#define ATA_SR_DRQ     0x08
#define ATA_SR_CORR    0x04
#define ATA_SR_IDX     0x02
#define ATA_SR_ERR     0x01

// SATA specific features
#define ATA_FEATURE_SATA_ENABLE 0x10
#define ATA_FEATURE_SATA_DISABLE 0x90

// Drive detection structure
typedef struct {
    uint8 present;
    uint8 type;  // 0=IDE, 1=SATA, 2=RAID
    char model[41];
    uint32 sectors;
    uint32 size_mb;
} drive_info_t;

static drive_info_t detected_drives[8];

static void ata_io_wait(uint16 io_base) {
    for (int i = 0; i < 4; i++) inportb(io_base + ATA_REG_ALTSTATUS);
}

static int ata_poll(uint16 io_base) {
    for (int i = 0; i < 100000; i++) {
        uint8 status = inportb(io_base + ATA_REG_STATUS);
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRDY)) return 0;
    }
    return -1;
}

// Enhanced drive detection for SATA compatibility
int ata_detect_drive(uint8 drive) {
    uint16 io_base = (drive & 2) ? ATA_SECONDARY_IO : ATA_PRIMARY_IO;
    uint8 slavebit = (drive & 1) ? 0xB0 : 0xA0;
    
    // Reset the drive first
    outportb(io_base + ATA_REG_HDDEVSEL, slavebit);
    ata_io_wait(io_base);
    
    // Try to detect if drive is present
    uint8 status = inportb(io_base + ATA_REG_STATUS);
    if (status == 0) {
        return -1; // No drive present
    }
    
    // Try to identify the drive
    uint16 identify_data[256];
    int result = ata_identify(drive, identify_data);
    
    if (result == 0) {
        // Drive found, extract information
        detected_drives[drive].present = 1;
        
        // Extract model name
        for (int i = 0; i < 20; i++) {
            detected_drives[drive].model[i*2] = (identify_data[27+i] >> 8) & 0xFF;
            detected_drives[drive].model[i*2+1] = identify_data[27+i] & 0xFF;
        }
        detected_drives[drive].model[40] = '\0';
        
        // Get drive size
        detected_drives[drive].sectors = identify_data[60] | (identify_data[61] << 16);
        detected_drives[drive].size_mb = (detected_drives[drive].sectors / 2048);
        
        // Determine drive type based on identify data
        if (identify_data[83] & 0x0400) {
            detected_drives[drive].type = 1; // SATA
        } else {
            detected_drives[drive].type = 0; // IDE
        }
        
        return 0;
    }
    
    return -1;
}

// Initialize all drives during system startup
void ata_init_drives() {
    // Clear drive info
    for (int i = 0; i < 8; i++) {
        detected_drives[i].present = 0;
        detected_drives[i].type = 0;
        detected_drives[i].sectors = 0;
        detected_drives[i].size_mb = 0;
        detected_drives[i].model[0] = '\0';
    }
    
    // Probe only primary drives (0,1) first for faster boot
    // Secondary drives (2,3) are less common and can be detected on-demand
    for (int drive = 0; drive < 2; drive++) {
        ata_detect_drive(drive);
    }
}

int ata_identify(uint8 drive, uint16* identify_data) {
    uint16 io_base = (drive & 2) ? ATA_SECONDARY_IO : ATA_PRIMARY_IO;
    uint8 slavebit = (drive & 1) ? 0xB0 : 0xA0;
    
    // Reset drive
    outportb(io_base + ATA_REG_HDDEVSEL, slavebit);
    ata_io_wait(io_base);
    
    // Clear registers
    outportb(io_base + ATA_REG_SECCOUNT0, 0);
    outportb(io_base + ATA_REG_LBA0, 0);
    outportb(io_base + ATA_REG_LBA1, 0);
    outportb(io_base + ATA_REG_LBA2, 0);
    
    // Send IDENTIFY command
    outportb(io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ata_io_wait(io_base);
    
    uint8 status = inportb(io_base + ATA_REG_STATUS);
    
    if (status == 0) {
        return -1;
    }
    
    // Wait for BSY to clear (reduced timeout for faster boot)
    int timeout = 10000; // Reduced from 1000000
    while ((inportb(io_base + ATA_REG_STATUS) & ATA_SR_BSY) && --timeout);
    if (timeout == 0) { 
        return -1; 
    }
    
    // Wait for DRQ to set (reduced timeout for faster boot)
    timeout = 10000; // Reduced from 1000000
    while (!(inportb(io_base + ATA_REG_STATUS) & ATA_SR_DRQ) && --timeout);
    if (timeout == 0) { 
        return -1; 
    }
    
    // Read identify data
    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(io_base + ATA_REG_DATA);
    }
    
    return 0;
}

int ata_read_sector(uint8 drive, uint32 lba, uint8* buf) {
    if (drive >= 8 || !detected_drives[drive].present) {
        return -1;
    }
    
    uint16 io_base = (drive & 2) ? ATA_SECONDARY_IO : ATA_PRIMARY_IO;
    uint8 slavebit = (drive & 1) ? 0xF0 : 0xE0;
    
    // Set up LBA addressing
    outportb(io_base + ATA_REG_HDDEVSEL, slavebit | ((lba >> 24) & 0x0F));
    outportb(io_base + ATA_REG_SECCOUNT0, 1);
    outportb(io_base + ATA_REG_LBA0, (uint8)(lba & 0xFF));
    outportb(io_base + ATA_REG_LBA1, (uint8)((lba >> 8) & 0xFF));
    outportb(io_base + ATA_REG_LBA2, (uint8)((lba >> 16) & 0xFF));
    
    // Send read command
    outportb(io_base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);
    
    // Wait for BSY to clear (reduced timeout for faster boot)
    int timeout = 10000; // Reduced from 1000000
    while ((inportb(io_base + ATA_REG_STATUS) & ATA_SR_BSY) && --timeout);
    if (timeout == 0) { 
        return -1; 
    }
    
    // Wait for DRQ to set (reduced timeout for faster boot)
    timeout = 10000; // Reduced from 1000000
    while (!(inportb(io_base + ATA_REG_STATUS) & ATA_SR_DRQ) && --timeout);
    if (timeout == 0) { 
        return -1; 
    }
    
    // Read data
    for (int i = 0; i < 256; i++) {
        uint16 data = inw(io_base + ATA_REG_DATA);
        buf[i*2] = data & 0xFF;
        buf[i*2+1] = (data >> 8) & 0xFF;
    }
    
    ata_io_wait(io_base);
    return 0;
}

int ata_write_sector(uint8 drive, uint32 lba, const uint8* buf) {
    if (drive >= 8 || !detected_drives[drive].present) {
        return -1;
    }
    
    uint16 io_base = (drive & 2) ? ATA_SECONDARY_IO : ATA_PRIMARY_IO;
    uint8 slavebit = (drive & 1) ? 0xF0 : 0xE0;
    
    // Set up LBA addressing
    outportb(io_base + ATA_REG_HDDEVSEL, slavebit | ((lba >> 24) & 0x0F));
    outportb(io_base + ATA_REG_SECCOUNT0, 1);
    outportb(io_base + ATA_REG_LBA0, (uint8)(lba & 0xFF));
    outportb(io_base + ATA_REG_LBA1, (uint8)((lba >> 8) & 0xFF));
    outportb(io_base + ATA_REG_LBA2, (uint8)((lba >> 16) & 0xFF));
    
    // Send write command
    outportb(io_base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

    // Wait for BSY to clear and DRQ to set (reduced timeout for faster boot)
    int timeout = 10000; // Reduced from 1000000
    while ((inportb(io_base + ATA_REG_STATUS) & ATA_SR_BSY) && --timeout);
    if (timeout == 0) { 
        return -1; 
    }
    
    timeout = 10000; // Reduced from 1000000
    while (!(inportb(io_base + ATA_REG_STATUS) & ATA_SR_DRQ) && --timeout);
    if (timeout == 0) { 
        return -1; 
    }

    // Write the data
    for (int i = 0; i < 256; i++) {
        uint16 data = buf[i*2] | (buf[i*2+1] << 8);
        outw(io_base + ATA_REG_DATA, data);
    }

    ata_io_wait(io_base);

    // Wait for BSY to clear and DRDY to set after writing
    timeout = 1000000;
    while ((inportb(io_base + ATA_REG_STATUS) & ATA_SR_BSY) && --timeout);
    if (timeout == 0) { 
        return -1; 
    }
    
    timeout = 1000000;
    while (!(inportb(io_base + ATA_REG_STATUS) & ATA_SR_DRDY) && --timeout);
    if (timeout == 0) { 
        return -1; 
    }

    // Check for errors
    uint8 status = inportb(io_base + ATA_REG_STATUS);
    if (status & (ATA_SR_ERR | ATA_SR_DF)) {
        return -1;
    }

    return 0;
}

// Get drive information
drive_info_t* ata_get_drive_info(uint8 drive) {
    if (drive >= 8) return NULL;
    return &detected_drives[drive];
}

// Check if drive is present
int ata_drive_present(uint8 drive) {
    if (drive >= 8) return 0;
    return detected_drives[drive].present;
} 