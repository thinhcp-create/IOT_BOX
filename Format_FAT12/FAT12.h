#ifndef FAT12_H
#define FAT12_H
#include "stdint.h"

struct BootSector {
    uint8_t  jump[3];          // L?nh jump
    char     oem[8];           // Nh?n di?n OEM
    uint16_t bytes_per_sector; // S? byte m?i sector
    uint8_t  sectors_per_cluster; // S? sector m?i cluster
    uint16_t reserved_sectors; // S? sector dành riêng
    uint8_t  num_fats;         // S? lu?ng FAT
    uint16_t root_dir_entries; // S? lu?ng entries trong thu m?c g?c
    uint16_t total_sectors_short; // T?ng s? sector (n?u nh? hon 32K)
    uint8_t  media_descriptor; // Media descriptor
    uint16_t sectors_per_fat;  // S? sector m?i FAT
    uint16_t sectors_per_track; // S? sector m?i track (dia m?m)
    uint16_t num_heads;        // S? d?u d?c (dia m?m)
    uint32_t hidden_sectors;   // S? sector ?n
    uint32_t total_sectors_long; // T?ng s? sector (n?u l?n hon 32K)
    uint8_t  drive_number;     // S? ? dia
    uint8_t  reserved1;        // Ð?t riêng
    uint8_t  boot_signature;   // Ch? ký boot
    uint32_t volume_id;        // S? serial c?a volume
    char     volume_label[11]; // Nhãn volume
    char     fs_type[8];       // Lo?i h? th?ng t?p (FAT12)
} __attribute__((packed));

void create_fat12_disk(uint8_t *buffer,uint16_t SECTOR_SIZE, uint16_t TOTAL_SECTORS);

#endif // FAT12_H
