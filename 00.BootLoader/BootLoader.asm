[ORG 0x00] ; start address of code set 0x00
[BITS 16] ; code setting to 16-bit

SECTION .text

jmp 0x07c0:START ;copy 0x07c0 to CS segment and jump START LABLE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MINT64 OS env value
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;TOTALSECTORCOUNT: dw 1024
TOTALSECTORCOUNT: dw 0x02

KERNEL32SECTORCOUNT: dw 0x02 ;보호 모드 커널의 총 섹터 수
BOOTSTRAPPROCESSOR: db 0x01
STARTGRAPHICMODE:   db 0x01

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; CODE SECTION
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
START:
    mov ax, 0x07c0
    mov ds, ax
    mov ax, 0xb800
    mov es, ax

    ;create stack 64kb 0x0000:0000 ~ 0x0000:ffff area
    mov ax, 0x0000
    mov ss, ax      ;stack segment
    mov sp, 0xfffe
    mov bp, 0xfffe

    mov byte[BOOTDRIVE],dl

    mov si, 0

.SCREENCLEARLOOP:
    mov byte [es: si], 0
    
    mov byte [es: si+1], 0x0a
    
    add si, 2

    cmp si, 80*25*2

    jl .SCREENCLEARLOOP


    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; print start message top screen
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    push MESSAGE1
    push 0  ;set screen y value
    push 0  ;set screen x value

    call PRINTMESSAGE
    add sp, 6

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; print message "loading OS image"
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    push IMAGELOADINGMESSAGE
    push 1  ;set screen y value
    push 0  ;set screen x value
    call PRINTMESSAGE
    add sp,6

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Load OS Image on Disk
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; reset disk before read
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

RESETDISK:                          ;Start disk reset code
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; call BIOS Reset Function
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; service number 0, drive number (0=Floppy)

    mov ax, 0
    mov dl, byte[BOOTDRIVE]
    int 0x13
    ;if occur error, move to error process
    jc HANDLEDISKERROR

    mov ah,0x08
    mov dl, byte[BOOTDRIVE]
    int 0x13
    jc HANDLEDISKERROR

    mov byte[LASTHEAD],dh
    mov al,cl
    and al,0x3f

    mov byte[LASTSECTOR],al
    mov byte[LASTTRACK],ch

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Read Sector from Disk
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ;Address that copy disk content to memory(ES:BX) set 0x10000

    mov si, 0x1000
    mov es, si
    mov bx, 0x0000

    ;mov di, word [TOTALSECTORCOUNT]
    mov di, 1146


READDATA:                   ;start code that read disk
    ;Check read all Sector
    cmp di, 0
    je READEND
    sub di, 0x1

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; call BIOS Read Function
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    mov ah, 0x02 ; BIOS service number 2 (Read Sector) 
    mov al, 0x1  ; number of read sector 1
    mov ch, byte [TRACKNUMBER] ; set Track number to read 
    mov cl, byte [SECTORNUMBER] ; set Sector number to read
    mov dh, byte [HEADNUMBER]  ; set Head number to read
    mov dl, byte[BOOTDRIVE]
    mov dl, 0x00
    int 0x13 ; perform service interupt
    jc HANDLEDISKERROR

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; calc copy address and track, head, and sector address to copy
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    add si, 0x0020
    mov es, si      ;increase 1 sector (+512)

    mov al, byte [SECTORNUMBER]
    add al, 0x1
    mov byte [SECTORNUMBER], al
    cmp al, byte[LASTSECTOR]
    jle READDATA

    add byte[HEADNUMBER],0x01
    mov byte [SECTORNUMBER], 0x01

    mov al,byte[LASTHEAD]
    cmp byte[HEADNUMBER],al
    jg .ADDTRACK
    
    jmp READDATA


.ADDTRACK
    mov byte[HEADNUMBER],0x00
    add byte[TRACKNUMBER],0x01
    jmp READDATA

READEND:

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Print Message of Success OS Image
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    push LOADINGCOMPLETEMESSAGE
    push 1
    push 20
    call PRINTMESSAGE
    add sp, 6

    mov ax, 0x4F01
    mov cx, 0x117
    mov bx, 0x07E0
    mov es, bx
    mov di,0x00
    int 0x10
    cmp ax,0x004F
    jne VBEERROR

    cmp byte[STARTGRAPHICMODE], 0x00
    je JUMPTOPROTECTEDMODE
    mov ax, 0x4F02
    mov bx,0x4117

    int 0x10
    cmp ax,0x004F
    jne VBEERROR
    
    jmp JUMPTOPROTECTEDMODE

VBEERROR:
    push CHANGEGRAPHICMODEFAIL
    push 2
    push 0
    call PRINTMESSAGE
    add sp,6
    jmp $
JUMPTOPROTECTEDMODE:
    jmp 0x1000:0x0000
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; function code section
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; handle Disk Error function
HANDLEDISKERROR:
    push DISKERRORMESSAGE
    push 1                  ;set Y value
    push 20                 ;set X value
    call PRINTMESSAGE

    jmp $                   ;infinite loop in current position

;print message function
;   PARAM: x value, y value, stream
PRINTMESSAGE:
    push bp
    mov bp, sp

    push es
    push si
    push di
    push ax
    push cx
    push dx

    ;set video mode address  to ES segment register
    mov ax, 0xb800
    mov es, ax

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ;calc video memory address using x, y value
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    
    ;first get line address using y value

    mov ax, word [bp + 6]
    mov si, 160
    mul si
    mov di, ax

    ;finally get total address mul 2 using x value
    mov ax, word [bp + 4]
    mov si, 2
    mul si
    add di, ax

    ;address to print stream
    mov si, word [bp + 8]





.MESSAGELOOP:
    mov cl, byte [si]
    
    cmp cl, 0
    je .MESSAGEEND

    mov byte [es: di], cl

    add si, 1
    add di, 2

    jmp .MESSAGELOOP

.MESSAGEEND:
    pop dx
    pop cx
    pop ax
    pop di
    pop si
    pop es
    pop bp
    ret


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Data Section
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;BootLoader start message
MESSAGE1:   db  0

DISKERRORMESSAGE:   db 'Disk Error~!!', 0
IMAGELOADINGMESSAGE: db  0
LOADINGCOMPLETEMESSAGE: db 0
CHANGEGRAPHICMODEFAIL:  db 0
;related value to read Disk
SECTORNUMBER: db 0x02   ;Section that save Sector number to start OS Image
HEADNUMBER: db 0x00     ;Section that save Head number to start OS Image 
TRACKNUMBER: db 0x00    ;Section that save Track number to start OS Image

BOOTDRIVE:  db 0x00
LASTSECTOR: db 0x00
LASTHEAD:   db 0x00
LASTTRACK:  db 0x00
times 510 - ( $ -$$ )   db 0x00 ; $: current line address
                                ; $$: Start address of current Section(.text)
                                ; $ - $$: Offset based on current Section
                                ; 510 - ( $ - $$ ): until current address to 510
                                ; db 0x00: define 1bytes value to 0x00
                                ; perform time(loop)
                                ; fill value (0x00) current address to 510 address

db 0x55
db 0xAA ;0x55, 0xAA is BootLoader Signiture






