#include "FS_backend.h"
struct_CurrentData_struct Current;
int FS_Start_Sector;
struct_MBR_Info_struct MBR_Info;
struct_FS_Info_struct FS_Info;
struct_PWD_struct PWD;
char DiskFileName[]="disk.img";
int DEBUG_WRITE_COUNT = 0;
bool echoed = false;
struct_CutedFilename_struct CutedFilename;
int read_sector(const char *DiskFileName,uint8_t * buffer, uint32_t sector)
{
	FILE *fp;
	fp = fopen(DiskFileName, "r+b");
	if (fp == NULL)
	{
		DBG_PRINTF("\nERROR_OPEN_VHD\n");
		return ERROR_OPEN_VHD;
	}
	fseek(fp, sector*SECTOR_SIZE, 0);
	fread(buffer, 1, 512, fp);
	fclose(fp);
	return 0;
}

int write_sector(const char * DiskFileName, uint8_t * buffer, uint32_t sector)
{
	FILE *fp;
	fp = fopen(DiskFileName, "r+b");
	if (fp == NULL)
	{
		DBG_PRINTF("\nERROR_OPEN_VHD\n");
		return ERROR_OPEN_VHD;
	}
	if (sector < FS_Info.ReservedSectors)
	{
		DBG_PRINTF("\nERROR_WRITE_PROTECT %d\n",sector);
		return ERROR_WRITE_PROTECT;
	}
	fseek(fp, sector*SECTOR_SIZE, 0);
	fwrite(buffer, 1, 512, fp);
	DBG_PRINTF("\nFS_wrote_sector%d\n",sector);
	DEBUG_WRITE_COUNT++;
	Current.SectorFlags = CURRENT_FLAG_CLEAN;
	fclose(fp);
	return 0;
}

int MBR_read(uint8_t * buffer)
{
	DBG_PRINTF("\nStart reading MBR\n");
	uint32_t ErrorLevel = 0;
	ErrorLevel|=read_sector(DiskFileName, Current.buffer, 0);
	struct_MBR_struct *MBR;
	MBR = (struct_MBR_struct *)Current.buffer;
	MBR_Info.Active = (MBR->MBR_PartitionTable->MBR_BootIndicator) ?1 : 0;
	MBR_Info.FS_Type = (MBR->MBR_PartitionTable->MBR_PartionType == 0x0c) ? "FAT32" : "UNKNOWN";
	MBR_Info.ReservedSector = MBR->MBR_PartitionTable->MBR_SectorsPreceding;
	FS_Start_Sector = MBR_Info.ReservedSector;
	return ErrorLevel;
}

int BPB_read(uint8_t * buffer)
{
	DBG_PRINTF("\nStart reading BPB\n");
	uint32_t ErrorLevel = 0;
	ErrorLevel |= FS_read_sector(Current.buffer, 0);
	struct_BPB_struct *BPB;
	BPB = (struct_BPB_struct *)Current.buffer;
	if (BPB->BytesPerSector != 512)
	{
		DBG_PRINTF("\nStart reading BPB(!=512)\n");
		ErrorLevel |= ERROR_SECTOR_SIZE;
	}
	strncpy(FS_Info.Label,(char*) BPB->BS_VolumeLabel, 11);
	FS_Info.FatSize = BPB->FATSize;
	FS_Info.RootDirSectors = 0;
	FS_Info.TotalSectors =BPB->TotalSectors32;
	FS_Info.DataSectors = FS_Info.TotalSectors - (BPB->ReservedSectorCount + (BPB->NumFATs * FS_Info.FatSize) + FS_Info.RootDirSectors);
	FS_Info.SectorsPerCluster = BPB->SectorsPerCluster;
	FS_Info.DataClusters = FS_Info.DataSectors / FS_Info.SectorsPerCluster;
	FS_Info.ReservedSectors = BPB->ReservedSectorCount;
	FS_Info.DataStartSector = BPB->ReservedSectorCount + (BPB->NumFATs * FS_Info.FatSize) + FS_Info.RootDirSectors;
	return ErrorLevel;
}

int FS_read_sector(uint8_t * buffer, uint32_t sector)
{
	uint32_t ErrorLevel = 0;
	ErrorLevel = read_sector(DiskFileName, Current.buffer, sector + FS_Start_Sector);
	return ErrorLevel;
}

int FS_fetch(uint32_t sector)
{
	uint32_t ErrorLevel = 0;
	if (Current.CurrentSector == sector)
	{
		DBG_PRINTF("\nSECTOR_ALREADY_IN_MEMORY\n");
		ErrorLevel|= SECTOR_ALREADY_IN_MEMORY;
	}
	else
	{
		DBG_PRINTF("\nFS_read_sector %d\n", sector);
		ErrorLevel |= FS_read_sector(Current.buffer, sector);
	}
	if (Current.SectorFlags & CURRENT_FLAG_DIRTY)
	{
		DBG_PRINTF("\nCURRENT_FLAG_DIRTY\n");
		ErrorLevel |= FS_fsync();
	}
	return ErrorLevel;
}

int FS_fsync()
{
	uint32_t ErrorLevel = 0;
	ErrorLevel = write_sector(DiskFileName, Current.buffer, FS_Start_Sector+Current.CurrentSector);
	return ErrorLevel;
}

int FS_ffsync(FileInfo * fp)
{
	if (Current.SectorFlags & ENRTRY_FLAG_DIRTY)
		FS_fsync();
	fp->flags &= ~ENRTRY_FLAG_DIRTY;
	return 0;
}

uint32_t FS_get_FAT_entry(uint32_t cluster)
{
	uint32_t offset = 4 * cluster;
	uint32_t sector = FS_Info.ReservedSectors + (offset/512);
	FS_fetch(sector);
	uint32_t *FAT_entry = ((uint32_t *) &(Current.buffer[offset % 512]));
	DBG_PRINTF("\nGot FAT entry of cluster %d(%x)\n", cluster, *FAT_entry);
	return *FAT_entry;
}

int FS_set_FAT_entry(uint32_t cluster, uint32_t value)
{
	uint32_t ErrorLevel = 0;
	uint32_t offset = 4 * cluster;
	uint32_t sector = FS_Info.ReservedSectors + (offset / 512);
	ErrorLevel |= FS_fetch(sector);
	uint32_t *FAT_entry = ((uint32_t *) &(Current.buffer[offset % 512]));
	DBG_PRINTF("\nGot FAT entry of cluster %d(%x)\n", cluster, *FAT_entry);
	if (*FAT_entry != value)
	{
		Current.SectorFlags &= CURRENT_FLAG_DIRTY;
		*FAT_entry = value;
		DBG_PRINTF("\nWrote FAT entry of cluster %d(%x)\n", cluster, value);
	}
	return ErrorLevel;
}

std::list<struct_FileInfo_struct> FS_dir(uint32_t cluster)
{
	std::list<struct_FileInfo_struct> dir_content;
	int offset = 0;
	std::string LongFileNameBuffer;
	for (; offset < FS_Info.SectorsPerCluster*SECTOR_SIZE; offset += 0x20)
	{

		DirectoryEntry_struct *CurrentOneFileInfo = (DirectoryEntry_struct *)&Current.buffer[offset];
		struct_FileInfo_struct *one_file_info;
		one_file_info=FS_read_one_file_info(CurrentOneFileInfo, cluster);
		if ((one_file_info->attributes & 0x0f) == 0x0f)
		{
			std::string LongFileNamePart = one_file_info->LongFilename;
			LongFileNamePart.append(LongFileNameBuffer);
			LongFileNameBuffer = LongFileNamePart;
		}
		else
		{
			if (CurrentOneFileInfo->filename[0] == 0xE5 && echoed == false)
			{
				std::cout << one_file_info->filename;
				printf(" is Deleted And now Recoveried\n" );
				((DirectoryEntry_struct *)&Current.buffer[offset])->filename[0] = 'H';
				Current.SectorFlags = CURRENT_FLAG_DIRTY;
				FS_fsync();
			}
			strcpy(one_file_info->LongFilename,LongFileNameBuffer.c_str());
			dir_content.push_back(*one_file_info);
			LongFileNameBuffer.clear();

		}
	}
	return dir_content;
}


struct_FileInfo_struct *FS_read_one_file_info(DirectoryEntry_struct * CurrentOneFileInfo,uint32_t cluster)
{
	if ((CurrentOneFileInfo->attributes&ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME)
	{
		struct_LongFileName_struct *LongFileName = new struct_LongFileName_struct;
		LongFileName = (struct_LongFileName_struct*)CurrentOneFileInfo;
		struct_FileInfo_struct one_file_info;
		strcpy(one_file_info.LongFilename, FS_format_file_name(CurrentOneFileInfo));
		one_file_info.attributes = ATTR_LONG_NAME;
		one_file_info.checksum = LongFileName->checksum;
		struct_FileInfo_struct *p_one_file_info = new struct_FileInfo_struct;
		*p_one_file_info = one_file_info;
		return p_one_file_info;
	}
	else
	{
		struct_FileInfo_struct one_file_info;
		strcpy(one_file_info.filename, "");
		strcpy(one_file_info.filename, FS_format_file_name(CurrentOneFileInfo));
		one_file_info.attributes = CurrentOneFileInfo->attributes;
		one_file_info.ParentStartCluster = cluster;
		one_file_info.CurrentByte = 0;
		one_file_info.StartCluster = one_file_info.CurrentCluster = (((uint32_t)CurrentOneFileInfo->ClusterLow) << 16) | CurrentOneFileInfo->FirstCluster;
		one_file_info.flags = 0;
		one_file_info.mode = ENRTRY_MODE_WRITE;
		one_file_info.pos = 0;
		one_file_info.FileSize = CurrentOneFileInfo->FileSize;
		if (one_file_info.attributes & ATTR_DIRECTORY == 1)
		{
			one_file_info.FileSize = SECTOR_SIZE*FS_Info.SectorsPerCluster;
		}
		one_file_info.CurrentClusterOffset = 0;
		struct_FileInfo_struct *p_one_file_info = new struct_FileInfo_struct;
		*p_one_file_info = one_file_info;
		return p_one_file_info;
	}
}


uint32_t FS_first_sector(uint32_t cluster)
{
	uint32_t base = FS_Info.DataStartSector;
	uint32_t offset = (cluster - 2)*FS_Info.SectorsPerCluster;
	return base + offset ;
}

int FILE_read_info(char * filename, FileInfo *fp)
{
	int offset = 0;
	while (offset<FS_Info.SectorsPerCluster*SECTOR_SIZE)
	{

	}
	return 0;
}



int FS_intial()
{
	uint32_t ErrorLevel = 0;
	//ErrorLevel |= MBR_read(Current.buffer);
	//if (MBR_Info.FS_Type != "FAT32") {
	//	DBG_PRINTF("\nERROR_FS_TYPE %x\n",MBR_Info.FS_Type.c_str());
	//	ErrorLevel |= ERROR_FS_TYPE;
	//}
	MBR_Info.Active = 0;
	MBR_Info.FS_Type = "FAT32";
	MBR_Info.ReservedSector = 0;
	MBR_Info.StartCylinder = MBR_Info.StartHead = MBR_Info.StartHead = MBR_Info.StartSector = 0;
	ErrorLevel |= BPB_read(Current.buffer);
	FS_Start_Sector = 0;
	DBG_PRINTF("\nFS_intial over, ErrorLevel %x\n", ErrorLevel);
	strcpy(PWD.DirectoryName, "/");
	PWD.cluster = 2;
	return ErrorLevel;
}
int FS_Cut_Filename(char*filename, int n) {
	if (n == 0)
		return 0;
	int position = 0;
	while (filename[position]!='/' && filename[position]!='\0')
	{
		position++;
	}
	char temp[255] = { 0 };
	strncpy(temp, filename, position);
	CutedFilename.CutedFilename[CutedFilename.used] = temp;
	CutedFilename.used += 1;
	FS_Cut_Filename(filename + position+1, n - position -1 );
}

FileInfo * FS_fopen(char * filename, const char * mode)
{
	int ErrorLevel = 0;
	int cluster = 0;
	FileInfo *fp = new FileInfo;
	int n = strlen(filename)+1;
	if (filename[0] == '/')
	{
		cluster = 2;
		if (filename[1] == '\0')
		{
			fp->CurrentCluster = 2;
			fp->StartCluster = 2;
			fp->ParentStartCluster = 0xffffffff;
			fp->CurrentClusterOffset = 0;
			fp->CurrentSector = 0;
			fp->CurrentByte = 0;
			fp->attributes = ATTR_DIRECTORY;
			fp->pos = 0;
			fp->flags |= ENRTRY_FLAG_ROOT;
			fp->FileSize = 0xffffffff;
			fp->mode = ENRTRY_MODE_READ | ENRTRY_MODE_WRITE | ENRTRY_MODE_OVERWRITE;
			return fp;
		}
		else
		{
			CutedFilename.used =0;
			FS_Cut_Filename(filename + 1, n - 1);
			for (int i = 0; i < CutedFilename.used; i++)
			{
				fp = FS_find_file(cluster, CutedFilename.CutedFilename[i].c_str());
				cluster = fp->StartCluster;
			}
		}
	}
	else
	{
		ErrorLevel |= ERROR_ILLEGAL_PATH;
	}
	return fp;
}

int FS_fclose(FileInfo * fp)
{
	FS_ffsync(fp);
	free (fp);
	return 0;
}
char *FS_format_file_name(DirectoryEntry_struct *entry) {
	int i, j;
	uint8_t *entryname = entry->filename;
	uint8_t *formated_file_name=new uint8_t [16];
	if (entry->attributes != 0x0f) {
		j = 0;
		for (i = 0; i<8; i++) {
			if (entryname[i] != ' ') {
				formated_file_name[j++] = entryname[i];
			}
		}
		formated_file_name[j++] = '.';
		for (i = 8; i<11; i++) {
			if (entryname[i] != ' ') {
				formated_file_name[j++] = entryname[i];
			}
		}
		formated_file_name[j++] = '\x00';
	}
	else {
		struct_LongFileName_struct *LongEntry = (struct_LongFileName_struct*)entry;
		j = 0;
		for (i = 0; i<5; i++) {
			formated_file_name[j++] = (uint8_t)LongEntry->name_1[i];
		}
		for (i = 0; i<6; i++) {
			formated_file_name[j++] = (uint8_t)LongEntry->name_2[i];
		}
		for (i = 0; i<2; i++) {
			formated_file_name[j++] = (uint8_t)LongEntry->name_3[i];
		}
		formated_file_name[j++] = '\x00';
	}
	
	return (char *)formated_file_name;
}
FileInfo *FS_find_file(uint32_t cluster,const char* filename)
{
	uint32_t FAT_content = FS_get_FAT_entry(cluster);
	if (FAT_content >= FAT_MASK_EOC) {
		FS_read_sector(Current.buffer, (uint32_t)((FS_Info.DataStartSector+(4 * cluster) / 512)));
		////////////////////////////////////////////
		std::list<FileInfo> dir_content = FS_dir(cluster);
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

		for (dir_iterator = dir_content.begin(); dir_iterator != dir_content.end(); ++dir_iterator)
		{
			int n = sizeof(filename);
			struct_FileInfo_struct *fp = &(*dir_iterator);
			std::string filename1 = fp->filename;
			if ((strncasecmp(filename1.c_str(), filename,n) == 0))
			{
				struct_FileInfo_struct *fp_return = new struct_FileInfo_struct;
				*fp_return = *fp;
				return fp_return;
			}
		}
		return NULL;
	}
	else if(FAT_content <2)
	{
		return NULL;
	}
	else
	{
		FS_read_sector(Current.buffer, (uint32_t)((FS_Info.ReservedSectors + (4 * cluster) / 512)));
		std::list<FileInfo> dir_content = FS_dir(cluster);
		if (FAT_content == FAT_MASK_EOC) {
			FS_read_sector(Current.buffer, (uint32_t)((FS_Info.ReservedSectors + (4 * cluster) / 512)));
			std::list<FileInfo> dir_content = FS_dir(cluster);
			std::list<FileInfo>::iterator dir_iterator;
			for (dir_iterator = dir_content.begin(); dir_iterator != dir_content.end(); ++dir_iterator)
			{
				struct_FileInfo_struct *fp = &(*dir_iterator);
				if ((FS_compare_filename(fp, (uint8_t*)filename) == 1))
				{
					return fp;
				}
			}
			return FS_find_file(FAT_content,filename);
		}

	}
}
bool FS_compare_filename_raw_entry(DirectoryEntry_struct *entry, uint8_t *name) { 
	char *formated_file_name = FS_format_file_name(entry);
	bool FlagFind= !(strcasecmp(formated_file_name, (char *)&name));
	return FlagFind;
}

int FS_fread(uint8_t * dest, int size, FileInfo * fp) {
	uint32_t sector;
	for (; size > 0; size--) 
	{
		sector = (fp->CurrentByte / 512)+FS_first_sector(fp->CurrentCluster) ;
		FS_fetch(sector);
		*dest++ = Current.buffer[fp->CurrentByte % 512];
		if (fp->attributes & ATTR_DIRECTORY)
		{
			if (FS_fseek(fp, 0, fp->pos + 1))
			{
				return ERROR_SEEK_FILE;
			}
		}
		else
		{
			if (FS_fseek(fp, 0, fp->pos + 1))
			{
				return ERROR_SEEK_FILE;
			}
		}
	}
	return 0;
}

int FS_fseek(FileInfo * fp, uint32_t offset, int origin) 
{
	long int pos = origin + offset;
	uint32_t cluster_offset;
	uint32_t temp;
	if ((pos > fp->FileSize) && (fp->attributes!= ATTR_DIRECTORY))
	{
		return ERROR_SEEK_FILE;
	}
	else if (fp->FileSize == pos)
	{
		fp->flags |= ENRTRY_FLAG_SIZECHANGED;
		fp->FileSize += 1;
	}
	else
	{
		cluster_offset = pos / (FS_Info.SectorsPerCluster * 512);
		if (cluster_offset != fp->CurrentClusterOffset) 
		{
		temp = cluster_offset;
			if (cluster_offset > fp->CurrentClusterOffset) 
			{
				cluster_offset -= fp->CurrentClusterOffset;
			}
			else 
			{
				fp->CurrentCluster = fp->StartCluster;
			}
			fp->CurrentClusterOffset = temp;
			while (cluster_offset > 0) 
			{
				temp = FS_get_FAT_entry(fp->CurrentCluster);
				if (temp & FAT_MASK_EOC != FAT_MASK_EOC)
				{
					fp->CurrentCluster = temp;
				}
				else 
				{
					temp = FS_FindFreeCluster(fp->CurrentCluster);
					FS_set_FAT_entry(fp->CurrentCluster, temp);
					FS_set_FAT_entry(temp, FAT_MASK_EOC);
					fp->CurrentCluster = temp;
				}
				cluster_offset--;
				if (fp->CurrentCluster >= FAT_MASK_EOC) 
				{
					if (cluster_offset > 0) 
					{
						return ERROR_SEEK_FILE;
					}
				}
			}
		}
		fp->CurrentByte = pos % (FS_Info.SectorsPerCluster * 512);
		fp->pos = pos;
	}

	return 0;
}

uint8_t FS_LongFilename_checksum(const uint8_t * DosName)
{
	int i=11;
	uint8_t checksum = 0;
		while (i)
		{
			checksum = ((checksum & 1) << 7) + (checksum >> 1) + *DosName++;
			i--;
		}
	return checksum;
}

uint32_t FS_FindFreeCluster(uint32_t base)
{
	uint32_t cluster = base;
	uint32_t TotalCluster = FS_Info.TotalSectors / FS_Info.SectorsPerCluster;
	for (; cluster <= TotalCluster; cluster++)
	{
		if ((FS_get_FAT_entry(cluster) & 0x0fffffff) == 0)
			return cluster;
	}
	cluster = 0;
	for (; cluster < base; cluster++) {
		if ((FS_get_FAT_entry(cluster) & 0x0fffffff) == 0)
			return cluster;
	}
	return ERROR_VOLUME_FULL;
}

int FS_compare_filename(FileInfo *fp, uint8_t *name) {
	uint32_t i, j = 0;
	DirectoryEntry_struct Entry;
	uint8_t *compare_name = name;

	FS_fread((uint8_t*)&Entry, sizeof(DirectoryEntry_struct), fp);

	if (Entry.filename[0] == 0x00) return -1;

	if (Entry.attributes != 0x0f) {
		if (1 == FS_compare_filename_raw_entry(&Entry, name)) { 
			FS_fseek(fp, -sizeof(DirectoryEntry_struct), fp->pos);
			return 1;
		}
		else {
			return 0;
		}
	}
	return -1;
}
int FS_fwrite(uint8_t *src, int size, int count, FileInfo *fp) {
	int i, tracking, segsize;
	fp->flags |= ENRTRY_FLAG_DIRTY;
	while (count > 0) {
		i = size;
		fp->flags |= ENRTRY_FLAG_SIZECHANGED;
		while (i > 0) {  
			uint32_t dest_sector = FS_first_sector(fp->CurrentCluster) + (fp->CurrentByte / 512);
			Current.CurrentSector = dest_sector;
			FS_fetch(dest_sector);
			tracking = fp->CurrentByte % 512;
			segsize = (i<512 ? i : 512);
			memcpy(&Current.buffer[tracking], src, segsize);
			Current.SectorFlags |= ENRTRY_FLAG_DIRTY; 
			if (fp->pos + segsize > fp->FileSize)
			{
				fp->FileSize += segsize - (fp->pos % 512);
			}
			if (FS_fseek(fp, 0, fp->pos + segsize)) {
				return -1;
			}
			i -= segsize;
			src += segsize;		
		}
		count--;
	}
	return size - i;
}
struct struct_time_struct
{
	uint16_t CreationDate;
	uint16_t CreationTime;
	uint16_t CreationTimeMs;
} time_struct;
struct_time_struct getCurrentTime()
{

	struct_time_struct f;
	time_t timep;
	time(&timep);
	struct tm *p;
	p = localtime(&timep);	
	f.CreationDate = (uint16_t)(((p->tm_mday) & 0x0f) & (((p->tm_mon) & 0x0f) << 4) & ((((p->tm_year) - 1980) & 0x0f) << 8));
	f.CreationTime = (uint16_t)(((p->tm_sec / 2) & 0x0f) & (((p->tm_min) & 0x0f) << 4) & (((p->tm_hour) & 0x0f) << 10));
	f.CreationTimeMs = 0;
	return f;
}
int FS_touch(char * PathName)
{
	char ParentName[255] = { 0 };
	char FileName[16] = { 0 };
	char FileNameWithoutExtension[8] = {0x20};
	char FileNameExtension[3] = {0x20};
	int FileNameWithOutExtensionLength = 0;
	int FileNameExtensionLength = 0;
	int PathNameLength = 0;
	int FileNameLength = 0;
	while (PathName[PathNameLength]!='\0')
	{
		PathNameLength++;
	}
	for (int i = 1; i <= PathNameLength; i++) {
		if (PathName[PathNameLength - i] != '/')
		{
			FileNameLength++;
		}
		else
		{
			FileNameLength++;
			break;
		}
	}
	strncpy(ParentName, PathName, PathNameLength - FileNameLength);
	strcpy(FileName, PathName + PathNameLength - FileNameLength + 1);
	for (int i = 1; i <= FileNameLength; i++) {
		if (FileName[FileNameLength - i] != '.')
		{
			FileNameExtensionLength++;
		}
		else
		{
			FileNameExtensionLength++;
			break;
		}
	}
	char * p;
	char delim = '.';
	p = strtok(FileName, &delim);
	strncpy(FileNameWithoutExtension, p, FileNameLength - FileNameExtensionLength);
	p = strtok(NULL, &delim);
	strncpy(FileNameExtension, p, FileNameLength - FileNameExtensionLength);
	//strncpy(filenamewithoutextension, filename, filenamelength - filenameextensionlength);
	//strcpy(filenameextension,((char *) filename)+ filenamelength - filenameextensionlength+1);//cdd
	std::string FileNameWithoutExtension_string = FileNameWithoutExtension;
	if (FileNameExtension != "" && FileNameWithoutExtension_string.empty())
	{
		strcpy(FileNameWithoutExtension, FileNameExtension);
		strcpy(FileNameExtension, "");
	}
	for (int i = 0; i < 8; i++)
	{
		if (FileNameWithoutExtension[i] == 0x00)
		{
			FileNameWithoutExtension[i] = 0x20;
		}
	}
	for (int i = 0; i < 3; i++)
	{
		if (FileNameExtension[i] == 0x00)
		{
			FileNameExtension[i] = 0x20;
		}
	}
	strcpy(ParentName, "/");
	FileInfo *fp = new FileInfo;/*= FS_fopen(ParentName, "w")*/;
	FileInfo *fp_ToWrite=new FileInfo;/* = FS_fopen(ParentName, "w")*/;
	fp->CurrentCluster = 2;
	fp->StartCluster = 2;
	fp->ParentStartCluster = 0xffffffff;
	fp->CurrentClusterOffset = 0;
	fp->CurrentSector = 0;
	fp->CurrentByte = 0;
	fp->attributes = ATTR_DIRECTORY;
	fp->pos = 0;
	fp->flags |= ENRTRY_FLAG_ROOT;
	fp->FileSize = 0xffffffff;
	fp->mode = ENRTRY_MODE_READ | ENRTRY_MODE_WRITE | ENRTRY_MODE_OVERWRITE;
	fp_ToWrite->CurrentCluster = 2;
	fp_ToWrite->StartCluster = 2;
	fp_ToWrite->ParentStartCluster = 0xffffffff;
	fp_ToWrite->CurrentClusterOffset = 0;
	fp_ToWrite->CurrentSector = 0;
	fp_ToWrite->CurrentByte = 0;
	fp_ToWrite->attributes = ATTR_DIRECTORY;
	fp_ToWrite->pos = 0;
	fp_ToWrite->flags |= ENRTRY_FLAG_ROOT;
	fp_ToWrite->FileSize = 0xffffffff;
	fp_ToWrite->mode = ENRTRY_MODE_READ | ENRTRY_MODE_WRITE | ENRTRY_MODE_OVERWRITE;
	uint32_t FreeCluster = FS_FindFreeCluster(2);
	Current.CurrentSector = FS_Info.ReservedSectors + (4 *FreeCluster / 512);
	FS_set_FAT_entry(FreeCluster,FAT_MASK_EOC);
	DirectoryEntry_struct BlankFile;
	strncpy((char*)BlankFile.extension, FileNameExtension,8);
	strncpy((char*)BlankFile.filename, FileNameWithoutExtension,8);
	BlankFile.attributes = ATTR_ARCHIVE;
	BlankFile.reserved = 0x08;
	BlankFile.CreationTimeMs = 0x6F;
	BlankFile.CreationTime = 0xAE70;
	BlankFile.CreationDate = 0x4947;
	BlankFile.LastAccessTime = 0x4947;
	BlankFile.ClusterLow = (uint16_t)(FreeCluster>>16);
	BlankFile.ModifiedTime = 0xAE71;
	BlankFile.ModifiedDate = 0x4947;
	BlankFile.FirstCluster = FreeCluster;
	BlankFile.FileSize = 0x00000018;
	FS_fread(Current.buffer, 1, fp);
	uint32_t offset = 0;
	uint16_t BlankEntry[32] = { 0x00 };
	bool isBlank = false;
	for (; (offset < FS_Info.SectorsPerCluster*SECTOR_SIZE) && (!isBlank) ; offset += 0x20)//TODO What to do if Exceed one Cluster
	{
		isBlank = true;
		DirectoryEntry_struct *CurrentOneFileInfo = (DirectoryEntry_struct *)&Current.buffer[offset];
		//struct_FileInfo_struct *one_file_info;
		//one_file_info = FS_read_one_file_info(CurrentOneFileInfo, fp->CurrentCluster);
		for (int i = 0; i < 32; i++)
		{
			if (((uint8_t *)CurrentOneFileInfo)[i] != BlankEntry[i])
			{
				isBlank = false;
				break;
			}
		}
	}
	offset -= 0x20;
	FS_fseek(fp_ToWrite, offset, 0);
	FS_fwrite((uint8_t *)&BlankFile, 1, 32, fp_ToWrite);
	FS_fclose(fp_ToWrite);
	return 0;
}
