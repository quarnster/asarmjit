#ifndef __INCLUDED_REGISTERMANAGER_H
#define __INCLUDED_REGISTERMANAGER_H

#include "asregister.h"
#include "block.h"


class Block;
class asCJitArm;
class RegisterManager
{
public:
    RegisterManager(asCJitArm *j, int registerCount, int saveMask);
    virtual ~RegisterManager();

    int GetUsedMask();
    void FreeRegisters();
    void FreeRegister(int native);
    int FindRegister(int asRegister);
    void UseRegister(int native, int as);
    void TouchRegister(int native);

    virtual void LoadRegister(int asRegister, int native) = 0;
    virtual void SaveRegister(int asRegister, int native) = 0;

    void WriteTo(int native);
    int AllocateRegister(int asRegister, bool loadData = false, bool first = false, bool kickout =  false);
    void CreateRegisterMap(std::vector<Block> &blocks, std::vector<ASRegister> &totalRegisterUsage);

    void FlushUnmappedRegisters();
protected:
    int saveMask;
    int registerCount;
    int usedMask;
    int mappedMask;
    int lastUsedCount;
    int *lastUsed;
    int *loadedRegisters;
    bool *dirtyRegisters;
    int *usedRegisters;

    asCJitArm *jit;
};



#endif
