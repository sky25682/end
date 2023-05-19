#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
// #include <io.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

//기타 매크로
#define DWORD   unsigned int
#define BYTE unsigned char
#define MIN(x,y)    (((x) < (y)) ? (x) : (y))

#define SERIAL_FIFOMAXSIZE  16  

int main(int argc, char** argv)
{
    char vcFileName[256];
    char vcDataBuffer[SERIAL_FIFOMAXSIZE];
    struct sockaddr_in stSocketAddr;
    int iSocket;
    BYTE bAck;
    DWORD dwDataLength;
    DWORD dwSentSize;
    DWORD dwTemp;
    FILE* fp;

    //-----------------------------------------------------
    //파일 열기
    //------------------------------------------------------
    if(argc<2)
    {
        fprintf(stderr,"Input File Name: ");
        gets(vcFileName);
    }
    else
    {
        strcpy(vcFileName,argv[1]);
    }
    fp = fopen(vcFileName,"rb");
    if(fp == NULL)
    {
        fprintf(stderr,"%s File Open Error\n",vcFileName);
        return 0;
    }

    fseek(fp,0,SEEK_END);
    dwDataLength = ftell(fp);
    fseek(fp,0,SEEK_SET);
    fprintf(stderr,"File name %s, Data Length %d Byte\n",vcFileName,dwDataLength);

    //-----------------------------------------
    //네트워크 접속
    //-------------------------------------------
    //접속할 QEMU의 Address를 설정
    stSocketAddr.sin_family = AF_INET;
    stSocketAddr.sin_port = htons(4444);
    stSocketAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    //소켓 생성 후 QEMU에 접속 시도
    iSocket = socket(AF_INET,SOCK_STREAM,0);
    if(connect(iSocket,(struct sockaddr*) & stSocketAddr, sizeof(stSocketAddr)) == -1)
    {
        fprintf(stderr, "Socket Connect Error IP 127.0.0.1, Port 4444\n");
        return 0;
    }
    else
    {
        fprintf(stderr,"Socket Connect Success, IP 127.0.0.1 Port 4444\n");
    }

    //----------------------------------
    //데이터 전송
    //----------------------------------
    // 데이터 길이를 전송
    if(send(iSocket,&dwDataLength,4,0) != 4)
    {
        fprintf(stderr,"Data Length Send Fail, [%d] Byte\n",dwDataLength);
        return 0;
    }   
    else
    {
        fprintf(stderr,"Data Length Send Success, [%d] Byte\n",dwDataLength);
    }
    //zck수신할ㄸ'까지 대기
    if(recv(iSocket,&bAck,1,0) != 1)
    {
        fprintf(stderr,"Ack Receive Erroe\n");
        return 0;
    }
    //데이터 전송
    fprintf(stderr,"Now Data transfer...");
    dwSentSize = 0;
    while(dwSentSize < dwDataLength)
    {
        dwTemp = MIN(dwDataLength - dwSentSize, SERIAL_FIFOMAXSIZE);
        dwSentSize += dwTemp;

        if(fread(vcDataBuffer,1,dwTemp,fp) != dwTemp)
        {
            fprintf(stderr, "File Read Error\n");
            return 0;
        }
        if(send(iSocket,vcDataBuffer,dwTemp,0) != dwTemp)
        {
            fprintf(stderr,"Socket Send Erroe\n");
            return 0;
        }

        if(recv(iSocket,&bAck,1,0) != 1){
        fprintf(stderr,"Ack Receive Error\n");
        return 0;
        }
        fprintf(stderr,"#");
    }
    fclose(fp);
    close(iSocket);

    fprintf(stderr,"\nSend Complete. [%d] Byte\n",dwSentSize);
    fprintf(stderr,"Press Enter Key To Exit\n");
    getchar();
    return 0;   
}

