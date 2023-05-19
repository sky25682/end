#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BYTESOFTSECTOR  512

#define PACKAGESIGNATURE    "MINT64OSPACKAGE "
#define MAXFILENAMELENGTH 24

#define DWORD   unsigned int

#pragma pack(push,1)

typedef struct PackageItemStruct
{
    char vcFileName[MAXFILENAMELENGTH];
    DWORD dwFileLength;
}PACKAGEITEM;

typedef struct PackageHeaderStruct
{
    char vcSignature[16];
    DWORD dwHeaderSize;
    PACKAGEITEM vstItem[0];
}PACKAGEHEADER;
#pragma pack(pop)

int AdjustInSectorSize(int iFd,int iSourceSize);
int CopyFile(int iSourceFd,int iTargetFd);

int main(int argc,char* argv[])
{
    int iSourceFd;
    int iTargetFd;
    int iSourceSize;
    int i;
    struct stat stFileData;
    PACKAGEHEADER stHeader;
    PACKAGEITEM stItem;

    if(argc<2)
    {
        fprintf(stderr,"[ERROR] PackageMaker.exe app1.elf app2.elf data.txt ...\n");
        exit(-1);
    }
    if((iTargetFd = open("Package.img",O_RDWR | O_CREAT | O_TRUNC , __S_IREAD | __S_IWRITE))==-1)
    {
        fprintf(stderr,"[ERROR] Package.img open fail\n");
        exit(-1);
    }
    printf("[INFO] Create package header ...\n");

    memcpy(stHeader.vcSignature,PACKAGESIGNATURE,sizeof(stHeader.vcSignature));
    stHeader.dwHeaderSize = sizeof(PACKAGEHEADER) + (argc - 1 ) * sizeof(PACKAGEITEM);
    if(write(iTargetFd,&stHeader,sizeof(stHeader)) != sizeof(stHeader))
    {
        fprintf(stderr,"[ERROR] Data write fail\n");
        exit(-1);
    }
    for(i=1;i<argc;i++)
    {
        if(stat(argv[i],&stFileData) != 0)
        {
            fprintf(stderr,"ERROR %s file open fail\n");
            exit(-1);
        }
        memset(stItem.vcFileName,0,sizeof(stItem.vcFileName));
        strncpy(stItem.vcFileName,argv[i],sizeof(stItem.vcFileName));
        stItem.vcFileName[sizeof(stItem.vcFileName) - 1] = '\0';
        stItem.dwFileLength = stFileData.st_size;

        if(write(iTargetFd,&stItem,sizeof(stItem)) != sizeof(stItem))
        {
            fprintf(stderr,"EROR Data Write fial\n");
            exit(-1);
        }
        printf("[%d] file : %s,size: %d Byte\n",i,argv[i],stFileData.st_size);

    }
    printf("iffo Create complete\n");

    printf("info copy data file to package...\n");
    iSourceSize = 0;
    for(i=1;i<argc;i++)
    {
        if((iSourceFd = open(argv[i],O_RDONLY)) == -1)
        {
            fprintf(stderr,"ERORR %s open fail\n",argv[1]);
            exit(-1);
        }
        iSourceSize += CopyFile(iSourceFd,iTargetFd);
        close(iSourceFd);
    }

    AdjustInSectorSize(iTargetFd,iSourceSize + stHeader.dwHeaderSize);
    printf("info Total %d byte copy complete\n",iSourceSize);
    printf("info package file create complete\n");

    close(iTargetFd);
    return 0;

}

int AdjustInSectorSize(int iFd,int iSourceSize)
{
    int i;
    int iAdjustSizeToSector;
    char cCh;
    int iSectorCount;

    iAdjustSizeToSector = iSourceSize % BYTESOFTSECTOR;
    cCh = 0x00;

    if(iAdjustSizeToSector != 0)
    {
        iAdjustSizeToSector = 512 - iAdjustSizeToSector;
        for(i=0;i<iAdjustSizeToSector;i++)
        {
            write(iFd,&cCh,1);
        }
    }
    else
    {
        printf("info file size is aligned 512 byte\n");
    }
    iSectorCount = (iSourceSize + iAdjustSizeToSector) / BYTESOFTSECTOR;
    return iSectorCount;
}

int CopyFile(int iSourceFd,int iTargetFd)
{
    int iSourceFileSize;
    int iRead;
    int iWrite;
    char vcBuffer[BYTESOFTSECTOR];
    iSourceFileSize = 0;
    while(1)
    {
        iRead = read(iSourceFd,vcBuffer,sizeof(vcBuffer));
        iWrite = write(iTargetFd,vcBuffer,iRead);
        if(iRead != iWrite)
        {
            fprintf(stderr,"ERROR iRead != iWrite..\n");
            exit(-1);
        }
        iSourceFileSize += iRead;
        if(iRead != sizeof(vcBuffer))
        {
            break;
        }
    }
    return iSourceFileSize;
}




