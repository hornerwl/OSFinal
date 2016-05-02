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
#include <math.h> 
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
				//cout << "File has already been created" << endl;
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
		offset = 2;
		for(int i = 0; i < 10; i++){
			if(dataBuffer[offset] == 'e'){
				dataBuffer[offset-1] = fName;
				dataBuffer[offset] = 'f';
				sprintf(dataBuffer+offset+1, "%i", inodeLoc);
				break;
			}
			offset+=6;
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
		//cout << "Entry Created " << process << " " << personMap[process].name << " " << personMap[process].mode << " " << personMap[process].loc << endl;
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
	char buffer[64];
	fill_n(buffer, 64, 'c');
	int length = len;
	int read = 0;
	int blocksNavigated = 0;
	if(personMap.find(fileDesc) == personMap.end()){
		return -1;
	}
	//check to make sure out length is not negative
	else if (len < 0) {
		return -2;
	}
	//check to make sure we have permission to write
	else if(personMap[fileDesc].mode != 'r' && personMap[fileDesc].mode != 'm') {
		return -3;
	}
	//cout << "Location of read: byte:" <<  personMap[fileDesc].rwptr << " in block " << personMap[fileDesc].loc << endl;
	if ((personMap[fileDesc].rwptr + len) < 64)
	{
		myPM->readDiskBlock(personMap[fileDesc].loc, buffer);
		for (int i = personMap[fileDesc].rwptr; i < (personMap[fileDesc].rwptr + len); i++){
			if (buffer[i] != 'c'){
				data[i-personMap[fileDesc].rwptr] = buffer[i];
				read++;
			}
			//data[i-personMap[fileDesc].rwptr] = buffer[i];
		}

		personMap[fileDesc].rwptr += read;
	}
	else
	{
		//move to the next block and keep reading
		char fileinode[64];
		int myValue;
		int dataBufferOffset = personMap[fileDesc].rwptr;
		//cout << "inode pointer------------- : " << globalMap[personMap[fileDesc].name].inode << endl;
		myPM->readDiskBlock(globalMap[personMap[fileDesc].name].inode, fileinode);
		while (length > 64){
			myPM->readDiskBlock(personMap[fileDesc].loc, buffer);
			for (int i = personMap[fileDesc].rwptr; i < (64); i++){
				if (buffer[i] != 'c'){
				data[i + (64 * blocksNavigated) - dataBufferOffset] = buffer[i];
				read++;
				}
				
			}
			blocksNavigated++;
			length -= 64;
			//find next block
			int offset = 6;
			for (int j = 0; j < 2; j++){
				myValue = convertString(fileinode, offset);
				//cout << "Compare Value: " << myValue << " : " << personMap[fileDesc].loc << endl;
				if (myValue == personMap[fileDesc].loc)
				{
					offset += 4;
					myValue = convertString(fileinode, offset);
					if (myValue == 0)
					{
						//cout << "End of file change nothing" << endl;
					}
					else{
						personMap[fileDesc].loc = myValue; 
						//cout << "Next File Location: " << personMap[fileDesc].loc << endl;
						personMap[fileDesc].rwptr = 0; 
					}
					j = 2;
				}

				offset += 4;
			}
			myValue = convertString(fileinode, offset);
			myPM->readDiskBlock(globalMap[personMap[fileDesc].name].inodeptr, fileinode);
			//cout << "Compare Value: " << myValue << " : " << personMap[fileDesc].loc << endl;
			if (myValue == personMap[fileDesc].loc) {
				//move to inode
				myValue = convertString(fileinode, 0);
				if (myValue == 0)
					{
						cout << "End of file change nothing" << endl;
					}
				else{
					personMap[fileDesc].loc = myValue; 
					//cout << "Next File Location: " << personMap[fileDesc].loc << endl;
					personMap[fileDesc].rwptr = 0; 
				}

			}
			else
			{
				int offset = 0;
				for (int k = 0; k < 14; k++){
					myValue = convertString(fileinode, offset);
					//cout << "Compare Value: " << myValue << " : " << personMap[fileDesc].loc << endl;
					if (myValue == personMap[fileDesc].loc)
					{
						offset += 4;
						myValue = convertString(fileinode, offset);
						if (myValue == 0)
						{
							//cout << "End of file change nothing" << endl;
						}
						else{
							personMap[fileDesc].loc = myValue; 
							//cout << "Next File Location: " << personMap[fileDesc].loc << endl;
							personMap[fileDesc].rwptr = 0; 
						}
						k = 16;
					}

					offset += 4;
				}
			}
		}
		myPM->readDiskBlock(personMap[fileDesc].loc, buffer);
		//cout <<  (len - 64 * blocksNavigated) << endl;
		for (int i = personMap[fileDesc].rwptr; i < (len - 64 * blocksNavigated); i++){
			if (buffer[i] != 'c'){
				data[((64 * blocksNavigated) + i) - dataBufferOffset] = buffer[i];
				read++;
			}
			//data[i-personMap[fileDesc].rwptr] = buffer[i];
		}

	}



	return read;//len;//sizeof(data);
}
int FileSystem::convertString(char *data, int startLoc){
	string var;
	var.push_back(data[startLoc]);
	var.push_back(data[startLoc + 1]);
	var.push_back(data[startLoc + 2]);
	var.push_back(data[startLoc + 3]);
	istringstream converter(var);
	int var2;
	converter >> var2;
	return var2;

}
int FileSystem::writeFile(int fileDesc, char *data, int len){
	char buffer[64];
	char databuffer[64];
	bool inodeWriteFlag = false;
	int lastFile;
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
		lastFile = (personMap[fileDesc].rwptr + len) - 64;
		personMap[fileDesc].rwptr += len;
	}
	else{
		int compareNum = 2;
		int nextBlock;
		//keep allocating blocks while our need is more than a block left
		while (len - (64 * globalMap[personMap[fileDesc].name].blockCount) > 64)
		{
			if(globalMap[personMap[fileDesc].name].blockCount > compareNum)
			{
				if (globalMap[personMap[fileDesc].name].blockCount == 16){
					//cout << "ERROR: NO MORE FILE SPACE!!!!!!!" << endl;
					return -4;
				}
				//compareNum = 16;
				myPM->readDiskBlock(globalMap[personMap[fileDesc].name].inode, buffer);
				int inodeExist =  convertString (buffer, 18);

				if (inodeExist != 0){

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
					nextBlock = myPM->getFreeDiskBlock();
					int nextWriteBlock = myPM->getFreeDiskBlock();
					sprintf(buffer+18, "%i", nextBlock);
					globalMap[personMap[fileDesc].name].inodeptr = nextBlock;
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
					inodeWriteFlag = true;
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
		if(inodeWriteFlag){
			myPM->readDiskBlock(globalMap[personMap[fileDesc].name].inodeptr, buffer);
			int newBlock2 = myPM->getFreeDiskBlock();
			int offset3 = 4;
			for (int n = 0; n < 15; n++){
				if (buffer[offset3] == '0')
				{
					personMap[fileDesc].loc = newBlock2;
					sprintf(buffer+offset3, "%i", newBlock2);
					myPM->writeDiskBlock(globalMap[personMap[fileDesc].name].inodeptr, buffer);
					n = 15;
				}
				offset3+=4;
			}
			fill_n(buffer, 64, 'c');
			for(int i = 0; i < len - (64 * globalMap[personMap[fileDesc].name].blockCount); i++) {
				buffer[i] = data[(64 * globalMap[personMap[fileDesc].name].blockCount) + i];
			}
			lastFile = len - (64 * globalMap[personMap[fileDesc].name].blockCount);
			personMap[fileDesc].rwptr = len - (64 * globalMap[personMap[fileDesc].name].blockCount);
			//cout << "Read Write Pointer: "<<personMap[fileDesc].rwptr << endl;
		}
		else {
			fill_n(buffer, 64, 'c');
			//finish writing out data to the last block allocated
			for(int i = 0; i < (64 - personMap[fileDesc].rwptr); i++) {
				buffer[i] = data[(64 * globalMap[personMap[fileDesc].name].blockCount) + i];

			}
			personMap[fileDesc].rwptr = (64 - personMap[fileDesc].rwptr);
			//cout << "Read Write Pointer: "<<personMap[fileDesc].rwptr << endl;
			lastFile = (64 - personMap[fileDesc].rwptr);
		}

	}
	myPM->writeDiskBlock(personMap[fileDesc].loc, buffer);

	if (globalMap[personMap[fileDesc].name].size < ((64 * globalMap[personMap[fileDesc].name].blockCount) + lastFile))
	{
			globalMap[personMap[fileDesc].name].size = (64 * globalMap[personMap[fileDesc].name].blockCount) + lastFile;

			myPM->readDiskBlock(globalMap[personMap[fileDesc].name].inode, buffer);
			sprintf(buffer+2, "%i", globalMap[personMap[fileDesc].name].size);
 			myPM->writeDiskBlock(globalMap[personMap[fileDesc].name].inode, buffer);
	}
	return len;
}
int FileSystem::appendFile(int fileDesc, char *data, int len){
	int savedLOC = personMap[fileDesc].loc;
	int savedRWPTR = personMap[fileDesc].rwptr;
	char buffer[64];
	int getValue;

	if(globalMap[personMap[fileDesc].name].blockCount <= 3){
		//search direct inode
		myPM->readDiskBlock(globalMap[personMap[fileDesc].name].inode, buffer);
		getValue = 2 + (4 * globalMap[personMap[fileDesc].name].blockCount) ;
	}
	else
	{
		//search indirect indode
		myPM->readDiskBlock(globalMap[personMap[fileDesc].name].inodeptr, buffer);
		getValue = 4 * ((globalMap[personMap[fileDesc].name].blockCount) -4);
	}

	personMap[fileDesc].loc = convertString (buffer, getValue);

	personMap[fileDesc].rwptr = globalMap[personMap[fileDesc].name].size % 64;
	int returnValue = writeFile(fileDesc, data, len);
	personMap[fileDesc].loc = savedLOC;
	personMap[fileDesc].rwptr = savedRWPTR;
	return returnValue;
}
//TODO: Ask about flag systems
int FileSystem::seekFile(int fileDesc, int offset, int flag){
	int returnVal;
	char inode[64];
	int myValue, blockIam = 0;
	if (personMap.find(fileDesc) == personMap.end()){
		return - 1;
	}
	if (flag == 0)
	{
		myPM->readDiskBlock(globalMap[personMap[fileDesc].name].inode, inode);
		int currentLoc;
		//---------------------------------------------------------------------------------------
		bool foundMe = false;
		int displace = 6;
		for (int j = 0; j < 2; j++){
			myValue = convertString(inode, displace);
			//cout << "Compare Value: " << myValue << " : " << personMap[fileDesc].loc << endl;
			if (myValue == personMap[fileDesc].loc)
			{
				foundMe = true;
				break;
			}

			displace += 4;
			blockIam++;
		}
		//blockIam++;//account for 0th block of indirect inode
		myPM->readDiskBlock(globalMap[personMap[fileDesc].name].inodeptr, inode);
		if (!foundMe)
		{
			blockIam++;//account for 0th block of indirect inode
			displace = 0;
			for (int k = 0; k < 14; k++){
					myValue = convertString(inode, displace);
					//cout << "Compare Value: " << myValue << " : " << personMap[fileDesc].loc << endl;
					if (myValue == personMap[fileDesc].loc)
					{
						break;
					}
					displace += 4;
					blockIam++;
				}
		}
		//cout << "My pointer is in block: " << blockIam << endl;
		//---------------------------------------------------------------------------------------
		/*if (globalMap[personMap[fileDesc].name].blockCount < 3){
			currentLoc = (blockIam * 64) + personMap[fileDesc].rwptr;
		}
		else
		{
			currentLoc = ((globalMap[personMap[fileDesc].name].blockCount) * 64) + personMap[fileDesc].rwptr;
		}*/
		currentLoc = (blockIam * 64) + personMap[fileDesc].rwptr;
		cout << "CurrentLoc in terms of size: " << currentLoc << endl;
		//cout << "Offset: " << offset << endl;
		currentLoc += offset;
		//cout << "Changed Location: " << currentLoc << endl;
		returnVal = convertLoc(currentLoc, fileDesc);
		return returnVal;

	}
	else{
		if ((offset < 0 || offset > globalMap[personMap[fileDesc].name].size))
		{
			return -1;
		}
		returnVal = convertLoc(offset, fileDesc);
		return returnVal;
	}
	return -1;

}
int FileSystem::convertLoc(int currentLoc, int fileDesc){
	int readwrite;
	double block;
	if (currentLoc < 0 || currentLoc > globalMap[personMap[fileDesc].name].size){
		//cout << "Size: " << globalMap[personMap[fileDesc].name].size << endl;
		return -2;
	}
	readwrite = currentLoc % 64;
	block = floor(currentLoc/64);
	//cout << "Block: " << int(block) << " rwptr: " << readwrite << endl;

	setLoc(int(block), readwrite, fileDesc);
	return 0;

}
void FileSystem::setLoc(int currentBlock, int currentReadWrite, int fileDesc){
	char file[64];
	char indirect[64];

	myPM->readDiskBlock(globalMap[personMap[fileDesc].name].inode, file);
	myPM->readDiskBlock(globalMap[personMap[fileDesc].name].inodeptr, indirect);

	if ( currentBlock < 3){
		int blockLocation = 6 + (currentBlock * 4);
		personMap[fileDesc].loc = convertString(file, blockLocation);
	}
	else
	{
		int blockLocation = ((currentBlock-3) * 4);
		personMap[fileDesc].loc = convertString(indirect, blockLocation);
	}
	personMap[fileDesc].rwptr = currentReadWrite;
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

	int offset = 1;
    
	for(int i = 0; i < 11; i++){
		if(buffer[offset] == fileName){
			return true;
		}
		offset+=6;
	}
	return false;

}
