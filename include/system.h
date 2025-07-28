#ifndef SYSTEM_H
#define SYSTEM_H
#include "types.h"
#include <stdint.h>

uint8 inportb (uint16 _port);
void outportb (uint16 _port, uint8 _data);
void sleep(uint8 times);
void Shutdown(void);
int ata_read_sector(uint8 drive, uint32 lba, uint8* buf);
int ata_write_sector(uint8 drive, uint32 lba, const uint8* buf);
int ata_identify(uint8 drive, uint16* identify_data);
void ata_init_drives(void);
int ata_detect_drive(uint8 drive);
int ata_drive_present(uint8 drive);
uint16 inw(uint16 _port);
void outw(uint16 _port, uint16 _data);



#endif