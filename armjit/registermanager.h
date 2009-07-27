#ifndef __INCLUDED_REGISTERMANAGER_H
#define __INCLUDED_REGISTERMANAGER_H

#include "as_jit_arm_op.h"
//#include "as_jit_arm.h"
#include "block.h"

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

    CALLCONV_FREE_REGISTERMASK = (1<<REG_R0)|(1<<REG_R1)|(1<<REG_R2)|(1<<REG_R3)|(1<<REG_R12)|(1<<REG_R14),
    REGISTER_EMPTY = 1 << 17,
    REGISTER_TEMP = 1 << 18,
    REGISTER_TEMP2
};

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

class Block;
class asCJitArm;
class RegisterManager
{
public:
    RegisterManager(asCJitArm *j);
    ~RegisterManager();

    int GetUsedMask();
    void FreeRegisters();
    void FreeRegister(int native);
    int FindRegister(int asRegister);
    void UseRegister(int native, int as);

    void LoadRegister(int asRegister, int native);
    void SaveRegister(int asRegister, int native);

    void WriteTo(int native);
    int AllocateRegister(int asRegister, bool loadData = false, bool first = false, bool kickout =  false);
    void CreateRegisterMap(std::vector<Block> &blocks, std::vector<ASRegister> &totalRegisterUsage);
private:
    asCJitArm *jit;
};



#endif
