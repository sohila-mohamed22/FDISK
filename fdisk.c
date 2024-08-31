#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#pragma pack(push, 1)

typedef struct {
    uint8_t boot_indicator; // Boot indicator (0x80 if bootable, 0x00 otherwise)
    uint8_t start_head;
    uint8_t start_sector;
    uint8_t start_cylinder;
    uint8_t partition_type; // Partition type identifier
    uint8_t end_head;
    uint8_t end_sector;
    uint8_t end_cylinder;
    uint32_t start_sector_abs; // Start sector in LBA
    uint32_t total_sectors;    // Total sectors in the partition
} PartitionEntry;

typedef struct {
    uint8_t boot_code[446];
    PartitionEntry partitions[4];
    uint16_t signature; // Should be 0xAA55
} MBR;

#pragma pack(pop)

// Partition type descriptions (simplified)
const char *get_partition_type(uint8_t type) {
    switch (type) {
        case 0x0B: return "W95 FAT32";
        case 0x0C: return "W95 FAT32 (LBA)";
        case 0x83: return "Linux";
        case 0x05: return "Extended";
        case 0x07: return "HPFS/NTFS/exFAT";
        case 0x82: return "Linux swap / Solaris";
        case 0xEF: return "EFI System";
        case 0xA0: return "BIOS boot";
        default:   return "Unknown";
    }
}

// Print partition information with boot status
void print_partition_info(const char *dev_name, PartitionEntry *partition, int is_bootable) {
    if (partition->partition_type != 0x00) {
        // Calculate the end sector
        uint32_t end_sector = partition->start_sector_abs + partition->total_sectors - 1;

        // Calculate size in human-readable format
        float size_mb = partition->total_sectors * 512.0 / (1024 * 1024);
        float size_gb = size_mb / 1024;
        char size_str[16];

        if (size_gb >= 1.0) {
            snprintf(size_str, sizeof(size_str), "%.1fG", size_gb);
        } else {
            snprintf(size_str, sizeof(size_str), "%.1fM", size_mb);
        }

        // Determine if the partition is bootable
        const char *boot_indicator = is_bootable ? "*" : " ";

        // Print partition information with correct formatting
        printf("%-10s %4s %7u %7u %7u %6s %-18s\n",
               dev_name,
               boot_indicator,
               partition->start_sector_abs,
               end_sector,
               partition->total_sectors,
               size_str,
               get_partition_type(partition->partition_type));
    }
}

// Parse extended partition and print all partitions
void parse_extended_partition(int fd, uint32_t ebr_start, int *partition_index, const char *base_name) {
    uint32_t next_ebr = ebr_start;
    PartitionEntry ebr_partitions[2];
    char dev_name[16];

    while (next_ebr) {
        // Seek to the EBR start sector
        if (lseek(fd, next_ebr * 512, SEEK_SET) == (off_t) -1) {
            perror("lseek");
            return;
        }

        // Read EBR partitions
        if (read(fd, ebr_partitions, sizeof(ebr_partitions)) != sizeof(ebr_partitions)) {
            perror("read");
            return;
        }

        // Process each partition entry in the EBR
        for (int i = 0; i < 2; i++) {
            if (ebr_partitions[i].partition_type != 0x00) {
                snprintf(dev_name, sizeof(dev_name), "%s%d", base_name, (*partition_index)++);
                print_partition_info(dev_name, &ebr_partitions[i], 0);
            }
        }

        // Move to the next EBR if present
        if (ebr_partitions[1].partition_type == 0x05 || ebr_partitions[1].partition_type == 0x0F) {
            next_ebr = ebr_partitions[1].start_sector_abs;
        } else {
            next_ebr = 0;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s /dev/sdX\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *device = argv[1];

    int fd = open(device, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    MBR mbr;
    if (read(fd, &mbr, sizeof(MBR)) != sizeof(MBR)) {
        perror("read");
        close(fd);
        return EXIT_FAILURE;
    }

    if (mbr.signature != 0xAA55) {
        fprintf(stderr, "Invalid MBR signature\n");
        close(fd);
        return EXIT_FAILURE;
    }

    // Print header with correct alignment
    printf("Device       Boot  Start      End    Sectors  Size  Type\n");

    int partition_index = 1;
    char dev_name[16];

    // Print primary partitions
    for (int i = 0; i < 4; i++) {
        if (mbr.partitions[i].partition_type != 0x00) {
            snprintf(dev_name, sizeof(dev_name), "%s%d", device, partition_index);
            int is_bootable = (mbr.partitions[i].boot_indicator == 0x80);
            print_partition_info(dev_name, &mbr.partitions[i], is_bootable);
            partition_index++;
        }
    }

    // Parse and print logical partitions within extended partitions
    for (int i = 0; i < 4; i++) {
        if (mbr.partitions[i].partition_type == 0x05 || mbr.partitions[i].partition_type == 0x0F) {
            parse_extended_partition(fd, mbr.partitions[i].start_sector_abs, &partition_index, device);
        }
    }

    close(fd);
    return EXIT_SUCCESS;
}

