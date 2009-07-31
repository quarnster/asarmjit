#include "as_jit_arm_op.h"
#include "armregistermanager.h"


#include "as_jit_arm.h"
#include <stdarg.h>
#include <vector>

ARMRegisterManager::ARMRegisterManager(asCJitArm *j)
: RegisterManager(j, 8,  ~CALLCONV_FREE_REGISTERMASK)
{
    usedRegisters[REG_R0] = RESERVED; // used as the "this" pointer for the AS register struct
}

ARMRegisterManager::~ARMRegisterManager()
{
}

void ARMRegisterManager::SaveRegister(int asRegister, int native)
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

void ARMRegisterManager::LoadRegister(int asRegister, int native)
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
