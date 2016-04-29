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
map<int, bool> lockIDNums;

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
		globalMap[fName].lockId = 0;
		globalMap[fName].blockCount = 1;
		return 0;
	}
	else {
		//cout << "could not process file: invalid directory" << endl;
		return -3;
	}

	
		
}
int FileSystem::createDirectory(char *dirname, int dnameLen){

}
int FileSystem::lockFile(char *filename, int fnameLen){
	char fname = filename[fnameLen-1];

	if (globalMap.find(fname) == globalMap.end()){
		return -2;
	}
	else if (globalMap[fname].lockId != 0){
		return -1;
	}
	else
	{
		for (map<int,pp>::iterator it = personMap.begin(); it != personMap.end(); ++it )
		{
	    	if (it->second.name == fname)
	        	return -3;
	    }  
		globalMap[fname].lockId = getLockID();
		return globalMap[fname].lockId;
    }
	return -4;
}
int FileSystem::unlockFile(char *filename, int fnameLen, int lockId){
    char name = filename[fnameLen-1];
	if (globalMap.find(name) != globalMap.end() && globalMap[name].lockId != lockId)
	{
		return -1;
	}
	else if(globalMap[name].lockId == lockId){
		globalMap[name].lockId = 0;
		return 0;
	}
	return -2;
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
	else if((globalMap[fname].lockId != 0 && globalMap[fname].lockId != lockId) || (globalMap[fname].lockId == 0 && lockId > 0)){
		return -3;
	}
	else{
		int process = getUniqueID();
		personMap[process].name = fname;
		personMap[process].mode = mode;
		personMap[process].loc = globalMap[fname].fileLoc;
		personMap[process].rwptr = 0;
		//cout << "Entry Created " << personMap[process].name << " " << personMap[process].mode << " " << personMap[process].loc << endl;
		return process;
	}
	return -4;

}
int FileSystem::closeFile(int fileDesc){
	if (personMap.find(fileDesc) != personMap.end()){
		personMap.erase(fileDesc);
		return 0;
	}
	else{
		return -1;

	}
	return -2;

}
int FileSystem::readFile(int fileDesc, char *data, int len){
	return 0;
}
int FileSystem::writeFile(int fileDesc, char *data, int len){
	char buffer[64];
	char databuffer[64];
	myPM->readDiskBlock(personMap[fileDesc].loc, buffer);//get data already in the location

	//check to make sure the map location actually exists
	if(personMap.find(fileDesc) == personMap.end()){
		return -1;
	}
	//check to make sure out length is not negative
	else if (len < 0) {
		return -2;
	}
	//check to make sure we have permission to write
	else if(personMap[fileDesc].mode != 'w' && personMap[fileDesc].mode != 'm') {
		return -3;
	}
	if ((personMap[fileDesc].rwptr + len) < 64)
	{
		for(int i = (personMap[fileDesc].rwptr); i < (personMap[fileDesc].rwptr + len); i++) {
			buffer[i] = data[i];
		}
		myPM->writeDiskBlock(personMap[fileDesc].loc, buffer);
		personMap[fileDesc].rwptr += len;
		//cout << personMap[fileDesc].rwptr << endl;

	}
	else{
		int compareNum = 2;
		int nextBlock;
		//keep allocating blocks while our need is more than a block left
		while (len - (64 * globalMap[personMap[fileDesc].name].blockCount) > 64)
		{
			if(globalMap[personMap[fileDesc].name].blockCount > compareNum)
			{
				if (compareNum == 16){
					cout << "ERROR: NO MORE FILE SPACE!!!!!!!" << endl;
					return -4;
				}
				//compareNum = 16;
				myPM->readDiskBlock(globalMap[personMap[fileDesc].name].inode, buffer);
				int inodeExist = buffer[18] - '0';
				cout << "I node exist: "<< inodeExist << endl;
				if (inodeExist != 0){
					cout << "inode does exist" << endl;
					myPM->readDiskBlock(nextBlock, buffer);
					int newBlock = myPM->getFreeDiskBlock();;
					int offset2 = 4;
					for (int n = 0; n < 15; n++){
						if (buffer[offset2] == '0')
						{
							personMap[fileDesc].loc = newBlock;
							sprintf(buffer+offset2, "%i", newBlock);
							myPM->writeDiskBlock(nextBlock, buffer);
							n = 15;
						}
						offset2+=4;
					}
					for(int i = 0; i < 64; i++) {
						buffer[i] = data[(64 * globalMap[personMap[fileDesc].name].blockCount) + i];
					}
					myPM->writeDiskBlock(personMap[fileDesc].loc, buffer);


				}
				else
				{
					cout << "inode does not exist" << endl;
					nextBlock = myPM->getFreeDiskBlock();
					int nextWriteBlock = myPM->getFreeDiskBlock();
					sprintf(buffer+18, "%i", nextBlock);
					myPM->writeDiskBlock(globalMap[personMap[fileDesc].name].inode, buffer);
					fill_n(buffer, 64, '.');
					int offset = 4;
					sprintf(buffer, "%i", nextWriteBlock);
					for(int k = 0; k < 15; k++){
						sprintf(buffer+offset, "%i", 0);
						offset += 4;
					}
					myPM->writeDiskBlock(nextBlock, buffer);
					personMap[fileDesc].loc = nextWriteBlock;

					for(int i = 0; i < 64; i++) {
						buffer[i] = data[(64 * globalMap[personMap[fileDesc].name].blockCount) + i];
					}
					myPM->writeDiskBlock(personMap[fileDesc].loc, buffer);

				}
				
				
			}
			else
			{
				//finish writing out current block.
				if((personMap[fileDesc].rwptr) < 64) {
					//cout<< "I am entering the else statement into the for: " << personMap[fileDesc].rwptr << endl;
					for(int i = (personMap[fileDesc].rwptr); i < 64; i++) {
						buffer[i] = data[(64 * globalMap[personMap[fileDesc].name].blockCount) + i];
					}
					myPM->writeDiskBlock(personMap[fileDesc].loc, buffer);

				}
				personMap[fileDesc].rwptr = 0;
				//allocate additional block
				int newFileLoc = myPM->getFreeDiskBlock();
				myPM->readDiskBlock(globalMap[personMap[fileDesc].name].inode, buffer);
				int offset = 6;
				for(int i = 0; i < 3; i++){
					if(buffer[offset] == '0') {
						personMap[fileDesc].loc = newFileLoc;
						sprintf(buffer+offset, "%i", newFileLoc);
						myPM->writeDiskBlock(globalMap[personMap[fileDesc].name].inode, buffer);
						i = 3;
						//break;
					}
					offset+=4;
				}
			}
			globalMap[personMap[fileDesc].name].blockCount++;
		}

		fill_n(buffer, 64, 'i');
		//finish writing out data to the last block allocated
		for(int i = 0; i < (64 - personMap[fileDesc].rwptr); i++) {
			buffer[i] = data[(64 * globalMap[personMap[fileDesc].name].blockCount) + i];

		}
		personMap[fileDesc].rwptr = (64 - personMap[fileDesc].rwptr);
		cout << "Read Write Pointer: "<<personMap[fileDesc].rwptr << endl;
		myPM->writeDiskBlock(personMap[fileDesc].loc, buffer);

	}

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
	//cout<< "Generated ID: " << ID << endl;
	return ID;
	
}
int FileSystem::getLockID(){
	int id;
  	srand (time(0));

  	id = rand() % 100 + 1;
		
	while (lockIDNums[id] == true){
  		id = rand() % 100 + 1;
				
	}
	lockIDNums[id] = true;
	//cout<< "Generated ID: " << ID << endl;
	return id;
	
}
bool FileSystem::searchForFile(char fileName){
	char buffer[64];
	myPM->readDiskBlock(1, buffer);

	int offset = 5;
    
	for(int i = 0; i < 11; i++){
		if(buffer[offset] == fileName){
			return true;
		}
		offset+=5;
	}
	return false;

}
