INCDIRS=-I ../../inc/ -I ../../../../../../BREWUIWidgets_2.0.1/widgets/inc/ -I ../../../../../../BREWFontExtensions_2.1.5/btfe/inc/

CFLAGS=-c $(INCDIRS) -fno-builtin -ffunction-sections -O2 -DDYNAMIC_APP -DBREW -DHAVE_CONFIG_H -DDEBUG
LDFLAGS+= -nostdlib -nodefaultlibs -nostartfiles -Wl,--emit-relocs -Wl,-Map,brew_out/kphandset.map -Wl,--cref

OBJDIR=brew_out
OUTPUT=$(OBJDIR)/kphandset

SRC_KPHANDSET = \
	kpaudio.o \
	kpcamera.o \
	kpcutscene.o \
	kpdebug.o \
	kphandset.o \
	kphelpers.o \
	kpnetwork.o \
	kpstill.o \
	kptimeout.o \
	kpupdate.o

OBJS += $(addprefix $(OBJDIR)/, $(SRC_KPHANDSET))

all: $(MKDIR) $(OUTPUT).mod

$(OUTPUT).mod: $(OUTPUT).elf
	@echo [Creating mod $@]
	elf2mod.exe $^

$(OUTPUT).elf: $(OBJDIR)/AEEModGen.o $(OBJDIR)/AEEAppGen.o $(OBJDIR)/AEEMediaUtil.o $(OBJS)
	@echo [Linking $@]
	arm-none-eabi-gcc $(LDFLAGS) -T elf2mod.x -o $@ $^ -lgcc -lc 

$(MKDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: %.cpp %.h
	@echo [Compiling $<]
	arm-none-eabi-g++ $(CFLAGS) -c $< -o $@

$(OBJDIR)/AEEModGen.o: ../../src/AEEModGen.c
	arm-none-eabi-gcc $(CFLAGS) -std=c99 -o $(OBJDIR)/AEEModGen.o ../../src/AEEModGen.c

$(OBJDIR)/AEEAppGen.o: ../../src/AEEAppGen.c
	arm-none-eabi-gcc $(CFLAGS) -std=c99 -o $(OBJDIR)/AEEAppGen.o ../../src/AEEAppGen.c

$(OBJDIR)/AEEMediaUtil.o: ../../src/AEEMediaUtil.c
	arm-none-eabi-gcc $(CFLAGS) -std=c99 -o $(OBJDIR)/AEEMediaUtil.o ../../src/AEEMediaUtil.c