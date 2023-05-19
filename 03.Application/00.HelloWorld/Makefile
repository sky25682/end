NASM64 = nasm -f elf64
GCC64 = gcc -c -m64 -ffreestanding -mcmodel=large -fno-pie
LD64 = ld -melf_x86_64 -T ../elf_x86_64.x -nostdlib -e _START -Ttext 0x0000
LDFLAGS = -lMINT64OS -L../../UserLibrary
CFLAGS = -I../../UserLibrary/Source

OBJCOPY64 = objcopy -j .text -j .data -j .rodata -j .bss

OBJECTDIRECTORY = Temp
SOURCEDIRECTORY = Source

all: prepare Output.elf

prepare:
	mkdir -p $(OBJECTDIRECTORY)
	
dep:
	@echo === Make Dependency File ===
	make -C $(OBJECTDIRECTORY) -f ../Makefile InternalDependency
	@echo === Dependency Search Complete ===

ExecuteInternalBuild: dep
	make -C $(OBJECTDIRECTORY) -f ../Makefile Main.elf

Output.elf: ExecuteInternalBuild
	$(OBJCOPY64) $(OBJECTDIRECTORY)/Main.elf $@

clean:
	rm -f *.elf
	rm -f $(OBJECTDIRECTORY)/*.*

CSOURCEFILES = $(wildcard ../$(SOURCEDIRECTORY)/*.c)
ASSEMBLYSOURCEFILES = $(wildcard ../$(SOURCEDIRECTORY)/*.asm)
COBJECTFILES = $(notdir $(patsubst %.c,%.o,$(CSOURCEFILES)))
ASSEMBLYOBJECTFILES = $(notdir $(patsubst %.asm,%.o,$(ASSEMBLYSOURCEFILES)))

# .c 파일을 .o 파일로 바꾸는 규칙 정의
%.o: ../$(SOURCEDIRECTORY)/%.c
	$(GCC64) -c $< $(CFLAGS)

# .asm 파일을 .o 파일로 바꾸는 규칙 정의
%.o: ../$(SOURCEDIRECTORY)/%.asm
	$(NASM64) -o $@ $<

# 의존성에 관련된 파일을 생성
InternalDependency:
	$(GCC64) -MM $(CSOURCEFILES) > Dependency.dep $(CFLAGS)

# 실제 응용 프로그램 실행 파일을 빌드
Main.elf: $(COBJECTFILES) $(ASSEMBLYOBJECTFILES)
	$(LD64) -r -o $@ $^ $(LDFLAGS)

# 현재 디렉터리의 파일 중, dependency 파일이 있으면 make에 포함	
ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif