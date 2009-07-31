CXXFLAGS = -g -I../angelscript/sdk/angelscript/include -Iarmjit
LD = ld
LDFLAGS = -lstdc++ -langelscript
DELETER = rm -f

ifneq ($(ARCH),ppc)
	CC = arm-apple-darwin9-gcc-4.0.1
	CXX = arm-apple-darwin9-g++-4.0.1
	CXXFLAGS += -isysroot /Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS2.2.sdk
	LDFLAGS += -isysroot /Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS2.2.sdk -L../angelscript/sdk/angelscript/lib-ipod
else
	CC = gcc
	CXX = g++
	LDFLAGS += -L../angelscript/sdk/angelscript/lib
endif
 

OBJDIR = ipod
SRCNAMES = test.cpp utils.cpp \
	armjit/block.cpp \
	armjit/registermanager.cpp \
	armjit/asregister.cpp \
	armjit/armregistermanager.cpp \
	armjit/vfpregistermanager.cpp \
	armjit/as_jit_arm.cpp
SRCDIR = ./

OBJ = $(SRCNAMES:.cpp=.o)
platform=/Developer/Platforms/iPhoneOS.platform
allocate=${platform}/Developer/usr/bin/codesign_allocate 


BIN = asjittest
	
all: $(BIN)

$(BIN): $(OBJ)
	$(CC) -o $(BIN) $(OBJ) $(LDFLAGS)
	export CODESIGN_ALLOCATE=${allocate} && codesign -fs "Fredrik Ehnbom" $(BIN) 


%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

depend:
	makedepend $(SRCNAMES) -f Makefile.depend 2> /dev/null
	
clean:
	$(DELETER) $(OBJ) $(BIN) *.bak


.PHONY: all clean install uninstall
include Makefile.depend
