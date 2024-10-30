#include <stdio.h>
#include <string.h>
#include "FAT12.h"
#include "stdint.h"
#include "stdlib.h"



// Hàm t?o boot sector trong buffer
void create_boot_sector(uint8_t *buffer,uint16_t SECTOR_SIZE, uint16_t TOTAL_SECTORS) {
    struct BootSector boot_sector = {0};
    
    boot_sector.jump[0] = 0xEB;
    boot_sector.jump[1] = 0x3C;
    boot_sector.jump[2] = 0x90;
    memcpy(boot_sector.oem, "MSDOS5.0", 8);
    boot_sector.bytes_per_sector = SECTOR_SIZE;
    boot_sector.sectors_per_cluster = 1;
    boot_sector.reserved_sectors = 1;
    boot_sector.num_fats = 2;
    boot_sector.root_dir_entries = 224;
    boot_sector.total_sectors_short = TOTAL_SECTORS;
    boot_sector.media_descriptor = 0xF0;
    boot_sector.sectors_per_fat = 9;
    boot_sector.sectors_per_track = 18;
    boot_sector.num_heads = 2;
    boot_sector.hidden_sectors = 0;
    boot_sector.total_sectors_long = 0;
    boot_sector.drive_number = 0x00;
    boot_sector.boot_signature = 0x29;
    boot_sector.volume_id = 0x12345678;
    memcpy(boot_sector.volume_label, "NO NAME    ", 11);
    memcpy(boot_sector.fs_type, "FAT12   ", 8);
    
    memcpy(buffer, &boot_sector, sizeof(struct BootSector));

    // Ði?n các byte còn l?i c?a sector 0
    memset(buffer + sizeof(struct BootSector), 0, SECTOR_SIZE - sizeof(struct BootSector));
		buffer[SECTOR_SIZE - 2] = 0x55;
    buffer[SECTOR_SIZE - 1] = 0xAA;
}

// Hàm t?o b?ng FAT trong buffer
void create_fat_tables(uint8_t *buffer,uint16_t SECTOR_SIZE, uint16_t TOTAL_SECTORS) {
    uint8_t fat[SECTOR_SIZE * 9] ;
		memset(fat,0,sizeof(fat));
    // Ð?t các byte d?c bi?t cho FAT12
    fat[0] = 0xF0; // Media type
    fat[1] = 0xFF;
    fat[2] = 0xFF; // K?t thúc chu?i cluster
    
		fat[3] = 0xFF;
    fat[4] = 0x0F;
    // Vi?t b?ng FAT 1 vào buffer
    memcpy(buffer + SECTOR_SIZE, fat, sizeof(fat));

    // Vi?t b?ng FAT 2 (b?n sao c?a FAT 1) vào buffer
    memcpy(buffer + SECTOR_SIZE * 10, fat, sizeof(fat));
}

// Hàm t?o thu m?c g?c trong buffer
void create_root_directory(uint8_t *buffer,uint16_t SECTOR_SIZE, uint16_t TOTAL_SECTORS) {
    // 14 sectors cho thu m?c g?c (224 entries)
    memset(buffer + SECTOR_SIZE * 19, 0, SECTOR_SIZE * 14);
}

// T?o dia ?o FAT12 trên buffer
void create_fat12_disk(uint8_t *buffer,uint16_t SECTOR_SIZE, uint16_t TOTAL_SECTORS ) {
    // Kh?i t?o t?t c? b? nh? dia thành 0
    memset(buffer, 0, SECTOR_SIZE * TOTAL_SECTORS);

    // T?o boot sector
    create_boot_sector(buffer,SECTOR_SIZE,  TOTAL_SECTORS);

    // T?o hai b?ng FAT
    create_fat_tables(buffer, SECTOR_SIZE, TOTAL_SECTORS);

    // T?o thu m?c g?c
    create_root_directory(buffer, SECTOR_SIZE, TOTAL_SECTORS);
}
