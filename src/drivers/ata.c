#include "../../include/types.h"
#include "../../include/system.h"
#include "../../include/vga.h"

#define ATA_PRIMARY_IO 0x1F0
#define ATA_SECONDARY_IO 0x170
#define ATA_PRIMARY_CTRL 0x3F6
#define ATA_SECONDARY_CTRL 0x376

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

#define ATA_CMD_READ_PIO   0x20
#define ATA_CMD_WRITE_PIO  0x30
#define ATA_CMD_IDENTIFY   0xEC

#define ATA_SR_BSY     0x80
#define ATA_SR_DRDY    0x40
#define ATA_SR_DF      0x20
#define ATA_SR_DSC     0x10
#define ATA_SR_DRQ     0x08
#define ATA_SR_CORR    0x04
#define ATA_SR_IDX     0x02
#define ATA_SR_ERR     0x01

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

int ata_identify(uint8 drive, uint16* identify_data) {
    uint16 io_base = (drive & 2) ? ATA_SECONDARY_IO : ATA_PRIMARY_IO;
    uint8 slavebit = (drive & 1) ? 0xB0 : 0xA0;
    outportb(io_base + ATA_REG_HDDEVSEL, slavebit);
    ata_io_wait(io_base);
    outportb(io_base + ATA_REG_SECCOUNT0, 0);
    outportb(io_base + ATA_REG_LBA0, 0);
    outportb(io_base + ATA_REG_LBA1, 0);
    outportb(io_base + ATA_REG_LBA2, 0);
    outportb(io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ata_io_wait(io_base);
    if (inportb(io_base + ATA_REG_STATUS) == 0) return -1;
    int timeout = 1000000;
    while ((inportb(io_base + ATA_REG_STATUS) & ATA_SR_BSY) && --timeout);
    if (timeout == 0) { printf("%cIDENTIFY timeout (BSY)\n", 255,0,0); return -1; }
    timeout = 1000000;
    while (!(inportb(io_base + ATA_REG_STATUS) & ATA_SR_DRQ) && --timeout);
    if (timeout == 0) { printf("%cIDENTIFY timeout (DRQ)\n", 255,0,0); return -1; }
    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(io_base + ATA_REG_DATA);
    }
    return 0;
}

int ata_read_sector(uint8 drive, uint32 lba, uint8* buf) {
    uint16 io_base = (drive & 2) ? ATA_SECONDARY_IO : ATA_PRIMARY_IO;
    uint8 slavebit = (drive & 1) ? 0xF0 : 0xE0;
    outportb(io_base + ATA_REG_HDDEVSEL, slavebit | ((lba >> 24) & 0x0F));
    outportb(io_base + ATA_REG_SECCOUNT0, 1);
    outportb(io_base + ATA_REG_LBA0, (uint8)(lba & 0xFF));
    outportb(io_base + ATA_REG_LBA1, (uint8)((lba >> 8) & 0xFF));
    outportb(io_base + ATA_REG_LBA2, (uint8)((lba >> 16) & 0xFF));
    outportb(io_base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);
    int timeout = 1000000;
    while ((inportb(io_base + ATA_REG_STATUS) & ATA_SR_BSY) && --timeout);
    if (timeout == 0) { printf("%cREAD timeout (BSY)\n", 255,0,0); return -1; }
    timeout = 1000000;
    while (!(inportb(io_base + ATA_REG_STATUS) & ATA_SR_DRQ) && --timeout);
    if (timeout == 0) { printf("%cREAD timeout (DRQ)\n", 255,0,0); return -1; }
    for (int i = 0; i < 256; i++) {
        uint16 data = inw(io_base + ATA_REG_DATA);
        buf[i*2] = data & 0xFF;
        buf[i*2+1] = (data >> 8) & 0xFF;
    }
    ata_io_wait(io_base);
    return 0;
}

int ata_write_sector(uint8 drive, uint32 lba, const uint8* buf) {
    uint16 io_base = (drive & 2) ? ATA_SECONDARY_IO : ATA_PRIMARY_IO;
    uint8 slavebit = (drive & 1) ? 0xF0 : 0xE0;
    outportb(io_base + ATA_REG_HDDEVSEL, slavebit | ((lba >> 24) & 0x0F));
    outportb(io_base + ATA_REG_SECCOUNT0, 1);
    outportb(io_base + ATA_REG_LBA0, (uint8)(lba & 0xFF));
    outportb(io_base + ATA_REG_LBA1, (uint8)((lba >> 8) & 0xFF));
    outportb(io_base + ATA_REG_LBA2, (uint8)((lba >> 16) & 0xFF));
    outportb(io_base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
    int timeout = 1000000;
    while ((inportb(io_base + ATA_REG_STATUS) & ATA_SR_BSY) && --timeout);
    if (timeout == 0) { printf("%cWRITE timeout (BSY)\n", 255,0,0); return -1; }
    timeout = 1000000;
    while (!(inportb(io_base + ATA_REG_STATUS) & ATA_SR_DRQ) && --timeout);
    if (timeout == 0) { printf("%cWRITE timeout (DRQ)\n", 255,0,0); return -1; }
    for (int i = 0; i < 256; i++) {
        uint16 data = buf[i*2] | (buf[i*2+1] << 8);
        outw(io_base + ATA_REG_DATA, data);
    }
    ata_io_wait(io_base);
    return 0;
} 