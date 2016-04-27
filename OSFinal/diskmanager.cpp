#include "disk.h"
#include "diskmanager.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <sstream>
using namespace std;

DiskManager::DiskManager(Disk *d, int partcount, DiskPartition *dp)
{
  myDisk = d;
  partCount= partcount;
  int r = myDisk->initDisk();
  char buffer[64];
  fill_n(buffer, 64, 'c');
  diskP = new DiskPartition[partCount];
  int start = 1;
  int index = 0;
  if (r == 1)
  {
	  //cout << "We have new disk '''''''''''''''''''''''''''''''''''''''''''''''''''''''''" << endl;

	  /* If needed, initialize the disk to keep partition information */
	  
          
	  for (int i = 0; i < partCount; i++)
	  {
		diskP[i].partitionName = dp[i].partitionName;
		diskP[i].partitionSize = dp[i].partitionSize;
		
		
		int size = dp[i].partitionSize;

		sprintf(buffer+(index),"%c", dp[i].partitionName);
		index+=1;
		sprintf(buffer+(index),"%i", size);
		index+=4;
		sprintf(buffer+(index), "%i", start);
		
		start += size;
		index += 3;
		myDisk->writeDiskBlock(0, buffer);
	  }
          
  }
  /* else  read back the partition information from the DISK1 */
  else
  {
    //cout << "We DO NOT have new disk '''''''''''''''''''''''''''''''''''''''''''''''''''''''''" << endl;
	index = 0;
	int actualValue = 0;

	for(int i = 0; i < partCount; i++) {
		myDisk->readDiskBlock(0, buffer);
		diskP[i].partitionName = buffer[index];
		string value;
		for(int j = index+1; j < index+4; j++)
		{
			value += buffer[j];
		}
		istringstream iss;
		iss.str(value);
		iss >> actualValue;
		index +=8; 
		diskP[i].partitionSize = actualValue;
	}	
  }
}

/*
 *   returns: 
 *   0, if the block is successfully read;
 *  -1, if disk can't be opened; (same as disk)
 *  -2, if blknum is out of bounds; (same as disk)
 *  -3 if partition doesn't exist
 */
int DiskManager::readDiskBlock(char partitionname, int blknum, char *blkdata)
{
  /* write the code for reading a disk block from a partition */
  bool partitionExist = false;
  int startblk = 1;
  for (int i = 0; i < partCount; i++){
		if (diskP[i].partitionName == partitionname)
		{
			partitionExist = true;
			break;
		}
		else
			startblk += diskP[i].partitionSize;
  }
  if(!partitionExist) return (-3);
  int returnNum = myDisk->readDiskBlock((startblk+blknum), blkdata);
  return returnNum;
}


/*
 *   returns: 
 *   0, if the block is successfully written;
 *  -1, if disk can't be opened; (same as disk)
 *  -2, if blknum is out of bounds;  (same as disk)
 *  -3 if partition doesn't exist
 */
int DiskManager::writeDiskBlock(char partitionname, int blknum, char *blkdata)
{
  /* write the code for writing a disk block to a partition */
  /* write the code for reading a disk block from a partition */
  bool partitionExist = false;
  int startblk = 1;
  for (int i = 0; i < partCount; i++){
		if (diskP[i].partitionName == partitionname)
		{
			partitionExist = true;
			break;
		}
		else
			startblk += diskP[i].partitionSize;
  }
  if(!partitionExist) return (-3);
  int returnNum = myDisk->writeDiskBlock((startblk+blknum), blkdata);
  return 0;
}

/*
 * return size of partition
 * -1 if partition doesn't exist.
 */
int DiskManager::getPartitionSize(char partitionname)
{
	int size = 0;
  /* write the code for returning partition size */
	for (int i = 0; i < partCount; i++){
		if (diskP[i].partitionName == partitionname)
			size = diskP[i].partitionSize;
	}
	return size;

}
//int DiskManager::getBlockSize() 
//{
  //return myDisk->getBlockSize();
//}
