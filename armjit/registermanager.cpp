#include "as_jit_arm_op.h"
#include "registermanager.h"


#include "as_jit_arm.h"
#include <stdarg.h>
#include <vector>

#define REQUIRED_SCRATCH_REGISTERS 4

RegisterManager::RegisterManager(asCJitArm *j, int regCount, int sMask)
: jit(j)
{
    saveMask        = sMask;
    registerCount   = regCount;
    usedMask        = 0;
    mappedMask      = 0;
    lastUsedCount   = 0;
    lastUsed        = new int[registerCount];
    loadedRegisters = new int[registerCount];
    dirtyRegisters  = new bool[registerCount];
    usedRegisters   = new int[registerCount];
    for (int i = 0; i < registerCount; i++)
    {
        usedRegisters[i] = REGISTER_EMPTY;
    }
}

RegisterManager::~RegisterManager()
{
    delete[] lastUsed;
    delete[] loadedRegisters;
    delete[] dirtyRegisters;
    delete[] usedRegisters;
}

void RegisterManager::FreeRegister(int native)
{
    dirtyRegisters[native] = false;
    usedRegisters[native] = REGISTER_EMPTY;
}

void RegisterManager::FreeRegisters()
{
    for (int r = 0; r < registerCount; r++)
    {
        dirtyRegisters[r] = false;
        usedRegisters[r] = REGISTER_EMPTY;
    }
}
int RegisterManager::FindRegister(int asRegister)
{
    for (int r = 0; r < registerCount; r++)
    {
        if (usedRegisters[r] == asRegister)
        {
            return r;
            break;
        }
    }
    return REGISTER_EMPTY;
}

void RegisterManager::TouchRegister(int native)
{
    lastUsed[native] = lastUsedCount++;
}

void RegisterManager::UseRegister(int native, int as)
{
    if (saveMask & (1 << native))
        // Need to save this register as the call convention expects this.
        // The prologue stm instruction will be patched later
        usedMask |= (1 << native);
    usedRegisters[native] = as;
    TouchRegister(native);
}

int RegisterManager::GetUsedMask()
{
    return usedMask;
}

void RegisterManager::WriteTo(int native)
{
    dirtyRegisters[native] = true;
}

int RegisterManager::AllocateRegister(int asRegister, bool loadData, bool first, bool kickout)
{
    int r = 0;
    int unused = REGISTER_EMPTY;
    if (first)
    {
        for (; r < registerCount; r++)
        {
            if (usedRegisters[r] == REGISTER_EMPTY)
            {
                unused = r;
                break;
            }
        }
    }
    else
    {
        for (; r < registerCount; r++)
        {
            if (usedRegisters[r] == REGISTER_EMPTY && (unused == REGISTER_EMPTY || r >= REG_R12))
                unused = r;
            if (usedRegisters[r] == asRegister)
            {
                return r;
            }
        }
    }
    r = unused;
    if (r == REGISTER_EMPTY)
    {
        // Couldn't find an unused slot, so need to kick out another register
        int least = registerCount-REQUIRED_SCRATCH_REGISTERS;
        for (r = registerCount-REQUIRED_SCRATCH_REGISTERS; r < registerCount; r++)
        {
            if (lastUsed[r] < lastUsed[least])
            {
                least = r;
            }
        }
        r = least;
        if (dirtyRegisters[r])
        {
            dirtyRegisters[r] = false;
            TouchRegister(r);
            SaveRegister(usedRegisters[r], r);
        }
    }
    UseRegister(r, asRegister);
    if (loadData)
        LoadRegister(asRegister, r);
    return r;
}

void RegisterManager::FlushUnmappedRegisters()
{
    for (int i = 0; i < registerCount; i++)
    {
        if (!(mappedMask & (1<< i)) && usedRegisters[i] != REGISTER_EMPTY && usedRegisters[i] != RESERVED)
        {
            if (dirtyRegisters[i])
            {
                SaveRegister(usedRegisters[i], i);
            }
            FreeRegister(i);
        }
    }
}

void RegisterManager::CreateRegisterMap(std::vector<Block> &blocks, std::vector<ASRegister> &totalRegisterUsage)
{
    mappedMask = 0;
    // Figure out what registers are used for the whole of the bytecode
    for (std::vector<Block>::iterator it = blocks.begin(); it < blocks.end(); it++)
    {
        for (std::vector<ASRegister>::iterator reg = it->registersUsed.begin(); reg < it->registersUsed.end(); reg++)
        {
            ASRegister *tot = NULL;
            for (std::vector<ASRegister>::iterator reg2 = totalRegisterUsage.begin(); reg2 < totalRegisterUsage.end(); reg2++)
            {
                if (reg2->id == reg->id)
                {
                    tot = &(*reg2);
                    break;
                }
            }
            if (!tot)
            {
                ASRegister tmp(reg->id, reg->firstUseWriteTo);
                totalRegisterUsage.push_back(tmp);
                tot = &totalRegisterUsage[totalRegisterUsage.size()-1];
            }
            tot->writtenTo |= reg->writtenTo;
            tot->score += reg->score;
        }
    }
    // Sort by score
    for (unsigned int i = 0; i < totalRegisterUsage.size(); i++)
    {
        for (unsigned int j = i+1; j < totalRegisterUsage.size(); j++)
        {
            if (totalRegisterUsage[i].score < totalRegisterUsage[j].score)
            {
                ASRegister tmp = totalRegisterUsage[i];
                totalRegisterUsage[i] =  totalRegisterUsage[j];
                totalRegisterUsage[j]= tmp;
            }
        }
    }
    printf("REGISTERS USED\n");
    printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
    int cutOffIdx = totalRegisterUsage.size();
    if (totalRegisterUsage.size() > registerCount-1-REQUIRED_SCRATCH_REGISTERS)
    {
        // All registers don't fit, so we can't have a 1-1 mapping
        cutOffIdx = registerCount-1-REQUIRED_SCRATCH_REGISTERS;
    }
    
    for (int i = 0; i < cutOffIdx; i++)
    {
        totalRegisterUsage[i].nativeMapping = AllocateRegister(totalRegisterUsage[i].id, false);
        printf("AS reg: ");
        if (totalRegisterUsage[i].id < AS_SPECIAL_REG)
            printf("  v%d ", totalRegisterUsage[i].id);
        else if (totalRegisterUsage[i].id == AS_REGISTER1)
            printf("<register1> ");
        else if (totalRegisterUsage[i].id == AS_STACK_FRAME_POINTER)
            printf("<stackFramePointer> ");

        printf("native: %d, score: %d\n", totalRegisterUsage[i].nativeMapping, totalRegisterUsage[i].score);

        for (unsigned int j = 0; j < blocks.size(); j++)
        {
            blocks[j].SetRegisterMapping(totalRegisterUsage[i].id, totalRegisterUsage[i].nativeMapping);
        }
        mappedMask |= 1 << totalRegisterUsage[i].nativeMapping;
    }
    printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
}


