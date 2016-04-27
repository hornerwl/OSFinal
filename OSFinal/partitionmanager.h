#include <cstdio>
#include <iostream>
#include "bitvector.h"

class PartitionManager {
  DiskManager *myDM;
  public:
    char myPartitionName;
    int myPartitionSize;
    PartitionManager(DiskManager *dm, char partitionname, int partitionsize);
    ~PartitionManager();
    int readDiskBlock(int blknum, char *blkdata);
    int writeDiskBlock(int blknum, char *blkdata);
    int getBlockSize();
    int getFreeDiskBlock();
    int returnDiskBlock(int blknum);
  private:
	BitVector *BV;
};

