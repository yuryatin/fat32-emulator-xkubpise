#include "fat32.h"
#include "utils.h"

extern char fat32ReadingErrors[FAT32ERRORS_SIZE];
extern FILE * volume;

static void appendToFAT32ReadingErrors(const char * newError, ...) {
    static size_t end = 0;
    size_t spaceLeft = FAT32ERRORS_SIZE - end - 1;
    if (spaceLeft == 0) return;

    if (spaceLeft > 0) {
        fat32ReadingErrors[end++] = '\t';
        --spaceLeft;
    }
    if (spaceLeft == 0) return;

    va_list args;
    va_start(args, newError);
    size_t written = vsnprintf(fat32ReadingErrors + end, spaceLeft + 1, newError, args);
    va_end(args);

    if (written <= 0) return;
    end += (written < spaceLeft) ? written : spaceLeft;
}

void commentOnExtFlags(uint16_t bpb_ExtFlags) {
    uint8_t activeFAT = bpb_ExtFlags & 0x000F;         // bits 0–3
    uint8_t mirrorDisabled = (bpb_ExtFlags >> 7) & 1;  // bit 7

    appendToFAT32ReadingErrors("BIOS Parameter Block has unexpected Extended Flags: 0x%04X\n", bpb_ExtFlags);

    if (mirrorDisabled)
        appendToFAT32ReadingErrors("Mirroring is DISABLED which is expected by FAT32 emulator \033[34mxkubpise\033[0m\n");
    else appendToFAT32ReadingErrors("\tMirroring is ENABLED which is NOT expected by FAT32 emulator \033[34mxkubpise\033[0m\n");

    if (activeFAT == 0)
        appendToFAT32ReadingErrors("\tActive FAT is 0, which is expected by FAT32 emulator \033[34mxkubpise\033[0m\n");
    else if (activeFAT > N_FATS)
        appendToFAT32ReadingErrors("\tActive FAT #%u is greater than the number of FATs (%d) in volume for FAT32 emulator \033[34mxkubpise\033[0m\n", activeFAT, N_FATS);
    else appendToFAT32ReadingErrors("\tActive FAT #%u and this is expected by FAT32 emulator \033[34mxkubpise\033[0m\n", activeFAT);

    if (bpb_ExtFlags & 0xFF70)
        appendToFAT32ReadingErrors("\tWarning: Reserved bits in Extended Flags are non-zero!\n");
}

boolean isValidFAT32xkubpise(const char * filename) {
    boolean anyErrors = False;
    volume = fopen(filename, "rb");
    if (!volume) {
        appendToFAT32ReadingErrors("Error opening the volume\n");
        return False;
    } else {
        fseek(volume, 0, SEEK_END);
        long size = ftell(volume);
        fseek(volume, 0, SEEK_SET);
        if (size != TOTAL_SIZE) {
            appendToFAT32ReadingErrors("Volume doesn't have mandatory size of 20 MB\n\tIts size is %ld bytes\n", size);
            anyErrors = True;
        }
        if (size < SECTOR_SIZE) {
            appendToFAT32ReadingErrors("Volume is even smaller than %d bytes to host its BIOS Parameter Block\n", SECTOR_SIZE);
            fclose(volume);
            return False;
        }
    }

    unsigned char buffer[SECTOR_SIZE];
    size_t bootSectorRead = fread(buffer, 1, SECTOR_SIZE, volume);
    if(bootSectorRead != SECTOR_SIZE) {
        appendToFAT32ReadingErrors("Couldn't read full boot sector\nOnly %ld bytes have been read\n", bootSectorRead);
        fclose(volume);
        return False;
    } else fclose(volume);

    if (!(buffer[0x52] == 'F' && buffer[0x53] == 'A' && buffer[0x54] == 'T' && buffer[0x55] == '3' && buffer[0x56] == '2')) {
        appendToFAT32ReadingErrors("Unexpected FAT32 signature: %c%c%c%c%c%c%c%c\n\t\t(hex): %02X %02X %02X %02X %02X %02X %02X %02X\n", safeChar(buffer[0x52]), safeChar(buffer[0x53]), safeChar(buffer[0x54]), safeChar(buffer[0x55]), safeChar(buffer[0x56]), safeChar(buffer[0x57]), safeChar(buffer[0x58]), safeChar(buffer[0x59]), (unsigned char)buffer[0x52], (unsigned char)buffer[0x53], (unsigned char)buffer[0x54], (unsigned char)buffer[0x55], (unsigned char)buffer[0x56], (unsigned char)buffer[0x57], (unsigned char)buffer[0x58], (unsigned char)buffer[0x59]);
        anyErrors = True;
    }

    const char customOEM[8] = { 'x','k','u','b','p','i','s','e' };
    uint32_t customVolumeID = 0xEFA032BE;
    uint32_t volumeID = buffer[0x43] | (buffer[0x44] << 8) | (buffer[0x45] << 16) | (buffer[0x46] << 24);
    const char customVolumeLabel[11] = { 'F','A','T','x','k','u','b','p','i','s','e' };

    if (memcmp(&buffer[0x03], customOEM, 8) != 0) {
        appendToFAT32ReadingErrors("Unexpected OEM Name: %c%c%c%c%c%c%c%c \n\t\t(hex): %02X %02X %02X %02X %02X %02X %02X %02X\n", safeChar(buffer[0x03]), safeChar(buffer[0x04]), safeChar(buffer[0x05]), safeChar(buffer[0x06]), safeChar(buffer[0x07]), safeChar(buffer[0x08]), safeChar(buffer[0x09]), safeChar(buffer[0x0a]), (unsigned char)buffer[0x03], (unsigned char)buffer[0x04], (unsigned char)buffer[0x05], (unsigned char)buffer[0x06], (unsigned char)buffer[0x07], (unsigned char)buffer[0x08], (unsigned char)buffer[0x09], (unsigned char)buffer[0x0a]);
        anyErrors = True;
    }
    if (volumeID != customVolumeID) {
        appendToFAT32ReadingErrors("Unexpected Volume ID: %08X\n", volumeID);
        anyErrors = True;
    }
    if (memcmp(&buffer[0x47], customVolumeLabel, 11) != 0) {
        appendToFAT32ReadingErrors("Unexpected Volume Label: %c%c%c%c%c%c%c%c%c%c%c\n\t\t(hex): %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n", safeChar(buffer[0x47]), safeChar(buffer[0x48]), safeChar(buffer[0x49]), safeChar(buffer[0x4a]), safeChar(buffer[0x4b]), safeChar(buffer[0x4c]), safeChar(buffer[0x4d]), safeChar(buffer[0x4e]), safeChar(buffer[0x4f]), safeChar(buffer[0x50]), safeChar(buffer[0x51]), (unsigned char)buffer[0x47], (unsigned char)buffer[0x48], (unsigned char)buffer[0x49], (unsigned char)buffer[0x4a], (unsigned char)buffer[0x4b], (unsigned char)buffer[0x4c], (unsigned char)buffer[0x4d], (unsigned char)buffer[0x4e], (unsigned char)buffer[0x4f], (unsigned char)buffer[0x50], (unsigned char)buffer[0x51]);
        anyErrors = True;
    }

    uint16_t bpb_BytsPerSec = buffer[0x0B] | (buffer[0x0C] << 8);
    if (bpb_BytsPerSec != SECTOR_SIZE) {
        appendToFAT32ReadingErrors("Unexpected sector size: %u\n", bpb_BytsPerSec);
        anyErrors = True;
    }
    uint8_t bpb_SecPerClus = buffer[0x0D];
    if (bpb_SecPerClus != SECTORS_PER_CLUSTER) {
        appendToFAT32ReadingErrors("Unexpected sector per cluster: %u\n", bpb_SecPerClus);
        anyErrors = True;
    }
    uint16_t bpb_RsvdSecCnt = buffer[0x0E] | (buffer[0x0F] << 8);
    if (bpb_RsvdSecCnt != N_RESERVED_SECTORS) {
        appendToFAT32ReadingErrors("Unexpected number of reserved sectors: %u\n", bpb_RsvdSecCnt);
        anyErrors = True;
    }
    uint8_t bpb_NumFATs = buffer[0x10];
    if (bpb_NumFATs != N_FATS) {
        appendToFAT32ReadingErrors("Unexpected number of FAT tables: %u\n", bpb_NumFATs);
        anyErrors = True;
    }
    uint16_t bpb_RootEntCnt = buffer[0x11] | (buffer[0x12] << 8);
    if (bpb_RootEntCnt != 0) { // FAT32 does not use root directory entries
        appendToFAT32ReadingErrors("BIOS Parameter Block indicates root directory\n");
        anyErrors = True;
    } 
    uint16_t bpb_TotSec16 = buffer[0x13] | (buffer[0x14] << 8);
    if (bpb_TotSec16 != 0) { // FAT32 does not use this field
        appendToFAT32ReadingErrors("BIOS Parameter Block indicates total number of sectors in 16-bit field\n");
        anyErrors = True;
    } 
    uint32_t bpb_TotSec32 = buffer[0x20] | (buffer[0x21] << 8) |
            (buffer[0x22] << 16) | (buffer[0x23] << 24);
    if (bpb_TotSec32 != TOTAL_N_SECTORS) {
        appendToFAT32ReadingErrors("Volume for FAT32 emulator \033[34mxkubpise\033[0m must have %d sectors in total, not %u sectors\n", TOTAL_N_SECTORS, bpb_TotSec32);
        anyErrors = True;
    }
    uint16_t bpb_FATSz16 = buffer[0x16] | (buffer[0x17] << 8);
    if (bpb_FATSz16 != 0) {
        if (bpb_TotSec16 != 0) { // FAT32 does not use 16-bit FAT size
            appendToFAT32ReadingErrors("BIOS Parameter Block indicates size of a FAT table in 16-bit field\n");
            anyErrors = True;
        } 
    }
    uint32_t bpb_FATSz32 = buffer[0x24] | (buffer[0x25] << 8) |
            (buffer[0x26] << 16) | (buffer[0x27] << 24);
    if (bpb_FATSz32 != FAT_SIZE) {
        appendToFAT32ReadingErrors("FAT table size in volume for FAT32 emulator \033[34mxkubpise\033[0m must be %d bytes, not %u bytes\n", FAT_SIZE, bpb_FATSz32);
        anyErrors = True;
    }
    uint16_t bpb_ExtFlags = buffer[0x28] | (buffer[0x29] << 8);
    if (bpb_ExtFlags != 0x80) { // FAT32 emulator xkubpise has no mirroring and use the first FAT table
        commentOnExtFlags(bpb_ExtFlags);
        anyErrors = True;
    } 
    uint16_t bpb_FSVer = buffer[0x2A] | (buffer[0x2B] << 8);
    if (bpb_FSVer != 0) {
        appendToFAT32ReadingErrors("FAT32 File System Version (should be zero): %u\n", bpb_FSVer);
        anyErrors = True;
    }
    return !anyErrors;
}