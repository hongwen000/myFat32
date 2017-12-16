概要

本程序与16年10月微软俱乐部技术部面试要求相关，在国庆假期实现。

基本实现读写Fat32文件系统所需的全部功能，包括支持长文件名。

面试时经过修改完成了要求的：使用程序读取提供的img格式储存的Fat32分区的数据，实现列出全部文件，恢复被删除的文件，创建新文件并写入内容等多项要求。由于未保存测试所用img文件，目前无法复现。

编译环境

bash on window (uname -a ：Linux 4.4.0-43-Microsoft #1-Microsoft Wed Dec 31 14:42:53 PST 2014 x8664 x8664 x86_64 GNU/Linux)

g++ version 6.2.0 20160901 (Ubuntu 6.2.0-3ubuntu11~16.04)

主要函数

摘录自FS_backend.h

//模拟与磁盘控制器的交互

int read_sector(const char *DiskFileName, uint8t * buffer, uint32t sector);

int write_sector(const char *DiskFileName, uint8t *buffer, uint32t sector);

//提供系统IO函数

int FS_fetch(uint32_t sector); //Fetch a sector

int FS_fsync(); //Write buffer to current sector

FileInfo *FS_fopen( char *filename, const char *mode);

int FS_fclose(FileInfo *fp);

int FS_fread(uint8_t *dest, int size, FileInfo *fp);

int FS_fseek(FileInfo * fp, uint32_t offset, int origin);

int FS_fwrite(uint8_t *src, int size, int count, FileInfo *fp);

int FS_touch(char *PathName);

uint32t FS_get_FAT_entry(uint32t cluster);//Get FAT entry of specified cluster

int FS_set_FAT_entry(uint32t cluster, uint32t value);


