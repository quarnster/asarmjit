
//
// as_jit_arm.h
//
// Implements an AngelScript to ARM machine code translator
// Written by Fredrik Ehnbom in June-August 2009


#ifndef AS_JIT_ARM_H
#define AS_JIT_ARM_H

#include "angelscript.h"
#include "vector"

#include "registermanager.h"
#include "settings.h"

BEGIN_AS_NAMESPACE

enum
{
    REVISIT_JUMP_BIT = 1<< 10,
};

class Block;
class RegisterManager;
class ASRegister;
class asCJitArm : public asIJITCompiler
{
public:
    asCJitArm(asIScriptEngine *engine, const Settings &settings = Settings());
    virtual ~asCJitArm();

    virtual int CompileFunction(asIScriptFunction *func, asJITFunction *output);
    virtual void ReleaseJITFunction(asJITFunction jitFunction);

    int GetImplementedInstructionCount();
    int GetCurrentBytecodePosition();
private:

    void    InsertFunctionPrologue(std::vector<ASRegister> &registerUsage);
    void    LoadConstantValue(int reg, int val);

    void    AddCode(int code);
    int     AddData(int data);

    void    SplitIntoBlocks(const asDWORD* bytecode, int bytecodeLen);

    void    FlushData();
    void    InsertFunctionEpilogue(std::vector<ASRegister> &registerUsage);

    int bytecodePos;
    Block *currBlock;
    std::vector<Block> blocks;
    int     m_refCount;
    asIScriptEngine *m_engine;

    RegisterManager *registerManager;
    RegisterManager *floatRegisterManager;

    asDWORD *bytecode;
    asDWORD *currMachine;
    int currData[256];
    int currCodeOffset;
    int prologueLen;
    int codelen;
    int datalen;
    int implementedInstructionSize;

    Settings    settings;


    friend class Block;
    friend class RegisterManager;
    friend class ARMRegisterManager;
    friend class VFPRegisterManager;
};

END_AS_NAMESPACE

#endif
