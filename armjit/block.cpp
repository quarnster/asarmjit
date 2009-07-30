#include "block.h"

#define VERBOSE_DEBUG

Block::Block(asCJitArm *j, int start, int end)
{
    jit = j;
    byteCodeStart = start;
    byteCodeEnd = end;
    hasFlushed = false;
    hasRegistersToLoad = hasRegistersToSave = false;
    registerUseMask = 0;
    nativeOffset = -1;
    isSeparateEnd = false;
    isSeparateStart = false;
    endsWithCrossSuspendJump = false;
    isJumpTarget = false;
    registerStoreMask = registerLoadMask = 0;
    loadInstructions = 0;
    ended = false;
    hasUnimplementedBytecode = false;
}

Block::~Block()
{
}

void Block::AddRegister(int id, bool writtenTo)
{
    for (std::vector<ASRegister>::iterator it = registersUsed.begin(); it < registersUsed.end(); it++)
    {
        if (it->id == id)
        {
            it->score++;
            it->writtenTo |= writtenTo;
            hasRegistersToSave |= writtenTo;
            return;
        }
    }
    //if (!writtenTo && id < AS_SPECIAL_REG)
    //    AddRegister(AS_STACK_FRAME_POINTER, false);
    hasRegistersToSave |= writtenTo;
    hasRegistersToLoad |= !writtenTo;
    registersUsed.push_back(ASRegister(id, writtenTo));
}

int Block::GetNative(int asRegister, ASRegisterType type)
{
    ASRegister *reg = NULL;
    for (std::vector<ASRegister>::iterator it = registersUsed.begin(); it < registersUsed.end(); it++)
    {
        if (it->id == asRegister)
        {
            if (it->nativeMapping != REGISTER_EMPTY)
                return it->nativeMapping;
            reg = &(*it);
            break;
        }
    }
    int r = jit->registerManager->AllocateRegister(asRegister, reg, false, true);
    registerUseMask |= 1 << r;
    if (reg)
    {
        reg->nativeMapping = r;
        // TODO..
    }
    return r;
}

void Block::WroteToRegister(int asRegister)
{
    for (std::vector<ASRegister>::iterator it = registersUsed.begin(); it < registersUsed.end(); it++)
    {
        if (it->id == asRegister)
        {
            assert(it->nativeMapping != REGISTER_EMPTY);
            jit->registerManager->WriteTo(it->nativeMapping);
            it->actuallyWrittenTo = true;
            return;
        }
    }
    assert(0);
}

ASRegister *Block::GetRegister(int id)
{
    for (std::vector<ASRegister>::iterator it = registersUsed.begin(); it < registersUsed.end(); it++)
    {
        if (it->id == id)
            return &(*it);
    }
    return NULL;
}

void Block::SetRegisterMapping(int id, int native)
{
    for (unsigned int i = 0; i < registersUsed.size(); i++)
    {
        if (registersUsed[i].id == id)
        {
            if (!registersUsed[i].writtenTo)
                registerLoadMask |= 1 << native;
            else
                registerStoreMask |= 1 << native;
            registersUsed[i].nativeMapping = native;
            break;
        }
    }
    registerUseMask |= 1 << native;
}

void Block::UpdateBC()
{
    /*
    int instr = jit->GetImplementedInstructionCount();

    if (instr > 0)
    {
    */
        int bc_reg = jit->registerManager->AllocateRegister(AS_BC, false, true, true);
        int pos = jit->GetCurrentBytecodePosition();
/*
        if (pos == byteCodeStart && *((unsigned char* ) &jit->bytecode[pos]) == BC_SUSPEND)
        {
            // We want to resume 
            pos-= asCByteCode::SizeOfType(BCT_SUSPEND);
        }
*/
        jit->AddCode(arm_ldr(COND_AL, bc_reg, REG_PC, jit->AddData((int) (&jit->bytecode[pos])), PRE_BIT|IMM_BIT));
        jit->AddCode(arm_str(COND_AL, bc_reg, REG_R0, offsetof(asSVMRegisters, programPointer), PRE_BIT|IMM_BIT));
        jit->registerManager->FreeRegister(bc_reg);
    //}
}

void Block::Return(int flushMask)
{
/*
    if (flushMask == 0xffff)
    {
        for (unsigned int i = 0; i < registersUsed.size(); i++)
        {
            if (!registersUsed[i].actuallyWrittenTo && registersUsed[i].nativeMapping != REGISTER_EMPTY) 
                flushMask &= ~(1 << registersUsed[i].nativeMapping);
        }
    }

    Flush(flushMask, 0, COND_AL);
*/
    UpdateBC();
    //jit->AddCode(arm_b(COND_AL, 
//    jit->AddCode(arm_ldm(COND_AL, REG_SP, (registerUseMask&~CALLCONV_FREE_REGISTERMASK) | (1 << REG_PC), IA_BIT|WRITEBACK_BIT));
//    jit->currMachine[nativeOffset] |= (registerUseMask&~CALLCONV_FREE_REGISTERMASK);
}

void Block::Suspend()
{
/*
    int reg = GetNative(REGISTER_TEMP);

    // Test if we shouldn't suspend
    jit->AddCode(arm_ldr(COND_AL, reg, REG_R0, offsetof(asCContext, doProcessSuspend), IMM_BIT|PRE_BIT|BYTE_BIT));
    jit->AddCode(arm_cmp(COND_AL, reg, 0, IMM_BIT));
    jit->registerManager->FreeRegister(reg);

    int branchOffset = jit->currCodeOffset+jit->codelen;
    jit->AddCode(arm_b(COND_EQ, 0, 0));

    int flushMask = registerStoreMask;
    for (unsigned int i = 0; i < registersUsed.size(); i++)
    {
        if (registersUsed[i].firstUseWriteTo && registersUsed[i].nativeMapping != REGISTER_EMPTY) 
            flushMask &= ~(1 << registersUsed[i].nativeMapping);
    }
    Return(isCrossSuspendJumpTarget ? flushMask : 0);

    jit->currMachine[branchOffset] |= ((jit->currCodeOffset+jit->codelen-2) - branchOffset) & B_MASK;
*/
}

void Block::End()
{
/*
    if (!ended)
    {
        ended = true;
        int flushMask = registerStoreMask;
        for (unsigned int i = 0; i < registersUsed.size(); i++)
        {
            if (!registersUsed[i].actuallyWrittenTo && registersUsed[i].nativeMapping != REGISTER_EMPTY) 
                flushMask &= ~(1 << registersUsed[i].nativeMapping);
        }

        Flush(flushMask, 0, COND_AL);

        UpdateBC();
        jit->AddCode(arm_ldm(COND_AL, REG_SP, (registerUseMask&~CALLCONV_FREE_REGISTERMASK) | (1 << REG_LR), IA_BIT|WRITEBACK_BIT));
        jit->currMachine[nativeOffset] |= (registerUseMask&~CALLCONV_FREE_REGISTERMASK);
    }
*/
}

void Block::AddBlockGlue(Block *nextBlock)
{
    if (nextBlock->isSeparateStart)
    {
        End();
    }
}

void Block::Flush(int flushMask, int freeMask, int cond)
{
//    if (hasFlushed)
//        return;
    hasFlushed = true;
    int fp_reg = 0;
    // Flush the register values to the stack frame pointer
    for (std::vector<ASRegister>::iterator it = registersUsed.begin(); it < registersUsed.end(); it++)
    {
        if (it->nativeMapping != REGISTER_EMPTY && it->/*actuallyWrittenTo*/writtenTo && (flushMask & (1 << it->nativeMapping)))
        {
            if (it->id == AS_REGISTER1)
                jit->AddCode(arm_str(cond, it->nativeMapping, REG_R0, offsetof(asSVMRegisters, valueRegister), PRE_BIT|IMM_BIT));
            else
            {
                if (!fp_reg)
                    fp_reg = jit->registerManager->AllocateRegister(AS_STACK_FRAME_POINTER, true);
                jit->AddCode(arm_str(cond, it->nativeMapping, fp_reg, -it->id*4, PRE_BIT|IMM_BIT));
            }
            // it->actuallyWrittenTo = false;
        }
        
        if (freeMask & (1 << it->nativeMapping))
        {
            jit->registerManager->FreeRegister(it->nativeMapping);
            it->nativeMapping = REGISTER_EMPTY;
        }
    }
    if (fp_reg)
        jit->registerManager->FreeRegister(fp_reg);
}

void Block::Start()
{
#ifdef VERBOSE_DEBUG
    printf("New section: %d, %d\n", byteCodeStart, byteCodeEnd);
    printf("---------------\n");
#endif
/*
    if (!isSeparateStart)
        return;

    int prologue = arm_stm(COND_AL, REG_SP, 1 << (REG_LR), DB_BIT|WRITEBACK_BIT);
    jit->AddCode(prologue);

    int loadMask = 0xffff;
    for (std::vector<ASRegister>::iterator it2 = registersUsed.begin(); it2 < registersUsed.end(); it2++)
    {
        if (it2->nativeMapping != REGISTER_EMPTY)
        {
            jit->registerManager->FreeRegister(it2->nativeMapping);
            jit->registerManager->UseRegister(it2->nativeMapping, it2->id);
        }
    }

    for (std::vector<ASRegister>::iterator it2 = registersUsed.begin(); it2 < registersUsed.end(); it2++)
    {
        if (it2->nativeMapping != REGISTER_EMPTY)
        {
            if (!it2->firstUseWriteTo && (loadMask & (1 << it2->nativeMapping)))
            {
                jit->registerManager->LoadRegister(it2->id, it2->nativeMapping);
            }
            if (it2->id < AS_SPECIAL_REG)
                printf("  v%d ", it2->id);
            else if (it2->id == AS_REGISTER1)
                printf("<register1> ");
            else if (it2->id == AS_STACK_FRAME_POINTER)
                printf("<stackFramePointer> ");

            printf("(first written to: %d, written to at all: %d, score: %d, native: %d)\n", it2->firstUseWriteTo, it2->writtenTo, it2->score, it2->nativeMapping);
        }
    }

#ifdef VERBOSE_DEBUG
    printf("---------------\n");
#endif
*/
}
