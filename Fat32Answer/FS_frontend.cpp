#include "FS_frontend.h"
#include "FS_backend.h"
#include <iostream>
using namespace std;
void Show_MBR()
{
	std::cout  <<  "-----------------------------MBR-------------------------------" << endl;
	std::cout  <<  "Found Volume" << endl;
	std::cout  <<  "Actvie:" << MBR_Info.Active<<endl;
	std::cout  <<  "Volume Type:" << MBR_Info.FS_Type<<endl;
	std::cout  <<  "-----------------------------MBR END---------------------------" << endl;
}

void Show_FS_Info()
{
	//std::cout  <<  "-----------------------------BPB-------------------------------" << endl;
	//std::cout  << "This Volume Total have # Sectors:" << FS_Info.TotalSectors<<endl;
	//std::cout  << FS_Info.Label << endl;
	printf("\Cluster Size:%d\n", FS_Info.SectorsPerCluster*SECTOR_SIZE);
	for (int i = 0; i < 11; i++)
	{
		printf("%c", FS_Info.Label[i]);
	}
	std::cout  << endl; 
	//std::cout <<  "Reserved Sectors:"<<FS_Info.ReservedSectors<<endl;
	//std::cout  <<  "Fat zone Total have # Sectors:" << FS_Info.FatSize << endl;
	//std::cout <<  "Data zone Start from Cluster:"<<FS_Info.DataStartSector<<endl;
	//std::cout  <<  "Data zone Total have # Sectors:" << FS_Info.DataSectors<<endl;
	//std::cout  <<  "Data zone Total have # Clusters:" << FS_Info.DataClusters<<endl;
	//
	//std::cout  <<  "-----------------------------BPB END---------------------------" << endl;
}

void Read_Print_File(char * filename)
{
	std::cout  <<  "-------------------Open Normal File----------------------------" << endl;
	FileInfo *fp;
	fp = FS_fopen(filename, "r");
	std::cout  <<  "File " << filename << " opened" << endl;
	std::cout  <<  "Parent Directory at Cluster:" << fp->ParentStartCluster << endl;;
	std::cout  <<  "File Start at Cluster:"<<fp->StartCluster<<endl;
	std::cout  <<  "File Start at Cluster:" << fp->CurrentSector << endl;
	std::cout <<  "File size (byte):"<<fp->FileSize<<endl;
	FS_fread(Current.buffer, 1, fp);
	std::cout  <<  "-------------------File Content---------------------" << endl;
	std::cout  << Current.buffer << endl;
	std::cout  <<  "-------------------File Content END------------------" << endl;
	std::cout  <<  "-------------------Open Normal File END------------------------" << endl;
}

void Read_Print_File_Long(char * filename)
{
	//std::cout  <<  "-------------------Open Directory with LFN---------------------" << endl;
	FileInfo *fp;
	fp = FS_fopen(filename, "r");
	//std::cout  <<  "File " << filename << " opened" << endl;
	//std::cout  <<  "Parent Directory at Cluster:" << fp->ParentStartCluster << endl;;
	//std::cout  <<  "File Start at Cluster:" << fp->StartCluster << endl;
	//std::cout  <<  "File Start at Cluster:" << fp->CurrentSector << endl;
	//std::cout  <<  "File size (byte):" << fp->FileSize << endl;
	//std::cout  <<  "-------------------Long File Name ---------------------" << endl;
	//std::cout  << fp->LongFilename<< endl;
	//std::cout  <<  "-------------------Long File Name END------------------" << endl;
	//std::cout  <<  "-----------------Open Directory with LFN END-------------------" << endl;

	
}

void WriteFile(char * filename, uint8_t * write_buffer,int WriteLength)
{
	FileInfo *fp_read, *fp_write, *fp_read2;
	fp_read = FS_fopen("/towrite.txt", "w");
	fp_read2 = FS_fopen("/towrite.txt", "w");
	fp_write = FS_fopen("/towrite.txt", "w");
	FS_fread(Current.buffer, 1, fp_read);
	std::cout  <<  "---------------Original File Content----------------" << endl;
	std::cout  << Current.buffer << endl;
	std::cout  <<  "---------------Original File Content END------------" << endl;
	FS_fwrite(write_buffer, sizeof(uint8_t), WriteLength, fp_write);
	std::cout  <<  "---------------File Content NOW----------------" << endl;
	FS_fread(Current.buffer, 1, fp_read2);
	std::cout  << Current.buffer << endl;
	std::cout  <<  "---------------File Content NOW END------------" << endl;
}

void FS_ls()
{

	std::list<FileInfo> dir_content = FS_dir(2);
	///////////////////////////////////////////
	std::list<FileInfo>::iterator dir_iterator;
	dir_iterator = dir_content.begin();
	while (dir_iterator != dir_content.end())
	{
		char FilenameFirstChar = dir_iterator->filename[0];
		if (FilenameFirstChar == '\000' || (int)FilenameFirstChar == 0x7f)
		{
			dir_content.erase(dir_iterator++);
		}
		else
		{
			++dir_iterator;
		}
	}
	

	std::cout  <<  "-----------------Listing Directionary-------------------" << endl;
	std::cout  <<  "8.3 file name" <<'\t'<< "Long file name" << endl;
	for (dir_iterator = dir_content.begin(); dir_iterator != dir_content.end(); ++dir_iterator)
	{
		struct_FileInfo_struct *fp = &(*dir_iterator);
			std::cout  << setw(13)<< fp->filename << '\t' << fp->LongFilename << endl;
		}
	std::cout  <<  "---------------Listing Directionary END-----------------" << endl;
	}

