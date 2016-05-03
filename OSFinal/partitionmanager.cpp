#include "disk.h"
#include "diskmanager.h"
#include "partitionmanager.h"
#include <iostream>
#include <vector>
#include <stdio.h>
using namespace std;

PartitionManager::PartitionManager(DiskManager *dm, char partitionname, int partitionsize)
{
	char buffer[64];
	fill_n(buffer, 64 , '0');
	myDM = dm;
	myPartitionName = partitionname;
	myPartitionSize = myDM->getPartitionSize(myPartitionName);
	/* If needed, initialize bit vector to keep track of free and allocted
	blocks in this partition */
	if (myPartitionSize > 0)
	{
	//cout<< "New Size New BitVector!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
	BV = new BitVector(myPartitionSize);
	

	}

	int checkBv = myDM->readDiskBlock(myPartitionName, 0, buffer);

	if(checkBv == 0){
		if(buffer[0] == 'c'){
			//cout<< "New BitVector!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
			BV->setBit(0);
			BV->setBit(1);
			BV->getBitVector((unsigned int*) buffer);
			myDM->writeDiskBlock(myPartitionName, 0, buffer);
			fill_n(buffer, 64 , '.');
			buffer[0] = '/';
			int offset = 1;
			for (int i = 0; i<10; i++ )
			{
				buffer[offset] = '*';
				offset++;
				buffer[offset] = 'e';
				offset++;
				sprintf(buffer+(offset), "%i", 0);
				offset+= 4;
			}
			sprintf(buffer+(offset), "%i", 0);
			myDM->writeDiskBlock(myPartitionName, 1, buffer);
		}
		else {
			BV->setBitVector((unsigned int*) buffer);
		}
	}
	else if(checkBv == -1) {
		cerr<<"DISK READ ERROR"<<endl;
	}
	
}

PartitionManager::~PartitionManager()
{
}

/*
 * return blocknum, -1 otherwise
 */
int PartitionManager::getFreeDiskBlock()
{
	char buf[64];
	fill_n (buf, 64, '0');
  /* write the code for allocating a partition block */
  for (int i = 0; i < myPartitionSize; i++)
  {
  	//cout<<BV->testBit(i)<<endl;
    if(BV->testBit(i) == 0)
    {
      BV->setBit(i);
      BV->getBitVector((unsigned int*) buf);
      myDM->writeDiskBlock(myPartitionName, 0, buf);
      return i;
    }
  }
  return -1;
}

/*
 * return 0 for sucess, -1 otherwise
 */
int PartitionManager::returnDiskBlock(int blknum)
{
  /* write the code for deallocating a partition block */
  BV->resetBit(blknum);
  char buffer[getBlockSize()];
  fill_n(buffer, getBlockSize(), 'c');
  myDM->writeDiskBlock(myPartitionName, blknum, buffer);
  char buf[64];
  fill_n (buf, 64, '0');
  BV->getBitVector((unsigned int*) buf);
  myDM->writeDiskBlock(myPartitionName, 0, buf);
  return 0;
}


int PartitionManager::readDiskBlock(int blknum, char *blkdata)
{
  return myDM->readDiskBlock(myPartitionName, blknum, blkdata);
}

int PartitionManager::writeDiskBlock(int blknum, char *blkdata)
{
  return myDM->writeDiskBlock(myPartitionName, blknum, blkdata);
}

int PartitionManager::getBlockSize() 
{
  return myDM->getBlockSize();
}
