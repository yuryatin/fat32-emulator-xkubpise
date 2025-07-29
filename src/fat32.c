#include "fat32.h"
#include "utils.h"

extern boolean enforceAbsolutePath;
extern char fat32ReadingErrors[FAT32ERRORS_SIZE];
extern FILE * volume;
extern char localFilesAndFolders[SECTOR_SIZE / FAT_ENTRY_SIZE][FULL_FILE_STRING_SIZE];

success readSector(uint32_t sector, uint8_t * buffer) {
    if (fseek(volume, sector * SECTOR_SIZE, SEEK_SET) != 0) {
        perror("fseek");
        return Failure;
    }
    if (fread(buffer, 1, SECTOR_SIZE, volume) != SECTOR_SIZE) {
        printf("Error reading sector %u\n", sector);
        perror("fread");
        return Failure;
    }
    return Success;
}

success readSectors(uint32_t sector, void * buffer, uint32_t count) {
    if (fseek(volume, sector * SECTOR_SIZE, SEEK_SET) != 0) {
        perror("fseek (readSectors)");
        return Failure;
    }
    if (fread(buffer, 1, SECTOR_SIZE * count, volume) != SECTOR_SIZE * count) {
        perror("fread (readSectors)");
        return Failure;
    }
    return Success;
}

void readCluster(uint32_t clusterNumber, uint8_t * buffer) {
    uint32_t firstSector = FIRST_DATA_SECTOR + (clusterNumber - 2) * SECTORS_PER_CLUSTER;
    for (uint32_t sector = 0; sector < SECTORS_PER_CLUSTER; ++sector) {
        readSector(firstSector + sector, buffer + sector * SECTOR_SIZE);
    }
}

uint32_t getDotDotCluster(uint32_t cluster) {
    uint8_t buffer[CLUSTER_SIZE];
    readCluster(cluster, buffer);

    for (int i = 0; i < CLUSTER_SIZE; i += ENTRY_SIZE) {
        if (buffer[i] == '.' && buffer[i + 1] == '.') {
            uint16_t high = *(uint16_t *)&buffer[i + 20];
            uint16_t low  = *(uint16_t *)&buffer[i + 26];
            return ((uint32_t)high << 16) | low;
        }
    }
    return 0;  // Not found
}

const char * findNameByCluster(uint32_t parentCluster, uint32_t targetCluster) {
    static char name[12];
    uint8_t buffer[CLUSTER_SIZE];
    readCluster(parentCluster, buffer);

    for (int i = 0; i < CLUSTER_SIZE; i += ENTRY_SIZE) {
        if (buffer[i] == 0x00 || buffer[i] == 0xE5) continue;
        if ((buffer[i + 11] & 0x10) == 0) continue;  // Not a directory

        uint16_t high = *(uint16_t *)&buffer[i + 20];
        uint16_t low  = *(uint16_t *)&buffer[i + 26];
        uint32_t cluster = ((uint32_t)high << 16) | low;

        if (cluster == targetCluster) {
            memcpy(name, &buffer[i], 11);
            name[11] = '\0';

            for (int j = 10; j >= 0; --j) {
                if (name[j] == ' ') name[j] = '\0';
                else break;
            }
            return name;
        }
    }
    return NULL;
}

void buildPathToRoot(uint32_t currentCluster, char * upPath) {
    char temp[MAX_PATH] = "";
    upPath[0] = '\0';

    while (currentCluster != ROOT_CLUSTER) { 
        uint32_t parentCluster = getDotDotCluster(currentCluster);
        const char * name = findNameByCluster(parentCluster, currentCluster);
        if (!name) break;

        char segment[64];
        snprintf(segment, sizeof(segment), "/%s", name);
        strcat(segment, temp);
        strcpy(temp, segment);

        currentCluster = parentCluster;
    }

    // root folder
    if (strlen(temp) == 0) strcpy(upPath, "/");
    else toLowerRegister(temp, upPath);
}

uint32_t findClusterByFullPath(const char * inputPath, uint32_t currentCluster) {
    if (!inputPath || strlen(inputPath) == 0) {
        puts("No path provided");
        return 0;
    }
    boolean absPath = False;
    if (inputPath[0] == '/') absPath = True;
    if (enforceAbsolutePath && !absPath) {
        puts("\tAbsolute path is required in this configuration of FAT32 emulator \x1b[34mxkubpise\x1b[0m\n\tUse -p option to allow relative paths when launching emulator");
        return 0;
    }
    for (size_t c = 0; c < strlen(inputPath); ++c) {
        if (!isValidShortChar(inputPath[c], True, True)) {
            puts("Invalid character(s) in path");
            return 0;
        }
    }
    char path[MAX_PATH] = {0};
    strcpy(path, inputPath);
    char * token = strtok(path, "/");
    uint32_t iCluster = currentCluster;
    if (absPath) iCluster = ROOT_CLUSTER;

    while (token) {
        uint32_t nextCluster = findSubdirectoryCluster(token, iCluster);
        if (nextCluster == 0) {
            printf("Directory \033[31m%s\033[0m from the path %s not found\n", token, inputPath);
            return 0;
        }
        iCluster = nextCluster;
        token = strtok(NULL, "/");
    }
    return iCluster;
}

uint32_t findSubdirectoryCluster(const char * inputName, uint32_t cluster) {
    char formattedName[FILE_AND_EXT_RAW_LENGTH];
    memset(formattedName, ' ', FILE_AND_EXT_RAW_LENGTH);
    if (strcmp(inputName, ".") == 0) return cluster;
    if (strcmp(inputName, "..") == 0) return getDotDotCluster(cluster);
    if (strlen(inputName) > FILE_NAME_MAX_LENGTH) {
        printf("Invalid length for folder name: %s\n", inputName);
        return 0;
    }
    for (size_t c = 0; c < FILE_NAME_MAX_LENGTH; ++c) {
        if (inputName[c] == '.') return 0; // Invalid folder name, no extension allowed
        if (inputName[c] == '\0' || inputName[c] == '/') break;
        formattedName[c] = toupper(inputName[c]);
    }

    uint8_t buffer[SECTOR_SIZE];

    for (uint32_t sector = 0; sector < SECTORS_PER_CLUSTER; ++sector) {
        uint32_t baseSector = ROOT_DIR_SECTOR + (cluster - 2) * SECTORS_PER_CLUSTER + sector;
        fseek(volume, (baseSector + sector) * SECTOR_SIZE, SEEK_SET);
        int ret = fread(buffer, 1, SECTOR_SIZE, volume);
        if (!ret) continue;

        for (int offset = 0; offset < SECTOR_SIZE; offset += ENTRY_SIZE) {
            uint8_t * entry = buffer + offset;

            if (entry[0] == 0x00) return 0;         // End of directory
            if (entry[0] == 0xE5) continue;         // Deleted entry
            if (!(entry[11] & 0x10)) continue;      // Not a directory

            if (memcmp(entry, formattedName, 11) == 0) {
                uint16_t high = *(uint16_t *)(entry + 20);
                uint16_t low  = *(uint16_t *)(entry + 26);
                return (high << 16) | low; 
            }
        }
    }
    return 0; // Not found
}


int collectNamesInCluster(int cluster) {
    if (cluster < 2 || cluster >= N_CLUSTERS) {
        printf("Invalid cluster number: %d\n", cluster);
        return 0;
    }
    int count = 0;

    uint32_t baseSector = ROOT_DIR_SECTOR + (cluster - 2) * SECTORS_PER_CLUSTER;
    unsigned char buffer[SECTOR_SIZE];

    for (int sector = 0; sector < SECTORS_PER_CLUSTER; ++sector) {
        fseek(volume, (baseSector + sector) * SECTOR_SIZE, SEEK_SET);
        int ret = fread(buffer, 1, SECTOR_SIZE, volume);
        if (ret != SECTOR_SIZE) {
            printf("Failed to read sector %u\nRead only %d bytes\n", baseSector + sector, ret);
            continue;
        }

        for (int i = 0; i < SECTOR_SIZE; i += ENTRY_SIZE) {
            unsigned char * entry = &buffer[i];

            if (entry[0] == 0x00) break; // No more entries
            if (entry[0] == 0xE5) continue; // Deleted entry
            if ((entry[11] & 0x0F) == 0x0F) continue; // Long File Name entry, skip for now
            extractNameToBuffer(entry, localFilesAndFolders[count++]);
        }
    }
    qsort(localFilesAndFolders, count, FULL_FILE_STRING_SIZE, cmpLocalNames);
    return count;
}

void initializeDotEntries(uint32_t cluster, uint32_t parentCluster) {
    unsigned char buffer[SECTOR_SIZE];
    memset(buffer, 0, SECTOR_SIZE);

    // Entry for '.'
    memset(buffer, ' ', 11);
    strncpy((char *)buffer, ".", 1);
    buffer[11] = 0x10; // Directory attribute
    buffer[26] = cluster & 0xFF;
    buffer[27] = (cluster >> 8) & 0xFF;
    buffer[20] = (cluster >> 16) & 0xFF;
    buffer[21] = (cluster >> 24) & 0xFF;

    // Entry for '..'
    memset(buffer + 32, ' ', 11);
    strncpy((char *)(buffer + 32), "..", 2);
    buffer[32 + 11] = 0x10; // Directory attribute
    buffer[32 + 26] = parentCluster & 0xFF;
    buffer[32 + 27] = (parentCluster >> 8) & 0xFF;
    buffer[32 + 20] = (parentCluster >> 16) & 0xFF;
    buffer[32 + 21] = (parentCluster >> 24) & 0xFF;

    // Write to first sector of the new cluster
    uint32_t sector = ROOT_DIR_SECTOR + (cluster - 2) * SECTORS_PER_CLUSTER;
    fseek(volume, sector * SECTOR_SIZE, SEEK_SET);
    fwrite(buffer, 1, SECTOR_SIZE, volume);
    if (ferror(volume)) {
        printf("Error writing dot entries to sector %u for cluster %u\n", sector, cluster);
    }
    fflush(volume);
}

uint32_t findFreeCluster() {
    uint8_t fatSector[SECTOR_SIZE];
    for (uint32_t cluster = ROOT_CLUSTER; cluster < N_CLUSTERS; ++cluster) { 
        uint32_t fatSectorIndex = cluster / (SECTOR_SIZE / FAT_ENTRY_SIZE);
        uint32_t offsetInSector = (cluster % (SECTOR_SIZE / FAT_ENTRY_SIZE)) * FAT_ENTRY_SIZE;

        // I read sector only once per sector group
        fseek(volume, (N_RESERVED_SECTORS + fatSectorIndex) * SECTOR_SIZE, SEEK_SET);
        fread(fatSector, 1, SECTOR_SIZE, volume);
        if (ferror(volume)) {
            printf("Error reading FAT sector for cluster %u\n", cluster);
            perror("fread");
            return 0;
        }

        uint32_t value = *(uint32_t *)(fatSector + offsetInSector) & 0x0FFFFFFF; // mask to 28-bit
        if (value == 0x00000000) return cluster;
    }
    return 0; // No free cluster found
}

success createNewObject(const char * objectName, int firstCluster, int parentCluster, IsFolder isFolder) {
    size_t nameLen = strlen(objectName);
    if (isFolder) {
        if (!objectName || nameLen == 0 || nameLen > FILE_NAME_MAX_LENGTH) {
            printf("Invalid folder name: %s\n", objectName);
            return Failure;
        }
        if (firstCluster < 2 || firstCluster >= N_CLUSTERS) {
            printf("Invalid cluster number: %d\n", firstCluster);
            return Failure;
        }
    } else {
        if (!objectName || nameLen == 0 || nameLen > FILE_AND_EXT_RAW_LENGTH + 1) {
            printf("Invalid file name: %s\n", objectName);
            return Failure;
        }
        firstCluster = 0;
    }

    if (parentCluster < 2 || parentCluster >= N_CLUSTERS) {
        printf("Invalid parent cluster number: %d\n", parentCluster);
        return Failure;
    }

    int freeEntryIndex = findFirstFreeEntry(parentCluster);
    if (freeEntryIndex < 0) {
        printf("No free entries available in cluster %d\n", parentCluster);
        return Failure;
    }

    uint32_t sector = ROOT_DIR_SECTOR + (parentCluster - 2) * SECTORS_PER_CLUSTER;
    unsigned char buffer[SECTOR_SIZE];
    fseek(volume, sector * SECTOR_SIZE, SEEK_SET);
    size_t readBytes = fread(buffer, 1, SECTOR_SIZE, volume);
    if (readBytes != SECTOR_SIZE) {
        printf("Failed to read sector %u for cluster %d\n", sector, parentCluster);
        return Failure;
    }
    unsigned char * entryName = buffer + freeEntryIndex * ENTRY_SIZE;
    memset(entryName, 0, ENTRY_SIZE); // Clear the entry
    if (isFolder) {
        buffer[freeEntryIndex * ENTRY_SIZE + FILE_AND_EXT_RAW_LENGTH] = 0x10; // Directory attribute
        memset(entryName, 0x20, FILE_AND_EXT_RAW_LENGTH);
        for (size_t i = 0; i < nameLen; ++i) {
            char c = objectName[i];
            if (c >= 'a' && c <= 'z') c -= 32;  // Uppercase
            entryName[i] = c;
        }
    } else {
        formatShortName(objectName, buffer + freeEntryIndex * ENTRY_SIZE);
        buffer[freeEntryIndex * ENTRY_SIZE + FILE_AND_EXT_RAW_LENGTH] = 0x20; // Regular file attribute
    }
    buffer[freeEntryIndex * ENTRY_SIZE + 26] = firstCluster & 0xFF;
    buffer[freeEntryIndex * ENTRY_SIZE + 27] = (firstCluster >> 8) & 0xFF;
    buffer[freeEntryIndex * ENTRY_SIZE + 20] = (firstCluster >> 16) & 0xFF;
    buffer[freeEntryIndex * ENTRY_SIZE + 21] = (firstCluster >> 24) & 0xFF;

    fseek(volume, sector * SECTOR_SIZE, SEEK_SET);
    size_t writtenBytes = fwrite(buffer, 1, SECTOR_SIZE, volume);
    if (writtenBytes != SECTOR_SIZE) {
        printf("Failed to write to sector %u for cluster %d, number of bytes written %zu\n", sector, parentCluster, writtenBytes);
        perror("fwrite");
        return Failure;
    }
    
    if (isFolder) {
        uint32_t fatOffset = firstCluster * FAT_ENTRY_SIZE;
        uint32_t fatSector = N_RESERVED_SECTORS + (fatOffset / SECTOR_SIZE);
        uint32_t fatSectorOffset = fatOffset % SECTOR_SIZE;

        // I read the sector containing the FAT entry
        unsigned char fatBuffer[SECTOR_SIZE];
        fseek(volume, fatSector * SECTOR_SIZE, SEEK_SET);
        if (fread(fatBuffer, 1, SECTOR_SIZE, volume) != SECTOR_SIZE) {
            printf("Failed to read FAT sector %u\n", fatSector);
            return Failure;
        }

        // I update the FAT entry to mark the cluster as end-of-chain
        fatBuffer[fatSectorOffset + 0] = 0xFF;
        fatBuffer[fatSectorOffset + 1] = 0xFF;
        fatBuffer[fatSectorOffset + 2] = 0xFF;
        fatBuffer[fatSectorOffset + 3] = 0x0F;

        // I write the modified FAT sector back
        fseek(volume, fatSector * SECTOR_SIZE, SEEK_SET);
        if (fwrite(fatBuffer, 1, SECTOR_SIZE, volume) != SECTOR_SIZE) {
            printf("Failed to write updated FAT sector %u\n", fatSector);
            return Failure;
        }
        initializeDotEntries(firstCluster, parentCluster);
    }
    fflush(volume);
    return Success;
}

int findFirstFreeEntry(int cluster) {
    if (cluster < 2 || cluster >= N_CLUSTERS) {
        printf("Invalid cluster number: %d\n", cluster);
        return -1;
    }

    uint32_t sector = ROOT_DIR_SECTOR + (cluster - 2) * SECTORS_PER_CLUSTER;
    unsigned char buffer[SECTOR_SIZE];
    fseek(volume, sector * SECTOR_SIZE, SEEK_SET);
    size_t readBytes = fread(buffer, 1, SECTOR_SIZE, volume);
    if (readBytes != SECTOR_SIZE) {
        printf("Failed to read sector %u for cluster %d\n", sector, cluster);
        return -1;
    }

    for (int i = 0; i < SECTOR_SIZE; i += 32)
        if (buffer[i] == 0x00 || buffer[i] == 0xE5) // Free entry or deleted entry
            return i / 32; // Return the index of the first free entry
    
    printf("No free entries found in cluster %d\n", cluster);
    return -1; // No free entries found
}

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

IsFormatted isValidFAT32xkubpise(const char * filename) {
    IsFormatted issues = notFormatted;
    boolean anyErrors = False;
    volume = fopen(filename, "r+b");
    if (!volume) {
        appendToFAT32ReadingErrors("Error opening the volume\n");
        return badSize;
    } else {
        fseek(volume, 0, SEEK_END);
        long size = ftell(volume);
        fseek(volume, 0, SEEK_SET);
        if (size != TOTAL_SIZE) {
            appendToFAT32ReadingErrors("Volume doesn't have mandatory size of 20 MB\n\tIts size is %ld bytes\n", size);
            issues = badSize;
        }
        if (size < SECTOR_SIZE) {
            appendToFAT32ReadingErrors("Volume is even smaller than %d bytes to host its BIOS Parameter Block\n", SECTOR_SIZE);
            fclose(volume);
            return badSize;
        }
    }

    unsigned char buffer[SECTOR_SIZE];
    size_t bootSectorRead = fread(buffer, 1, SECTOR_SIZE, volume);
    if(bootSectorRead != SECTOR_SIZE) {
        appendToFAT32ReadingErrors("Couldn't read full boot sector\nOnly %ld bytes have been read\n", bootSectorRead);
        fclose(volume);
        return badSize;
    }

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
    if (issues == notFormatted && !anyErrors) return formatted;
    if (issues == notFormatted && anyErrors) return notFormatted;
    return badSize;
}