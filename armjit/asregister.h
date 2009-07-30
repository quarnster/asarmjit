#ifndef __INCLUDED_ASREGISTER_H
#define __INCLUDED_ASREGISTER_H

enum 
{
    AS_GLOBAL_REG = 1 << 16,
    AS_SPECIAL_REG = 1 << 30,
    AS_STACK_POINTER,
    AS_STACK_FRAME_POINTER,
    AS_BC,
    AS_REGISTER1,
    AS_CONTEXT,
    RESERVED,
    
    REGISTER_EMPTY = 1 << 17,
    REGISTER_TEMP = 1 << 18,
    REGISTER_TEMP2
};


typedef enum
{
    ASRegisterType_int,
    ASRegisterType_float,
    ASRegisterType_double
} ASRegisterType;

class ASRegister
{
public:
    ASRegister(int i, bool w)
    : id(i), firstUseWriteTo(w), writtenTo(w), score(1), nativeMapping(REGISTER_EMPTY), actuallyWrittenTo(0)
    {
        
    }
    int id;
    int nativeMapping;
    
    int score; // How often the register is used
    bool writtenTo;
    bool actuallyWrittenTo;
    bool firstUseWriteTo;
};

#endif