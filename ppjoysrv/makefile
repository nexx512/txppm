CC	=WINEPREFIX=/home/tomas/.winedevcpp/ wine ~/.winedevcpp/drive_c/Dev-Cpp/bin/gcc.exe
CFLAGS	?=-Wall -O2

CCFLAGS =-c -s -I. $(CFLAGS)
MAKEFILE=makefile
MAKEDEP=$(MAKEFILE)
OBJS	=main.o
BIN	=ppjoysrv.exe

ifneq ($(V),1)
	Q := @
endif

# targets
all: $(OBJS) $(BIN)

clean:
	$(Q)rm -f $(BIN) $(OBJS)
	@printf "  CLEAN\n";

.c.o:
	@printf "  CC  $(subst $(shell pwd)/,,$(@))\n";
	$(Q)$(CC) $(CCFLAGS) -c -o$@ $<

# explicit rules
$(BIN):	$(OBJS) $(MAKEDEP)
	@printf "  LD  $(subst $(shell pwd)/,,$(@))\n";
	$(Q)$(CC) -s $(OBJS) -o $(BIN) -L"C:/Dev-Cpp/lib" -lws2_32
 
