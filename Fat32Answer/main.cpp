#include "FS_backend.h"
#include "FS_frontend.h"
#include <iostream>
using namespace std;
char filename[FS_MAX_PATH];
uint8_t write_buffer[24] = { 'T','h','i','s','M','S','C','S','e','r','v','e','r','d','i','s','k','\0' };
int WriteLength = sizeof(write_buffer);
int main(int argc, char* argv[])
{
	FS_intial();
	//Show_MBR();
	//getchar();
	Show_FS_Info();
	//getchar();
	//Read_Print_File("/hello.txt");
	//getchar();
	//Read_Print_File_Long("/SYSTEM~1.");
	//getchar();
	Read_Print_File_Long("/SYSTEM~1.");
	echoed = true;
	FS_ls();
	//getchar();
	//WriteFile("/towrite.txt", write_buffer, WriteLength);
	Read_Print_File_Long("/");
	cout << "OK" << endl; 
	FS_touch("/towrite.txt");
	std::cout  << "OK" << endl;	
	WriteFile("/towrite.txt", write_buffer, WriteLength);
	//Read_Print_File("/towrite.txt");
	//std::cout  <<  "--------------------Creating new Blank File--------------------" << endl;
	//std::cout  <<  "------------------Creating new Blank File END------------------" << endl;
}
