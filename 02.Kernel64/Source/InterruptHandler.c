/**
 *  file    InterruptHandler.c
 *  date    2009/01/24
 *  author  kkamagui 
 *          Copyright(c)2008 All rights reserved by kkamagui
 *  brief   인터럽트 및 예외 핸들러에 관련된 소스 파일
 */

#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"
#include "Console.h"
#include "Utility.h"
#include "Task.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "HardDisk.h"
#include "Mouse.h"

static INTERRUPTMANAGER gs_stInterruptManager;

void kInitializeHandler(void)
{
    kMemSet(&gs_stInterruptManager,0,sizeof(gs_stInterruptManager));
}

void kSetSymmetricIOMode(BOOL bSymmestricIOMode)
{
    gs_stInterruptManager.bSymmetricIOMode = bSymmestricIOMode;
}

void kSetInterruptLoadBalancing(BOOL bUseLoadBalancing)
{
    gs_stInterruptManager.bUseLoadBalancing = bUseLoadBalancing;
}

void kIncreaseInterruptCount(int iIRQ)
{
    gs_stInterruptManager.vvqwCoreInterruptCount[kGetAPICID()][iIRQ]++;
}

void kSendEOI(int iIRQ)
{
    if(gs_stInterruptManager.bSymmetricIOMode == FALSE)
    {
        kSendEOIToPIC(iIRQ);
    }
    else
    {
        kSendEOIToLocalAPIC();
    }
}

INTERRUPTMANAGER* kGetInterruptManager(void)
{
    return &gs_stInterruptManager;
}

void kProcessLoadBalancing(int iIRQ)
{
    QWORD qwMinCount = 0xFFFFFFFFFFFFFFFF;
    int iMinCountCoreIndex;
    int iCoreCount;
    int i;
    BOOL bResetCount = FALSE;
    BYTE bAPICID;

    bAPICID = kGetAPICID();

    if((gs_stInterruptManager.vvqwCoreInterruptCount[bAPICID][iIRQ] == 0 ) ||
    ((gs_stInterruptManager.vvqwCoreInterruptCount[bAPICID][iIRQ] % INTERRUPT_LOADBALANCINGDIVIDOR) != 0) ||
    (gs_stInterruptManager.bUseLoadBalancing == FALSE))
    {
        return ;
    }

    iMinCountCoreIndex = 0;
    iCoreCount = kGetProcessorCount();
    for(i=0;i<iCoreCount;i++)
    {
        if((gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ] <
        qwMinCount))
        {
            qwMinCount = gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ];
            iMinCountCoreIndex = i;
        }
        else if(gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ] >= 0xFFFFFFFFFFFFFFFE)
        {
            bResetCount = TRUE;
        }
    }
    kRoutingIRQToAPICID(iIRQ,iMinCountCoreIndex);

    if(bResetCount == TRUE)
    {
        for(i=0;i<iCoreCount;i++)
        {
            gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ] = 0;
        }
    }
}


/**
 *  공통으로 사용하는 예외 핸들러
 */
void kCommonExceptionHandler( int iVectorNumber, QWORD qwErrorCode )
{
    char vcBuffer[ 100 ];
    BYTE bAPICID;
    TCB* pstTask;

    bAPICID = kGetAPICID();
    pstTask = kGetRunningTask(bAPICID);
    
    kPrintStringXY( 0, 0, "====================================================" );
    kPrintStringXY( 0, 1, "                 Exception Occur~!!!!               " );
    kSPrintf( vcBuffer, "   Vector:%d    Core ID:0x%X     ErrorCode:0x%X  " ,
    iVectorNumber,bAPICID,qwErrorCode);
    kPrintStringXY( 0, 2, vcBuffer );
    kSPrintf(vcBuffer,"     Task ID:0x%X",pstTask->stLink.qwID);
    kPrintStringXY(0,3,vcBuffer);
    kPrintStringXY( 0, 4, "====================================================" );

    if(pstTask->qwFlags & TASK_FLAGS_USERLEVEL)
    {
        kEndTask(pstTask->stLink.qwID);
        while(1)
        {
            ;
        }
    }
    else{
    while( 1 ) 
    {
        ;
    }
    }
}

/**
 *  공통으로 사용하는 인터럽트 핸들러 
 */
void kCommonInterruptHandler( int iVectorNumber )
{
    char vcBuffer[] = "[INT:  , ]";
    static int g_iCommonInterruptCount = 0;
    int iIRQ;

    //=========================================================================
    // 인터럽트가 발생했음을 알리려고 메시지를 출력하는 부분
    // 인터럽트 벡터를 화면 오른쪽 위에 2자리 정수로 출력
    vcBuffer[ 5 ] = '0' + iVectorNumber / 10;
    vcBuffer[ 6 ] = '0' + iVectorNumber % 10;
    // 발생한 횟수 출력
    vcBuffer[ 8 ] = '0' + g_iCommonInterruptCount;
    g_iCommonInterruptCount = ( g_iCommonInterruptCount + 1 ) % 10;
    kPrintStringXY( 70, 0, vcBuffer );
    //=========================================================================
    
    iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

    kSendEOI(iIRQ);
    kIncreaseInterruptCount(iIRQ);
    kProcessLoadBalancing(iIRQ);

}

/**
 *  키보드 인터럽트의 핸들러
 */
void kKeyboardHandler( int iVectorNumber )
{
    char vcBuffer[] = "[INT:  , ]";
    static int g_iKeyboardInterruptCount = 0;
    BYTE bTemp;
    int iIRQ;

    //=========================================================================
    // 인터럽트가 발생했음을 알리려고 메시지를 출력하는 부분
    // 인터럽트 벡터를 화면 왼쪽 위에 2자리 정수로 출력
    vcBuffer[ 5 ] = '0' + iVectorNumber / 10;
    vcBuffer[ 6 ] = '0' + iVectorNumber % 10;
    // 발생한 횟수 출력
    vcBuffer[ 8 ] = '0' + g_iKeyboardInterruptCount;
    g_iKeyboardInterruptCount = ( g_iKeyboardInterruptCount + 1 ) % 10;
    kPrintStringXY( 0, 0, vcBuffer );
    //=========================================================================

    // 키보드 컨트롤러에서 데이터를 읽어서 ASCII로 변환하여 큐에 삽입
    if( kIsOutputBufferFull() == TRUE )
    {
        if(kIsMouseDataInOutputBuffer() == FALSE)
        {
            bTemp = kGetKeyboardScanCode();
            kConvertScanCodeAndPutQueue(bTemp);
        }
        else
        {
            bTemp = kGetKeyboardScanCode();
            kAccumulateMouseDataAndPutQueue(bTemp);
        }
    }

    iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

    kSendEOI(iIRQ);
    kIncreaseInterruptCount(iIRQ);
    kProcessLoadBalancing(iIRQ);
}

/**
 *  타이머 인터럽트의 핸들러
 */
void kTimerHandler( int iVectorNumber )
{
    char vcBuffer[] = "[INT:  , ]";
    static int g_iTimerInterruptCount = 0;
    int iIRQ;
    BYTE bCurrentAPICID;

    //=========================================================================
    // 인터럽트가 발생했음을 알리려고 메시지를 출력하는 부분
    // 인터럽트 벡터를 화면 오른쪽 위에 2자리 정수로 출력
    vcBuffer[ 5 ] = '0' + iVectorNumber / 10;
    vcBuffer[ 6 ] = '0' + iVectorNumber % 10;
    // 발생한 횟수 출력
    vcBuffer[ 8 ] = '0' + g_iTimerInterruptCount;
    g_iTimerInterruptCount = ( g_iTimerInterruptCount + 1 ) % 10;
    kPrintStringXY( 70, 0, vcBuffer );
    //=========================================================================
    iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

    kSendEOI(iIRQ);
    kIncreaseInterruptCount(iIRQ);

    bCurrentAPICID = kGetAPICID();
    if(bCurrentAPICID == 0)
    {
        g_qwTickCount++;
        
    }
        kDecreaseProcessorTime(bCurrentAPICID);
        if(kIsProcessorTimeExpired(bCurrentAPICID) == TRUE)
        {
            kScheduleInInterrupt();
        }
}

/**
 *  Device Not Available 예외의 핸들러
 */
void kDeviceNotAvailableHandler( int iVectorNumber )
{
    TCB* pstFPUTask, * pstCurrentTask;
    QWORD qwLastFPUTaskID;
    BYTE bCurrentAPICID;

    //=========================================================================
    // FPU 예외가 발생했음을 알리려고 메시지를 출력하는 부분
    char vcBuffer[] = "[EXC:  , ]";
    static int g_iFPUInterruptCount = 0;

    // 예외 벡터를 화면 오른쪽 위에 2자리 정수로 출력
    vcBuffer[ 5 ] = '0' + iVectorNumber / 10;
    vcBuffer[ 6 ] = '0' + iVectorNumber % 10;
    // 발생한 횟수 출력
    vcBuffer[ 8 ] = '0' + g_iFPUInterruptCount;
    g_iFPUInterruptCount = ( g_iFPUInterruptCount + 1 ) % 10;
    kPrintStringXY( 0, 0, vcBuffer );    
    //=========================================================================
    
    // CR0 컨트롤 레지스터의 TS 비트를 0으로 설정
    bCurrentAPICID = kGetAPICID();
    kClearTS();

    // 이전에 FPU를 사용한 태스크가 있는지 확인하여, 있다면 FPU의 상태를 태스크에 저장
    qwLastFPUTaskID = kGetLastFPUUsedTaskID(bCurrentAPICID);
    pstCurrentTask = kGetRunningTask(bCurrentAPICID);
    
    // 이전에 FPU를 사용한 것이 자신이면 아무것도 안 함
    if( qwLastFPUTaskID == pstCurrentTask->stLink.qwID )
    {
        return ;
    }
    // FPU를 사용한 태스크가 있으면 FPU 상태를 저장
    else if( qwLastFPUTaskID != TASK_INVALIDID )
    {
        pstFPUTask = kGetTCBInTCBPool( GETTCBOFFSET( qwLastFPUTaskID ) );
        if( ( pstFPUTask != NULL ) && ( pstFPUTask->stLink.qwID == qwLastFPUTaskID ) )
        {
            kSaveFPUContext( pstFPUTask->vqwFPUContext );
        }
    }
    
    // 현재 태스크가 FPU를 사용한 적이 있는 지 확인하여 FPU를 사용한 적이 없다면 
    // 초기화하고, 사용한적이 있다면 저장된 FPU 콘텍스트를 복원
    if( pstCurrentTask->bFPUUsed == FALSE )
    {
        kInitializeFPU();
        pstCurrentTask->bFPUUsed = TRUE;
    }
    else
    {
        kLoadFPUContext( pstCurrentTask->vqwFPUContext );
    }
    
    // FPU를 사용한 태스크 ID를 현재 태스크로 변경
    kSetLastFPUUsedTaskID( bCurrentAPICID,pstCurrentTask->stLink.qwID );
}

/**
 *  하드 디스크에서 발생하는 인터럽트의 핸들러
 */
void kHDDHandler( int iVectorNumber )
{
    char vcBuffer[] = "[INT:  , ]";
    static int g_iHDDInterruptCount = 0;
    BYTE bTemp;
    int iIRQ;

    //=========================================================================
    // 인터럽트가 발생했음을 알리려고 메시지를 출력하는 부분
    // 인터럽트 벡터를 화면 왼쪽 위에 2자리 정수로 출력
    vcBuffer[ 5 ] = '0' + iVectorNumber / 10;
    vcBuffer[ 6 ] = '0' + iVectorNumber % 10;
    // 발생한 횟수 출력
    vcBuffer[ 8 ] = '0' + g_iHDDInterruptCount;
    g_iHDDInterruptCount = ( g_iHDDInterruptCount + 1 ) % 10;
    // 왼쪽 위에 있는 메시지와 겹치지 않도록 (10, 0)에 출력
    kPrintStringXY( 10, 0, vcBuffer );
    //=========================================================================
    // 첫 번째 PATA 포트의 인터럽트 발생 여부를 TRUE로 설정
    kSetHDDInterruptFlag( TRUE, TRUE );

    iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

    kSendEOI(iIRQ);
    kIncreaseInterruptCount(iIRQ);
    kProcessLoadBalancing(iIRQ);
}

void kMouseHandler(int iVectorNumber)
{
    char vcBuffer[] = "[INT:  , ]";
    static int g_iMouseInterruptCount = 0;
    BYTE bTemp;
    int iIRQ;
    vcBuffer[5] = '0'+iVectorNumber/10;
    vcBuffer[6] = '0' + iVectorNumber % 10;
    vcBuffer[8] = '0' + g_iMouseInterruptCount;
    g_iMouseInterruptCount = (g_iMouseInterruptCount + 1) % 10;
    kPrintStringXY(0,0,vcBuffer);

    if(kIsOutputBufferFull() == TRUE)
    {
        if(kIsMouseDataInOutputBuffer() == FALSE)
        {
            bTemp = kGetKeyboardScanCode();
            kConvertScanCodeAndPutQueue(bTemp);
        }
        else
        {
            bTemp = kGetKeyboardScanCode();
            kAccumulateMouseDataAndPutQueue(bTemp);
        }
    }
    iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;
    kSendEOI(iIRQ);
    kIncreaseInterruptCount(iIRQ);
    kProcessLoadBalancing(iIRQ);
}