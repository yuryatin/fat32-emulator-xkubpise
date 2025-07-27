#include "format.h"

boolean preformat(void) {
    // Allocate and zero the first 512 bytes (boot sector)
    unsigned char bootSector[SECTOR_SIZE] = {0};

    // Jump instruction (for compatibility although it is not intented to be bootable)
    bootSector[0x00] = 0xEB;
    bootSector[0x01] = 0x58;
    bootSector[0x02] = 0x90;
    // OEM name
    memcpy(&bootSector[0x03], "xkubpise", 8);  // OEM

    // BIOS Parameter Block
    bootSector[0x0B] = SECTOR_SIZE & 0xFF;
    bootSector[0x0C] = (SECTOR_SIZE >> 8) & 0xFF;
    bootSector[0x0D] = SECTORS_PER_CLUSTER;
    bootSector[0x0E] = N_RESERVED_SECTORS & 0xFF;
    bootSector[0x0F] = (N_RESERVED_SECTORS >> 8) & 0xFF;
    bootSector[0x10] = N_FATS;
    bootSector[0x15] = 0xF8; // Media descriptor (fixed disk)
    bootSector[0x16] = 0x00; // FAT size (16-bit) = 0 for FAT32
    bootSector[0x18] = 0x3F; // Sectors per track (dummy)
    bootSector[0x1A] = 0xFF; // Number of heads (dummy)

    // Total sectors (32-bit)
    uint32_t totalSectors = TOTAL_N_SECTORS;
    bootSector[0x20] = totalSectors & 0xFF;
    bootSector[0x21] = (totalSectors >> 8) & 0xFF;
    bootSector[0x22] = (totalSectors >> 16) & 0xFF;
    bootSector[0x23] = (totalSectors >> 24) & 0xFF;

    // FAT size (32-bit)
    bootSector[0x24] = FAT_SIZE & 0xFF;
    bootSector[0x25] = (FAT_SIZE >> 8) & 0xFF;
    bootSector[0x26] = (FAT_SIZE >> 16) & 0xFF;
    bootSector[0x27] = (FAT_SIZE >> 24) & 0xFF;

    // Extended flags (bit 7 = 1, mirror disabled, active FAT = 0)
    bootSector[0x28] = 0x80;
    bootSector[0x29] = 0x00;

    // File system version = 0
    bootSector[0x2A] = 0x00;
    bootSector[0x2B] = 0x00;

    // Root cluster
    bootSector[0x2C] = 0x02;
    bootSector[0x2D] = 0x00;
    bootSector[0x2E] = 0x00;
    bootSector[0x2F] = 0x00;

    // FSInfo sector
    bootSector[0x30] = 0x01;
    bootSector[0x31] = 0x00;

    // Backup boot sector
    bootSector[0x32] = 0x06;
    bootSector[0x33] = 0x00;

    // Reserved
    memset(&bootSector[0x34], 0, 12);

    // Drive number
    bootSector[0x40] = 0x80;

    // Boot signature
    bootSector[0x42] = 0x29;

    // Volume ID
    uint32_t volumeID = 0xEFA032BE;
    bootSector[0x43] = volumeID & 0xFF;
    bootSector[0x44] = (volumeID >> 8) & 0xFF;
    bootSector[0x45] = (volumeID >> 16) & 0xFF;
    bootSector[0x46] = (volumeID >> 24) & 0xFF;

    // Volume label
    memcpy(&bootSector[0x47], "FATxkubpise", 11);

    // File system type
    memcpy(&bootSector[0x52], "FAT32   ", 8);

    // Boot sector signature (for compatibility these bytes are marked bootable)
    bootSector[0x1FE] = 0x55;
    bootSector[0x1FF] = 0xAA;

    // Write main boot sector
    if (fseek(volume, 0, SEEK_SET) != 0) {
        perror("Error saving sector 0");
        fclose(volume);
        return False;
    }
    if (fwrite(bootSector, 1, SECTOR_SIZE, volume) != SECTOR_SIZE) {
        perror("Error saving sector 0");
        fclose(volume);
        return False;
    }

    // Write backup boot sector
    if (fseek(volume, 6 * SECTOR_SIZE, SEEK_SET) != 0) {
        perror("Error saving sector 6");
        fclose(volume);
        return False;
    }
    if (fwrite(bootSector, 1, SECTOR_SIZE, volume) != SECTOR_SIZE) {
        perror("Error saving sector 6");
        fclose(volume);
        return False;
    }
    return True;
}