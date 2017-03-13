//#define DEBUG
#ifndef _FS_BACKEND_
#define _FS_BACKEND_
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <list>
#include <iomanip>
#include <time.h>
#include <iostream>

#endif // !_FS_BACKEND_
#ifdef DEBUG
#define DBG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DBG_PRINTF(...)
#endif // DEBUG

#define FS_MAX_PATH 255
#define FAT_MASK_EOC 0x0FFFFFFF 
#define SECTOR_SIZE 512
#define SECTOR_ALREADY_IN_MEMORY 0x00
#define NO_ERROR 0x00
#define ERROR_OPEN_VHD 0x01
#define ERROR_FS_TYPE 0x02
#define ERROR_SECTOR_SIZE 0x04
#define ERROR_SEEK_FILE 0x08
#define ERROR_ILLEGAL_PATH 0x0F
#define ERROR_READ_FAT 0x1F
#define ERROR_WRITE_PROTECT 0x2F
#define ERROR_VOLUME_FULL 0x4F
#define ERROR_UNKOWN 0xFFFFFFFF
#define CURRENT_FLAG_CLEAN 0x00
#define CURRENT_FLAG_DIRTY 0x01

#define ENRTRY_FLAG_DIRTY 0x01
#define ENRTRY_FLAG_OPEN 0x02
#define ENRTRY_FLAG_SIZECHANGED 0x04
#define ENRTRY_FLAG_ROOT 0x08

#define ENRTRY_MODE_READ 0x01
#define ENRTRY_MODE_WRITE 0x02
#define ENRTRY_MODE_APPEND 0x04
#define ENRTRY_MODE_OVERWRITE 0x08

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_DEVICE 0x40
#define ATTR_UNUSED 0x80
#define ATTR_LONG_NAME (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)
#define ATTR_LONG_NAME_MASK (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID | ATTR_DIRECTORY | ATTR_ARCHIVE)
#define LAST_LONG_ENTRY_MASK 0x40
#define _IO_SKIPWS 01
#define _IO_LEFT 02
#define _IO_RIGHT 04
#define _IO_INTERNAL 010
#define _IO_DEC 020
#define _IO_OCT 040
#define _IO_HEX 0100
#define _IO_SHOWBASE 0200
#define _IO_SHOWPOINT 0400
#define _IO_UPPERCASE 01000
#define _IO_SHOWPOS 02000
#define _IO_SCIENTIFIC 04000
#define _IO_FIXED 010000
#define _IO_UNITBUF 020000
#define _IO_STDIO 040000
#define _IO_DONT_CLOSE 0100000
//Structures


typedef struct struct_MBR_PartitionTable_struct {
	uint8_t MBR_BootIndicator;
	uint8_t MBR_StartHead[3];
	uint8_t MBR_PartionType;
	uint8_t MBR_EndHead[3];
	uint32_t MBR_SectorsPreceding;
	uint32_t MBR_SectorsInPartition;
}__attribute__ ((__packed__)) MBR_PartitionTable_struct;

typedef struct struct_MBR_struct {
	uint8_t MBR_BootRecord[446];
	MBR_PartitionTable_struct MBR_PartitionTable[4];
	uint16_t MBR_Signature;
}__attribute__ ((__packed__));
typedef struct struct_MBR_Info_struct {
	bool Active;
	int StartCylinder;
	int StartHead;
	int StartSector;
	int ReservedSector;
	std::string FS_Type;
};
typedef struct struct_BPB_struct {
	uint8_t     BS_JumpBoot[3];
	uint8_t     BS_OEMName[8];
	uint16_t    BytesPerSector;
	uint8_t     SectorsPerCluster;
	uint16_t    ReservedSectorCount;
	uint8_t     NumFATs;
	uint16_t    RootEntryCount;
	uint16_t    TotalSectors16;
	uint8_t     Media;
	uint16_t    FATSize16;
	uint16_t    SectorsPerTrack;
	uint16_t    NumberOfHeads;
	uint32_t    HiddenSectors;
	uint32_t    TotalSectors32;
	uint32_t    FATSize;
	uint16_t    ExtFlags;
	uint16_t    FSVersion;
	uint32_t    RootCluster;
	uint16_t    FSInfo;
	uint16_t    BkBootSec;
	uint8_t     Reserved[12];
	uint8_t     BS_DriveNumber;
	uint8_t     BS_Reserved1;
	uint8_t     BS_BootSig;
	uint32_t    BS_VolumeID;
	uint8_t     BS_VolumeLabel[11];
	uint8_t     BS_FileSystemType[8];
} __attribute__ ((__packed__))BPB_struct;
typedef struct struct_DirectoryEntry_struct {
	uint8_t filename[8];
	uint8_t extension[3];
	uint8_t attributes;
	uint8_t reserved;
	uint8_t CreationTimeMs;
	uint16_t CreationTime;
	uint16_t CreationDate;
	uint16_t LastAccessTime;
	uint16_t ClusterLow;
	uint16_t ModifiedTime;
	uint16_t ModifiedDate;
	uint16_t FirstCluster;
	uint32_t FileSize;
} __attribute__((__packed__))DirectoryEntry_struct;
typedef struct struct_LongFileName_struct {
	uint8_t sequence_number;
	uint16_t name_1[5]; 
	uint8_t attributes;
	uint8_t reserved;
	uint8_t checksum;
	uint16_t name_2[6];
	uint16_t firstCluster;
	uint16_t name_3[2];
} __attribute__((__packed__))LongFileName_struct;
typedef struct struct_FileInfo_struct {
	uint32_t ParentStartCluster;
	uint32_t StartCluster;
	uint32_t CurrentClusterOffset;
	uint32_t CurrentCluster;
	short CurrentSector;
	short CurrentByte;
	uint32_t pos;
	uint8_t flags;
	uint8_t attributes;
	uint8_t mode;
	uint32_t FileSize;
	char filename[16] = "";
	char LongFilename[255] = "";
	uint8_t checksum;
} FileInfo;
typedef struct struct_FS_Info_struct {
	char Label[11];
	uint8_t SectorsPerCluster;
	uint32_t ReservedSectors;
	uint32_t DataStartSector;
	uint32_t DataSectors;
	uint32_t DataClusters;
	uint32_t TotalSectors;
	uint32_t FatSize;
	uint32_t RootDirSectors;
} FS_Info_struct;
typedef struct struct_CurrentData_struct {
	uint32_t CurrentSector;
	uint8_t SectorFlags;
	uint8_t buffer[512];
} CurrentData_struct;
typedef struct struct_PWD_struct {
	char DirectoryName[FS_MAX_PATH];
	uint32_t cluster;
};
typedef struct struct_CutedFilename_struct {
	std::string CutedFilename[255];
	int used;
}CutedFilename_struct;
//Structures end

//Physics storage interface Functions
int read_sector(const char *DiskFileName, uint8_t * buffer, uint32_t sector);
int write_sector(const char *DiskFileName, uint8_t *buffer, uint32_t sector);
int FS_fetch(uint32_t sector); //Fetch a sector
uint32_t FS_get_FAT_entry(uint32_t cluster);//Get FAT entry of specified cluster
int FS_set_FAT_entry(uint32_t cluster, uint32_t value);

//internal helper Functions
int FS_intial();
int MBR_read(uint8_t *buffer);
int BPB_read(uint8_t *buffer);
int FS_read_sector(uint8_t * buffer, uint32_t sector);//Read a sector from the beginning of FS
int FS_read_entry(uint8_t * buffer, uint32_t sector);
int FS_compare_filename(FileInfo *fp, uint8_t *name);
bool FS_compare_filename_raw_entry(DirectoryEntry_struct *entry, uint8_t *name);
std::list<struct_FileInfo_struct> FS_dir(uint32_t cluster);
struct_FileInfo_struct *FS_read_one_file_info(DirectoryEntry_struct *CurrentOneFileInfo,uint32_t cluster);
char *FS_format_file_name(DirectoryEntry_struct *entry);
FileInfo *FS_find_file(uint32_t cluster, const char* filename);
uint8_t FS_LongFilename_checksum(const uint8_t *DosName);
uint32_t FS_FindFreeCluster(uint32_t base);
//System IO interface Functions
int FS_fsync(); //Write buffer to current sector
int FS_ffsync(FileInfo *fp);
FileInfo *FS_fopen( char *filename, const char *mode);
int FS_fclose(FileInfo *fp);
int FS_fread(uint8_t *dest, int size, FileInfo *fp);
int FS_fseek(FileInfo * fp, uint32_t offset, int origin);
int FS_fwrite(uint8_t *src, int size, int count, FileInfo *fp);
int FS_touch(char *PathName);
//Golbal Variables
extern char DiskFileName[FS_MAX_PATH];
extern struct_CurrentData_struct Current;
extern struct_MBR_Info_struct MBR_Info;
extern struct_FS_Info_struct FS_Info;
extern struct_PWD_struct PWD;
extern struct_CutedFilename_struct CutedFilename;
extern int DEBUG_WRITE_COUNT;
extern bool echoed;