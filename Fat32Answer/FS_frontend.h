#ifndef _FS_FRONTEND_
#define _FS_FRONTEND_
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#endif // !_FS_BACKEND_
void Show_MBR();
void Show_FS_Info();
void Read_Print_File(char *filename);
void Read_Print_File_Long(char *filename);
void WriteFile(char *filename, uint8_t *write_buffer,int WriteLength);
void FS_ls();

