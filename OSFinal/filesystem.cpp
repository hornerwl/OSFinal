#include "disk.h"
#include "diskmanager.h"
#include "partitionmanager.h"
#include "filesystem.h"
#include <time.h>
#include <cstdlib>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h> 
#include <iostream>
#include <sstream>
#include <string>
#include <time.h> 
using namespace std;
map<int, bool> uniqueNums;

FileSystem::FileSystem(DiskManager *dm, char fileSystemName){
	myPM = new PartitionManager(dm, fileSystemName, dm->getPartitionSize(fileSystemName));	

}

int FileSystem::createFile(char *filename, int fnameLen){	
	int offset = 5;
	char buffer[64];
	char dataBuffer[64];
	char fName = filename[fnameLen-1];
	fill_n(dataBuffer, 64,'.');
	fill_n(buffer, 64,'.');

	myPM->readDiskBlock(1, dataBuffer);

	if (filename[0] == '/' && isalpha(fName))
	{
		for (int i = 0; i < 11; i++)
		{
			if(searchForFile(fName))
			{
				cout << "File has already been created" << endl;
				return -1;
			}
		}
		offset = 0;
		int inodeLoc = myPM->getFreeDiskBlock();
		int fileLoc = myPM->getFreeDiskBlock();	
		fill_n(dataBuffer, 64,'.');
		fill_n(buffer, 64,'.');
		buffer[offset] = fName;
		offset++;
		buffer[offset] = 'f';
		offset++;
		sprintf(buffer+offset, "%i", 0);
		offset+=4;
		sprintf(buffer+offset, "%i", fileLoc);
		offset+=4;
		for(int i = 0; i < 2; i++)
		{
			sprintf(buffer+offset, "%i", 0);
			offset+=4;
		}	
		sprintf(buffer+offset, "%i", 0);
		offset+=4;

		//read/write bits
		sprintf(buffer+offset, "%i", 1);
		offset+=1;
		sprintf(buffer+offset, "%i", 1);
		offset+=2;
		
		//size bit
		sprintf(buffer+offset, "%i", 0);


		myPM->writeDiskBlock(inodeLoc, buffer);	
		myPM->readDiskBlock(1, dataBuffer);

		//create entry in root directory
		offset = 5;
		for(int i = 0; i < 11; i++){
			if(dataBuffer[offset] == 'e'){
				dataBuffer[offset] = fName;
				sprintf(dataBuffer+offset+1, "%i", inodeLoc);
				break;
			}
			offset+=5;
		}
		myPM->writeDiskBlock(1, dataBuffer);
		
		globalMap[fName].inode = inodeLoc;
		globalMap[fName].fileLoc = fileLoc;
		globalMap[fName].size = 0;
		return 0;
	}
	else {
		cout << "could not process file: invalid directory" << endl;
		return -3;
	}

	
		
}
int FileSystem::createDirectory(char *dirname, int dnameLen){

}
int FileSystem::lockFile(char *filename, int fnameLen){

}
int FileSystem::unlockFile(char *filename, int fnameLen, int lockId){

}
int FileSystem::deleteFile(char *filename, int fnameLen)
{

}
int FileSystem::deleteDirectory(char *dirname, int dnameLen){

}
int FileSystem::openFile(char *filename, int fnameLen, char mode, int lockId){
	//this is a comment
	char fname = filename[fnameLen-1];
	if (!searchForFile(fname) || filename[0] != '/'){
		return -1;
	}
	else if(mode != 'r' && mode != 'w' && mode != 'm')
	{
		return -2;
	}
	else{
	int process = getUniqueID();
	personMap[process].name = fname;
	personMap[process].mode = mode;
	personMap[process].loc = globalMap[fname].fileLoc;
	cout << "Entry Created " << personMap[process].name << " " << personMap[process].mode << " " << personMap[process].loc << endl;
	return process;
	}
	return -4;

}
int FileSystem::closeFile(int fileDesc){

}
int FileSystem::readFile(int fileDesc, char *data, int len){
	return 0;
}
int FileSystem::writeFile(int fileDesc, char *data, int len){
	char buffer[64];
	fill_n(buffer, 64,'.');
	//cout << len << endl;
	if(personMap[fileDesc].loc == 0){
		return -1;
	}
	if (len < 0) {
		return -2;
	}
	if(personMap[fileDesc].mode == 'w' && personMap[fileDesc].mode == 'm') {
		return -3;
	}

	for(int i = 0; i < len; i++) {
		buffer[i] = data[i];
	}
	//cout << personMap[fileDesc].loc << endl;
	myPM->writeDiskBlock(personMap[fileDesc].loc, buffer);
	return len;
}
int FileSystem::appendFile(int fileDesc, char *data, int len){

}
int FileSystem::seekFile(int fileDesc, int offset, int flag){

}
int FileSystem::renameFile(char *filename1, int fnameLen1, char *filename2, int fnameLen2){

}
int FileSystem::getAttribute(char *filename, int fnameLen /* ... and other parameters as needed */){

}
int setAttribute(char *filename, int fnameLen /* ... and other parameters as needed */){

}
int FileSystem::getUniqueID(){
	int ID;
  	srand (time(0));

  	ID = rand() % 100 + 1;
		
	while (uniqueNums[ID] == true){
  		ID = rand() % 100 + 1;
				
	}
	uniqueNums[ID] = true;
	cout<< "Generated ID: " << ID << endl;
	return ID;
	
}
bool FileSystem::searchForFile(char fileName){
	char buffer[64];
	myPM->readDiskBlock(1, buffer);
	int offset = 5;

	for(int i = 0; i < 11; i++){
		if(buffer[offset] == fileName){
			globalMap[fileName].inode = buffer[offset+1] - '0';
			myPM->readDiskBlock(globalMap[fileName].inode, buffer);
			globalMap[fileName].fileLoc = buffer[6] - '0';
			globalMap[fileName].size = buffer[25] - '0';
			return true;
		}
		offset+=5;
	}
	return false;

}
