[BITS 32]

global kReadCPUID, kSwitchAndExecute64bitKernel ;export function name in C

SECTION .text


; return CPUID
; param: DWORD dwEAX, DWORD* pdwEAX, *pdwEBX, *pdwECX, *pdwEDX
kReadCPUID:
    push ebp
    mov ebp, esp
    push eax
    push ebx
    push ecx
    push edx
    push esi

    ;run CPUID command using EAX register
    mov eax, dword[ebp + 8]
    cpuid
    
    ;save return value to parameter
    ;*pdwEAX
    mov esi, dword[ebp + 12] ;param2 (pdwEAX)
    mov dword[esi], eax
    
    ;*pdwEBX
    mov esi, dword[ebp + 16]
    mov dword[esi], ebx

    ;*pdwECX
    mov esi, dword[ebp + 20]
    mov dword[esi], ecx
    
    ;*pdwEDX
    mov esi, dword[ebp + 24]
    mov dword[esi], edx

    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
    pop ebp
    ret

;Switch to IA-32e mode and run 64bit kernel
;param: none
kSwitchAndExecute64bitKernel:
    ;PAE bit in CR4 set 1
    mov eax, cr4
    or eax, 0x620 ;PAE bit(bit5) set 1
    mov cr4, eax

    ;CR3컨트롤 레스터에 PML4 테이블의 어드레스와 캐시 활성화
    mov eax, 0x100000 ;EAX레지스터에 PML4 테이블이 존재하는 0x100000를 저장
    mov cr3, eax

    ;IA32_EFER.LME를 1로 설정하여 IA-32e 모드를 활성화
    mov ecx, 0xc0000080 ;IA32_EFER MSR 레지스터의 어드레스를 저장
    rdmsr               ;MSR 레지스터를 읽기

    or eax, 0x0101      ;EAX 레지스터에 저장된 IA32_EFER MSR의 하위 32비트에서 LME 비트(비트8)을 1로 설정
    wrmsr

    ;CR0 컨트롤 레지스터를 NW bit(bit29) = 0, CD bit(bit30) = 0, PG bit(bit31) = 1로 설정하여 캐시 기능과 페이징 기능을 활성화
    mov eax, cr0
    or eax, 0xe000000e
    xor eax, 0x60000004
    mov cr0, eax

    jmp 0x08:0x200000 ;CS 세그머느 셀렉터를 IA-32e 모드용 코드 세그먼트 디스크립터로 교체하고 0x200000어드레스로 이동

    jmp $

    