#include "as_jit_arm_op.h"
#include "registermanager.h"

#define assert(x)

#include "as_jit_arm.h"
#include <stdarg.h>
#include <vector>

#define REQUIRED_SCRATCH_REGISTERS 4
#define MAX_REG REG_R12

static int usedMask = 0;
static int lastUsedCount = 0;
static int lastUsed[MAX_REG+1];
static int loadedRegisters[MAX_REG+1];
static bool dirtyRegisters[MAX_REG+1];
static int usedRegisters[MAX_REG+1];

RegisterManager::RegisterManager(asCJitArm *j)
: jit(j)
{
    for (int i = 0; i <= MAX_REG; i++)
    {
        usedRegisters[i] = REGISTER_EMPTY;
    }
    // Don't overwrite the native stack pointer...
    if (MAX_REG > REG_R13)
        usedRegisters[REG_R13] = RESERVED;
}

RegisterManager::~RegisterManager()
{
}

void RegisterManager::FreeRegister(int native)
{
    usedRegisters[native] = REGISTER_EMPTY;
}

void RegisterManager::FreeRegisters()
{
    for (int r = REG_R1; r <= MAX_REG; r++)
    {
        usedRegisters[r] = REGISTER_EMPTY;
    }
}
int RegisterManager::FindRegister(int asRegister)
{
    int r = REG_R1;
    for (; r <= MAX_REG; r++)
    {
        if (usedRegisters[r] == asRegister)
        {
            return r;
            break;
        }
    }
    return REGISTER_EMPTY;
}

void RegisterManager::UseRegister(int native, int as)
{
    if (native >= REG_R4 && native < REG_R12)
        // Need to save this register as the call convention expects this.
        // The prologue stm instruction will be patched later
        usedMask |= (1 << native);
    usedRegisters[native] = as;
    lastUsed[native] = lastUsedCount++;
}

int RegisterManager::GetUsedMask()
{
    return usedMask;
}

void RegisterManager::WriteTo(int native)
{
    dirtyRegisters[native] = true;
}

void RegisterManager::SaveRegister(int asRegister, int native)
{
    switch (asRegister)
    {
        case AS_REGISTER1:
        {
            jit->AddCode(arm_str(COND_AL, native, REG_R0, offsetof(asSVMRegisters, valueRegister), PRE_BIT|IMM_BIT));
            break;
        }
        default:
        {
            int fp = AllocateRegister(AS_STACK_FRAME_POINTER, true, false, true);
            jit->AddCode(arm_str(COND_AL, native, fp, -usedRegisters[native]*4, PRE_BIT|IMM_BIT));
            break;
        }
    }
}

void RegisterManager::LoadRegister(int asRegister, int native)
{
    switch (asRegister)
    {
        case AS_BC:
        {
            jit->AddCode(arm_ldr(COND_AL, native, REG_R0, offsetof(asSVMRegisters, programPointer), PRE_BIT|IMM_BIT));
            break;
        }
        case AS_STACK_FRAME_POINTER:
        {
            loadedRegisters[native] = 1;
            jit->AddCode(arm_ldr(COND_AL, native, REG_R0, offsetof(asSVMRegisters, stackFramePointer), PRE_BIT|IMM_BIT));
            break;
        }
        case AS_REGISTER1:
        {
            loadedRegisters[native] = 1;
            jit->AddCode(arm_ldr(COND_AL, native, REG_R0, offsetof(asSVMRegisters, valueRegister), PRE_BIT|IMM_BIT));
            break;
        }
        default:
        {
            loadedRegisters[native] = 1;
            int fp = AllocateRegister(AS_STACK_FRAME_POINTER, true, false, true);
            jit->AddCode(arm_ldr(COND_AL, native, fp, (-asRegister*4), PRE_BIT|IMM_BIT));
            break;
        }
    }
}

int RegisterManager::AllocateRegister(int asRegister, bool loadData, bool first, bool kickout)
{
    // r0 is reserved as the "this" pointer of the asCContext
    int r = REG_R1;
    int unused = REGISTER_EMPTY;
    if (first)
    {
        for (; r <= MAX_REG; r++)
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
        for (; r <= MAX_REG; r++)
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
        int least = MAX_REG-REQUIRED_SCRATCH_REGISTERS-1;
        for (r = MAX_REG-REQUIRED_SCRATCH_REGISTERS; r < MAX_REG; r++)
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
            SaveRegister(usedRegisters[r], r);
        }
    }
    UseRegister(r, asRegister);
    if (loadData)
        LoadRegister(asRegister, r);
    return r;
}

void RegisterManager::CreateRegisterMap(std::vector<Block> &blocks, std::vector<ASRegister> &totalRegisterUsage)
{
    // mappedRegisterMask = 0;
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
    if (totalRegisterUsage.size() > MAX_REG-REQUIRED_SCRATCH_REGISTERS)
    {
        // All registers don't fit, so we can't have a 1-1 mapping
        cutOffIdx = MAX_REG-REQUIRED_SCRATCH_REGISTERS;
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
        // mappedRegisterMask |= 1 << totalRegisterUsage[i].nativeMapping;
    }
    printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
}


