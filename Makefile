ASDIR = ${HOME}/code/3rdparty/angelscript
CXXFLAGS = -arch i386 -g -Wall -I${ASDIR}/sdk/angelscript/include -Iarmjit
LD = ld
LDFLAGS = -arch i386 -lstdc++ -L${ASDIR}/sdk/angelscript/lib -langelscript
DELETER = rm -f

OBJDIR = ipod
SRCNAMES = \
	armjit/block.cpp \
	armjit/registermanager.cpp \
	armjit/asregister.cpp \
	armjit/armregistermanager.cpp \
	armjit/vfpregistermanager.cpp \
	armjit/as_jit_arm.cpp \
	armjit/as_jit_arm_op.cpp \
	test.cpp utils.cpp
SRCDIR = ./

OBJ = $(SRCNAMES:.cpp=.o)


BIN = asjittest

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) -o $(BIN) $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	$(DELETER) $(OBJ) $(BIN) *.bak


.PHONY: all clean install uninstall
