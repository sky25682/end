/**
 *  file    InterruptHandler.h
 *  date    2009/01/24
 *  author  kkamagui 
 *          Copyright(c)2008 All rights reserved by kkamagui
 *  brief   인터럽트 및 예외 핸들러에 관련된 헤더 파일
 */

#ifndef __INTERRUPTHANDLER_H__
#define __INTERRUPTHANDLER_H__

#include "Types.h"
#include "MultiProcessor.h"

#define INTERRUPT_MAXVECTORCOUNT    16
#define INTERRUPT_LOADBALANCINGDIVIDOR  10

#define INTERRUPT_MAXVECTORCOUNT    16
#define INTERRUPT_LOADBALANCINGDIVIDOR  10

typedef struct kInterruptManagerStruct
{
    QWORD vvqwCoreInterruptCount[MAXPROCESSORCOUNT][INTERRUPT_MAXVECTORCOUNT];

    BOOL bUseLoadBalancing;
    BOOL bSymmetricIOMode;
}INTERRUPTMANAGER;




////////////////////////////////////////////////////////////////////////////////
//
// 함수
//
////////////////////////////////////////////////////////////////////////////////
void kCommonExceptionHandler( int iVectorNumber, QWORD qwErrorCode );
void kCommonInterruptHandler( int iVectorNumber );
void kKeyboardHandler( int iVectorNumber );
void kTimerHandler( int iVectorNumber );
void kDeviceNotAvailableHandler( int iVectorNumber );
void kHDDHandler( int iVectorNumber );
void kSetSymmetricIOMode(BOOL bSymmetricIOMode);
void kSetInterruptLoadBalancing(BOOL bUseLoadBalancing);
void kIncreaseInterruptCount(int iIRQ);
void kSendEOI(int iIRQ);
INTERRUPTMANAGER* kGetInterruptManager(void);
void kProcessLoadBalancing(int iIRQ);
void kMouseHandler(int iVectorNumber);

#endif /*__INTERRUPTHANDLER_H__*/
