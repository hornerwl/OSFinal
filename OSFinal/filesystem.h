#include <string>
#include <map>
class FileSystem {
  DiskManager *myDM;
  PartitionManager *myPM;
  char myfileSystemName;
  int myfileSystemSize;
  
  /* declare other private members here */
  
  struct gft{
    int inode;
    int fileLoc;
    int size;
    char *attr;
    int lockId;
    int blockCount;
    int inodeptr;

  };
  struct pp {
  	char name;
  	int loc;
    int rwptr;
  	char mode;
  };
  
  map<char, gft> globalMap;
  map<int, pp> personMap;

  public:
    FileSystem(DiskManager *dm, char fileSystemName);
    int createFile(char *filename, int fnameLen);
    int getUniqueID();
    int createDirectory(char *dirname, int dnameLen);
    int lockFile(char *filename, int fnameLen);
    int unlockFile(char *filename, int fnameLen, int lockId);
    int deleteFile(char *filename, int fnameLen);
    int deleteDirectory(char *dirname, int dnameLen);
    int openFile(char *filename, int fnameLen, char mode, int lockId);
    int closeFile(int fileDesc);
    int readFile(int fileDesc, char *data, int len);
    int writeFile(int fileDesc, char *data, int len);
    int appendFile(int fileDesc, char *data, int len);
    int seekFile(int fileDesc, int offset, int flag);
    int renameFile(char *filename1, int fnameLen1, char *filename2, int fnameLen2);
    int getAttribute(char *filename, int fnameLen /* ... and other parameters as needed */);
    int setAttribute(char *filename, int fnameLen /* ... and other parameters as needed */);

    /* declare other public members here */
    bool searchForFile(char fileName, int block);
    int getLockID();
    int convertString(char *data, int startLoc);
    void setLoc(int currentBlock, int currentReadWrite, int fileDesc);
    int convertLoc(int currentLoc, int fileDesc);

};
