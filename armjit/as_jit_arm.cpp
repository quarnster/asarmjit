
//
// as_jit_arm.cpp
//
// Implements an AngelScript to ARM machine code translator
// Written by Fredrik Ehnbom in June-August 2009

#define AS_DEBUG 1
#include "as_jit_arm.h"
#include "as_jit_arm_op.h"

BEGIN_AS_NAMESPACE

#include "block.h"
#include "registermanager.h"
#include "armregistermanager.h"
#include "vfpregistermanager.h"

#ifdef _WIN32_WCE
#include <windows.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#endif

#define VERBOSE_DEBUG

// TODO: don't hardcode this
#define CODE_BLOCK_SIZE 4096 


asCJitArm::asCJitArm(asIScriptEngine *engine, const Settings &set)
: m_engine(engine), currMachine(NULL), settings(set)
{
    m_refCount = 1;
    m_engine->AddRef();
    registerManager = new ARMRegisterManager(this);
    floatRegisterManager = new VFPRegisterManager(this);
}

asCJitArm::~asCJitArm()
{
    delete floatRegisterManager;
    delete registerManager;
    m_engine->Release();
}

int asCJitArm::AddData(int data)
{
    for (int i = 0; i < datalen; i++)
    {
        if (currData[i] == data)
        {
            return i;
        }
    }
    int ret = datalen;
    currData[ret] = data;
    datalen++;
    return ret;
}

void asCJitArm::AddCode(int code)
{
#ifdef VERBOSE_DEBUG
    printf("        ; adding ");
    disasm(code);
#endif
    currMachine[currCodeOffset+codelen++] = code;
}

static void *CodeAlloc()
{
#ifdef _WIN32_WCE
    return VirtualAlloc(NULL,
                    CODE_BLOCK_SIZE,
                    MEM_COMMIT | MEM_RESERVE,
                    PAGE_EXECUTE_READWRITE);
#else
    return mmap(NULL,
            CODE_BLOCK_SIZE,
            PROT_READ | PROT_WRITE | PROT_EXEC,
            MAP_PRIVATE | MAP_ANON,
            -1,
            0);
#endif
}

static void CodeFree(void *mem)
{
#ifdef _WIN32_WCE
    VirtualFree(mem, 0, MEM_RELEASE);
#else
    munmap(mem, CODE_BLOCK_SIZE);
#endif
}

typedef struct
{
    int pos;
    bool isCrossSuspendTarget;
} JumpTarget;

void asCJitArm::SplitIntoBlocks(const asDWORD* bytecode, int bytecodeLen)
{
    int i = 0;
    int start = 0;
    int lastPos = 0;
    currBlock = NULL;

    // find all jump targets
    std::vector<JumpTarget> jumpTargets;
    while (i < bytecodeLen)
    {
        asDWORD opcode = *(unsigned char*)&bytecode[i];
        int size = asBCTypeSize[asBCInfo[opcode].type];

        switch (opcode)
        {
            case asBC_JMP:
            case asBC_JZ:
            case asBC_JNZ:
            case asBC_JS:
            case asBC_JNS:
            case asBC_JP:
            case asBC_JNP:
            {
                int arg = asBC_INTARG(&bytecode[i]);
                int start = i;
                int end = start+arg+size;
                if (end < start)
                {
                    int tmp = start;
                    start = end;
                    end = tmp;
                }
                if (*(unsigned char*) &bytecode[start] == asBC_SUSPEND)
                {
                    start += asBCTypeSize[asBCInfo[asBC_SUSPEND].type];
                }

                bool isCrossSuspendJump = false;
                while (start != end)
                {
                    asDWORD opcode = *(unsigned char*)&bytecode[start];
                    int size = asBCTypeSize[asBCInfo[opcode].type];

                    if (opcode == asBC_SUSPEND)
                    {
                        isCrossSuspendJump = true;
                        break;
                    }
                    start += size;
                }

                bool found = false;
                arg += size+i;
                for (unsigned int i = 0; i < jumpTargets.size(); i++)
                {
                    if (jumpTargets[i].pos == arg)
                    {
                        found = true;
                        jumpTargets[i].isCrossSuspendTarget |= isCrossSuspendJump;
                        break;
                    }
                }
                if (!found)
                {
                    JumpTarget t;
                    t.pos = arg;
                    t.isCrossSuspendTarget = isCrossSuspendJump;
                    jumpTargets.push_back(t);
                }
                break;
            }
            default:
                break;
        }
        i += size;
    }
    for (unsigned int i = 0; i < jumpTargets.size(); i++)
    {
        printf("jumpTarget[%d]: %d\n", i, jumpTargets[i].pos);
    }

    for (unsigned int i = 0; i < jumpTargets.size(); i++)
    {
        for (unsigned int j = i+1; j < jumpTargets.size(); j++)
        {
            if (jumpTargets[i].pos > jumpTargets[j].pos)
            {
                JumpTarget tmp = jumpTargets[i];
                jumpTargets[i] = jumpTargets[j];
                jumpTargets[j] = tmp;
            }
        }
    }
    for (unsigned int i = 0; i < jumpTargets.size(); i++)
    {
        printf("jumpTarget[%d]: %d\n", i, jumpTargets[i].pos);
    }

    // Split bytecode into logical blocks, starting at a jump target or a SUSPEND bytecode,
    // and ending at the bytecode before a SUSPEND or including a cross suspend jump bytecode.
    i = 0;
    int nextJump = 0;
    JumpTarget t;
    t.pos = 0xfffffff;// Something ridiculous so that atleast it isn't empty
    jumpTargets.push_back(t); 

    Block s(this, 0, 0);
    s.isSeparateStart = true;
    blocks.push_back(s);
    currBlock = &blocks[blocks.size()-1];
    start = 0;

    while (i <= bytecodeLen)
    {
        asDWORD opcode = asBC_SUSPEND;
        if (i < bytecodeLen)
            opcode = *(unsigned char*)&bytecode[i];
        int size = asBCTypeSize[asBCInfo[opcode].type];

        switch (asBCInfo[opcode].type)
        {
            case asBCTYPE_wW_ARG:
            case asBCTYPE_wW_W_ARG:
            case asBCTYPE_wW_QW_ARG:
            case asBCTYPE_wW_DW_ARG:
            {
                currBlock->AddRegister(asBC_SWORDARG0(&bytecode[i]), true);
                if (opcode == asBC_CpyRtoV4 || opcode == asBC_LdGRdR4)
                    currBlock->AddRegister(AS_REGISTER1);
                break;
            }
            case asBCTYPE_rW_ARG:
            case asBCTYPE_rW_DW_ARG:
            {
                if (opcode == asBC_CMPIi || opcode == asBC_CMPi)
                {
                    currBlock->AddRegister(AS_REGISTER1, true);
                }
                else if (opcode == asBC_CpyVtoR4)
                {
                    currBlock->AddRegister(AS_REGISTER1, true);
                }

                currBlock->AddRegister(asBC_SWORDARG0(&bytecode[i]));
                if (opcode == asBC_IncVi)
                    currBlock->AddRegister(asBC_SWORDARG0(&bytecode[i]), true);

                break;
            }
            case asBCTYPE_wW_rW_rW_ARG:
            {
                currBlock->AddRegister(asBC_SWORDARG2(&bytecode[i]));
                currBlock->AddRegister(asBC_SWORDARG1(&bytecode[i]));
                currBlock->AddRegister(asBC_SWORDARG0(&bytecode[i]), true);

                break;
            }
            case asBCTYPE_wW_rW_DW_ARG:
            case asBCTYPE_wW_rW_ARG:
            {
                currBlock->AddRegister(asBC_SWORDARG1(&bytecode[i]));
                currBlock->AddRegister(asBC_SWORDARG0(&bytecode[i]), true);
                break;
            }
            case asBCTYPE_rW_rW_ARG:
            {
                currBlock->AddRegister(asBC_SWORDARG1(&bytecode[i]));
                currBlock->AddRegister(asBC_SWORDARG0(&bytecode[i]));
                break;
            }
            case asBCTYPE_DW_ARG:
            {
                switch (opcode)
                {
                case asBC_JZ:
                    // currBlock->AddRegister(AS_BC);
                    currBlock->AddRegister(AS_REGISTER1);
                    break;
                }
                break;
            }
            case asBCTYPE_NO_ARG:
            {
                if (opcode == asBC_TS || opcode == asBC_ClrHi)
                    currBlock->AddRegister(AS_REGISTER1, true);
                break;
            }
        }

        bool doBlockBreak = false;
        bool isCrossSuspendJump = false;
        switch (opcode)
        {
            //case asBC_JitEntry:
            //    doBlockBreak = true;
            //    break;
            case asBC_JMP:
            case asBC_JZ:
            case asBC_JNZ:
            case asBC_JS:
            case asBC_JNS:
            case asBC_JP:
            case asBC_JNP:
            {
                doBlockBreak = true;
                int start = i;
                int end = start+asBC_INTARG(&bytecode[i])+size;
                if (end < start)
                {
                    int tmp = start;
                    start = end;
                    end = tmp;
                }
                if (*(unsigned char*) &bytecode[start] == asBC_SUSPEND)
                {
                    start += asBCTypeSize[asBCInfo[asBC_SUSPEND].type];
                }

                while (start != end)
                {
                    asDWORD opcode = *(unsigned char*)&bytecode[start];
                    int size = asBCTypeSize[asBCInfo[opcode].type];

                    if (opcode == asBC_SUSPEND)
                    {
                        isCrossSuspendJump = true;
                        break;
                    }
                    start += size;
                }
                break;
            }
            default:
                doBlockBreak = false;
                break;
        }

        bool isJumpTarget = false;
        bool isCrossSuspendJumpTarget = false;

        if (jumpTargets[nextJump].pos == i+size)
        {
            isCrossSuspendJumpTarget = jumpTargets[nextJump].isCrossSuspendTarget;
            nextJump++;
            doBlockBreak = true;
            isJumpTarget = true;
        }

        bool isSuspend = false;
        if (i+size < bytecodeLen && *(unsigned char*)&bytecode[i+size] == asBC_JitEntry)
        {
            isSuspend = true;
            doBlockBreak = true;
        }

        if (doBlockBreak || (currBlock && currBlock->byteCodeEnd && i >= currBlock->byteCodeEnd))
        {
            bool redo = false;
            if (0 && currBlock->byteCodeStart == i-1)
            {
                // Block only contains suspend. redo this block
                redo = true;
                currBlock->byteCodeStart = i+1;
                currBlock->registersUsed.clear();
            }
            else
            {
                currBlock->isCrossSuspendJumpEnd = isCrossSuspendJump;
                currBlock->isSeparateEnd = isSuspend;
                currBlock->byteCodeEnd = i;
                /*
                if (currBlock->registersUsed.size() > MAX_REG-REQUIRED_SCRATCH_REGISTERS)
                {
                    int size = currBlock->registersUsed.size();
                    // Need to split this block
                    redo = true;
                    currBlock->byteCodeEnd = currBlock->byteCodeStart + ((currBlock->byteCodeEnd - currBlock->byteCodeStart) >> 1);
                    currBlock->registersUsed.clear();
                }
                */
            }
            if (!redo)
            {
                int off = i;
                off += size;
                Block s(this, off, 0);
                s.isSeparateStart = isSuspend | isCrossSuspendJumpTarget | isCrossSuspendJump;
                s.isJumpTarget = isJumpTarget;
                s.isCrossSuspendJumpTarget = isCrossSuspendJumpTarget;
                blocks.push_back(s);
                currBlock = &blocks[blocks.size()-1];
                start = i;
            }
            else
            {
                lastPos = currBlock->byteCodeStart;
                i = currBlock->byteCodeStart;
                continue;
            }
        }
        lastPos = i;
        i += size;
    }
    currBlock->byteCodeEnd = i;
    for (unsigned int i = 0; i < blocks.size(); i++)
    {
        printf("blocks[%d]: %d, %d, jumpTarget: %d, suspendStart: %d, suspendEnd: %d\n"
               "              isCrossSuspendJumpTarget: %d, isCrossSuspendJumpEnd: %d\n",
            i, blocks[i].byteCodeStart, blocks[i].byteCodeEnd,
            blocks[i].isJumpTarget, blocks[i].isSeparateStart, blocks[i].isSeparateEnd,
            blocks[i].isCrossSuspendJumpTarget, blocks[i].isCrossSuspendJumpEnd
            );
        for (unsigned int j = 0; j < blocks[i].registersUsed.size(); j++)
        {
            printf("    register %d\n", blocks[i].registersUsed[j].id);
        }
    }

    // Code block that flushes the temp variables out to the VM
    Block s2(this, bytecodeLen, bytecodeLen);
    s2.isJumpTarget = true;
    blocks.push_back(s2);

    // Code block that flushes the value register, but skips the temp variables.
    // This should only be jumped to by the RET bytecode.
    Block s3(this, bytecodeLen+1, bytecodeLen+1);
    s3.isJumpTarget = true;
    blocks.push_back(s3);
}

int asCJitArm::GetImplementedInstructionCount()
{
    return implementedInstructionSize;
}

void asCJitArm::LoadConstantValue(int reg, int arg)
{
    if (arg >= 0 && arg <= 255)
    {
        // argument can be set with the mov instruction
        AddCode(arm_mov(COND_AL, reg, arg, IMM_BIT));
    }
    else if (arg < 0 && arg >= -256)
    {
        // argument can be set with the mvn instruction
        AddCode(arm_mvn(COND_AL, reg, (~arg)&0xff, IMM_BIT));
    }
    else
    {
        // need to load constant from memory
        AddCode(arm_ldr(COND_AL, reg, REG_PC, AddData(arg), PRE_BIT|IMM_BIT));
    }
}

void asCJitArm::InsertFunctionPrologue(std::vector<ASRegister> &registers)
{
    AddCode(arm_stm(COND_AL, REG_SP, (1 << (REG_LR)) | 1 << (REG_R1), DB_BIT|WRITEBACK_BIT));
    if (settings.regHandling == GLOBAL_LOAD_STORE)
    {
        for (unsigned int i = 0; i < registers.size(); i++)
        {
            if (registers[i].nativeMapping == REGISTER_EMPTY)
                break;
            registerManager->LoadRegister(registers[i].id, registers[i].nativeMapping);
        }
    }
    int reg = registerManager->AllocateRegister(REGISTER_TEMP, false, true, true);
    AddCode(arm_ldr(COND_AL, reg, REG_SP, 4, IMM_BIT));
    AddCode(arm_add(COND_AL, reg, REG_PC, reg, arm_shift_op2reg(SHIFT_LSL, 2, 0)));
    AddCode(arm_bx(COND_AL, reg, 0));
    registerManager->FreeRegister(reg);
    registerManager->FlushUnmappedRegisters();
}

static void PrintByteCode(const asDWORD *bytecode)
{
    asDWORD opcode = *(asBYTE*)bytecode;
    printf("%-10s", asBCInfo[opcode].name);
    switch (asBCInfo[opcode].type)
    {
        case asBCTYPE_W_ARG:
        {
            int arg = asBC_WORDARG0(bytecode);
            printf("0x%x (%d)\n", arg, arg);
            break;
        }
        case asBCTYPE_wW_W_ARG:
        {
            int d   = asBC_SWORDARG0(bytecode);
            int arg = asBC_WORDARG1(bytecode);
            printf("v%d, 0x%x (%d)\n", d, arg, arg);
            break;

        }
        case asBCTYPE_wW_DW_ARG:
        case asBCTYPE_rW_DW_ARG:
        {
            int d   = asBC_SWORDARG0(bytecode);
            int arg = asBC_DWORDARG(bytecode);
            printf("v%d, 0x%x (%d, %f)\n", d, arg, arg, *(float*) &arg);
            break;

            break;
        }
        case asBCTYPE_DW_ARG:
        {
            int arg = asBC_DWORDARG(bytecode);
            printf("0x%x (%d, %f)\n", arg, arg, *(float*) &arg);
            break;
        }
        case asBCTYPE_wW_ARG:
        case asBCTYPE_rW_ARG:
        {
            int d   = asBC_SWORDARG0(bytecode);
            printf("v%d\n", d);
            break;
        }
        case asBCTYPE_rW_rW_ARG:
        case asBCTYPE_wW_rW_ARG:
        {
            int d   = asBC_SWORDARG0(bytecode);
            int op1 = asBC_SWORDARG1(bytecode);
            printf("v%d, v%d\n", d, op1);
            break;
        }
        case asBCTYPE_wW_rW_DW_ARG:
        {
            int d   = asBC_SWORDARG0(bytecode);
            int op1 = asBC_SWORDARG1(bytecode);
            int op2 = asBC_INTARG(bytecode+1);
            printf("v%d, v%d, 0x%x\n", d, op1, op2);
            break;
        }
        case asBCTYPE_wW_rW_rW_ARG:
        {
            int d   = asBC_SWORDARG0(bytecode);
            int op1 = asBC_SWORDARG1(bytecode);
            int op2 = asBC_SWORDARG2(bytecode);
            printf("v%d, v%d, v%d\n", d, op1, op2);
            break;
        }
        default:
            printf("\n");
            break;
    }
}

static void GetConditions(asDWORD testopcode, int &cond1, int &cond2)
{
    switch (testopcode)
    {
        case asBC_TS:
            cond1 = COND_MI;
            cond2 = COND_PL;
            break;
        case asBC_TNS:
            cond1 = COND_PL;
            cond2 = COND_MI;
            break;
        case asBC_TZ:
            cond1 = COND_EQ;
            cond2 = COND_NE;
            break;
        case asBC_TNZ:
            cond1 = COND_NE;
            cond2 = COND_EQ;
            break;
        case asBC_TP:
            cond1 = COND_GT;
            cond2 = COND_LE;
            break;
        case asBC_TNP:
            cond1 = COND_LE;
            cond2 = COND_GT;
            break;            
        default:
            assert(0);
            break;
    }    
}

int asCJitArm::StartCompile(const asDWORD * bytecode, asUINT bytecodeLen, asJITFunction *output)
{
    this->bytecode = bytecode;
    currBlock = NULL;

    currMachine = (asDWORD*) CodeAlloc();
    bytecodePos = 0;
    currCodeOffset = 0;
    codelen = 0;
    datalen = 0;
    implementedInstructionSize = 0;

    std::vector<ASRegister> registerUsage;
    SplitIntoBlocks(bytecode, bytecodeLen);
    registerManager->CreateRegisterMap(blocks, registerUsage);

    InsertFunctionPrologue(registerUsage);
    prologueLen = codelen;

    bool parseCode = false;
    int blockPos = 0;

    currBlock = &blocks[blockPos];
    currBlock->nativeOffset = 0;
    Block * lastBlock = NULL;
    while (bytecodePos < bytecodeLen)
    {
        asDWORD opcode = *(unsigned char*)bytecode;
        int size = asBCTypeSize[asBCInfo[opcode].type];

        if (bytecodePos > currBlock->byteCodeEnd)
        {
            currBlock->End();
            lastBlock = currBlock;
            blockPos++;
            currBlock = &blocks[blockPos];
        }
        if (bytecodePos == currBlock->byteCodeStart)
        {
            currBlock->nativeOffset = currCodeOffset + codelen - prologueLen;

            parseCode = true;
            currBlock->Start();
            currBlock->loadInstructions = (currCodeOffset + codelen) - currBlock->nativeOffset-prologueLen;
        }
        if (parseCode)
        {
            implementedInstructionSize += size;
#ifdef VERBOSE_DEBUG
            PrintByteCode(bytecode);
#endif

            switch (opcode)
            {
                case asBC_PUSH:
                {
                    int amount = asBC_WORDARG0(bytecode);
                    int sp_reg = currBlock->GetNative(AS_STACK_POINTER);
                    AddCode(arm_ldr(COND_AL, sp_reg, REG_R0, offsetof(asSVMRegisters, stackPointer), PRE_BIT|IMM_BIT));
                    AddCode(arm_sub(COND_AL, sp_reg, sp_reg, amount*4, IMM_BIT));
                    AddCode(arm_str(COND_AL, sp_reg, REG_R0, offsetof(asSVMRegisters, stackPointer), PRE_BIT|IMM_BIT));
                    registerManager->FreeRegister(sp_reg);
		            break;
                }
                case asBC_PshC4:
                {
                    int sp_reg = currBlock->GetNative(AS_STACK_POINTER);
                    AddCode(arm_ldr(COND_AL, sp_reg, REG_R0, offsetof(asSVMRegisters, stackPointer), PRE_BIT|IMM_BIT));
                    AddCode(arm_sub(COND_AL, sp_reg, sp_reg, 4, IMM_BIT));
                    AddCode(arm_str(COND_AL, sp_reg, REG_R0, offsetof(asSVMRegisters, stackPointer), PRE_BIT|IMM_BIT));

                    int reg = currBlock->GetNative(REGISTER_TEMP);
                    LoadConstantValue(reg, asBC_DWORDARG(bytecode));
                    AddCode(arm_str(COND_AL, reg, sp_reg, 0, PRE_BIT|IMM_BIT));

                    registerManager->FreeRegister(reg);
                    registerManager->FreeRegister(sp_reg);
		            break;
                }

                case asBC_PshV4:
                {
                    int sp_reg = currBlock->GetNative(AS_STACK_POINTER);
                    AddCode(arm_ldr(COND_AL, sp_reg, REG_R0, offsetof(asSVMRegisters, stackPointer), PRE_BIT|IMM_BIT));
                    AddCode(arm_sub(COND_AL, sp_reg, sp_reg, 4, IMM_BIT));
                    AddCode(arm_str(COND_AL, sp_reg, REG_R0, offsetof(asSVMRegisters, stackPointer), PRE_BIT|IMM_BIT));

                    int reg = currBlock->GetNative(asBC_SWORDARG0(bytecode));
                    AddCode(arm_str(COND_AL, reg, sp_reg, 0, PRE_BIT|IMM_BIT));

                    registerManager->FreeRegister(sp_reg);
		            break;
                }
                case asBC_PshG4:
                {
                    int sp_reg = currBlock->GetNative(AS_STACK_POINTER);
                    AddCode(arm_ldr(COND_AL, sp_reg, REG_R0, offsetof(asSVMRegisters, stackPointer), PRE_BIT|IMM_BIT));
                    AddCode(arm_sub(COND_AL, sp_reg, sp_reg, 4, IMM_BIT));
                    AddCode(arm_str(COND_AL, sp_reg, REG_R0, offsetof(asSVMRegisters, stackPointer), PRE_BIT|IMM_BIT));

                    int reg = currBlock->GetNative(REGISTER_TEMP);
                    
                    AddCode(arm_ldr(COND_AL, reg, REG_R0, offsetof(asSVMRegisters, globalVarPointers), IMM_BIT|PRE_BIT));
                    AddCode(arm_ldr(COND_AL, reg, reg, asBC_WORDARG0(bytecode)*4, IMM_BIT|PRE_BIT));
                    AddCode(arm_ldr(COND_AL, reg, reg, 0, IMM_BIT|PRE_BIT));
                    AddCode(arm_str(COND_AL, reg, sp_reg, 0, PRE_BIT|IMM_BIT));
                    registerManager->FreeRegister(reg);
                    break;
                }
                case asBC_CpyVtoV4:
                {
                    int asdst = asBC_SWORDARG0(bytecode);
                    int src = asBC_SWORDARG1(bytecode);

                    int dst = currBlock->GetNative(asdst, true);
                    src = currBlock->GetNative(src);
                    
                    AddCode(arm_mov(COND_AL, dst, src, 0));
                    currBlock->WroteToRegister(asdst);
                    break;
                }
                case asBC_CpyGtoV4:
                {
                    int reg = currBlock->GetNative(asBC_SWORDARG0(bytecode));
                    AddCode(arm_ldr(COND_AL, reg, REG_R0, offsetof(asSVMRegisters, globalVarPointers), IMM_BIT|PRE_BIT));
                    AddCode(arm_ldr(COND_AL, reg, reg, asBC_WORDARG1(bytecode)*4, IMM_BIT|PRE_BIT));
                    AddCode(arm_ldr(COND_AL, reg, reg, 0, IMM_BIT|PRE_BIT));
                    // registerManager->FreeRegister(reg);

                    break;
                }
                /*
                case asBC_CpyVtoG4:
                {
                    //*(asDWORD*)module->globalVarPointers[WORDARG0(l_bc)] = *(asDWORD*)(l_fp - asBC_SWORDARG1(l_bc));
                    int reg = currBlock->GetNative(REGISTER_TEMP);
                    int reg2 = currBlock->GetNative(REGISTER_TEMP2);
                    AddCode(arm_ldr(COND_AL, reg, REG_R0, offsetof(asCContext, module), IMM_BIT|PRE_BIT));
                    AddCode(arm_ldr(COND_AL, reg, reg, offsetof(asCModule, globalVarPointers), IMM_BIT|PRE_BIT));


                    int arg = asBC_SWORDARG1(bytecode);
                    LoadConstantValue(reg2, arg);

                    AddCode(arm_str(COND_AL, reg2, reg, asBC_WORDARG0(bytecode), PRE_BIT));
                    registerManager->FreeRegister(reg);
                    registerManager->FreeRegister(reg2);
                    break;
                }
                */

                case asBC_LdGRdR4:
                {
                    int reg = currBlock->GetNative(AS_REGISTER1, true);
                    int reg2 = currBlock->GetNative(asBC_SWORDARG0(bytecode), true);

                    AddCode(arm_ldr(COND_AL, reg, REG_R0, offsetof(asSVMRegisters, globalVarPointers), IMM_BIT|PRE_BIT));
                    AddCode(arm_ldr(COND_AL, reg, reg, asBC_WORDARG1(bytecode)*4, IMM_BIT|PRE_BIT));
                    AddCode(arm_ldr(COND_AL, reg2, reg, 0, IMM_BIT|PRE_BIT));
                    //*(void**)&register1 = module->globalVarPointers[WORDARG1(l_bc)];
		            //*(l_fp - asBC_SWORDARG0(l_bc)) = **(asDWORD**)&register1;
                    currBlock->WroteToRegister(AS_REGISTER1);
                    currBlock->WroteToRegister(asBC_SWORDARG0(bytecode));

                    break;
                }
                case asBC_WRTV4:
                {
                    int reg = currBlock->GetNative(AS_REGISTER1);
                    int reg2= currBlock->GetNative(asBC_SWORDARG0(bytecode));
                    AddCode(arm_str(COND_AL, reg2, reg, 0, IMM_BIT|PRE_BIT));
                    break;
                }
                case asBC_CpyRtoV4:
                {
                    int asdst = asBC_SWORDARG0(bytecode);
                    int src = currBlock->GetNative(AS_REGISTER1);

                    int dst = currBlock->GetNative(asdst, true);
                    
                    AddCode(arm_mov(COND_AL, dst, src, 0));
                    currBlock->WroteToRegister(asdst);
                    break;
                }
                case asBC_CMPi:
                {
                    AddCode(arm_cmp(COND_AL, currBlock->GetNative(asBC_SWORDARG0(bytecode)), currBlock->GetNative(asBC_SWORDARG1(bytecode)), 0));

                    int reg = currBlock->GetNative(AS_REGISTER1, true);
                    AddCode(arm_mov(COND_EQ, reg, 0, IMM_BIT));
                    AddCode(arm_mvn(COND_LT, reg, 1, IMM_BIT));
                    AddCode(arm_mov(COND_GT, reg, 1, IMM_BIT));
                    currBlock->WroteToRegister(AS_REGISTER1);
                    break;
                }
                case asBC_CMPIi:
                {
                    int asReg = asBC_SWORDARG0(bytecode);
                    int reg = currBlock->GetNative(asReg);
                    int imm = asBC_DWORDARG(bytecode);
                    if (imm >= 0 && imm <= 255)
                    {
                        AddCode(arm_cmp(COND_AL, reg, imm, IMM_BIT));
                    }
                    else if (imm < 0 && imm > -255)
                    {
                        AddCode(arm_cmn(COND_AL, reg, -imm, IMM_BIT));
                    }
                    else
                    {
                        int reg2 = currBlock->GetNative(REGISTER_TEMP);
                        AddCode(arm_ldr(COND_AL, reg2, REG_PC, AddData(imm), PRE_BIT|IMM_BIT));
                        AddCode(arm_cmp(COND_AL, reg, reg2, 0));
                        registerManager->FreeRegister(reg2);
                    }

                    reg = currBlock->GetNative(AS_REGISTER1, true);

                    const asDWORD *bc = bytecode+size;
                    int testopcode = *(asBYTE*) (bc);
                    if (testopcode == asBC_TS || testopcode == asBC_TZ || testopcode == asBC_TP || testopcode == asBC_TNP || testopcode == asBC_TNS || testopcode == asBC_TNZ)
                    {
                        int cond1 = 0;
                        int cond2 = 0;

                        GetConditions(testopcode, cond1, cond2);

                        bc += asBCTypeSize[asBCInfo[testopcode].type];

                        if (*(asBYTE*) (bc) == asBC_ClrHi)
                        {
                            bc += asBCTypeSize[asBCInfo[asBC_ClrHi].type];
                            int jumpopcode = *(asBYTE*) (bc);
                            if (jumpopcode == asBC_JZ)
                            {
                                int jcond = cond2;

                                PrintByteCode(bytecode+size);
                                size += asBCTypeSize[asBCInfo[testopcode].type];
                                PrintByteCode(bytecode+size);
                                size += asBCTypeSize[asBCInfo[asBC_ClrHi].type];
                                PrintByteCode(bytecode+size);
                                size += asBCTypeSize[asBCInfo[jumpopcode].type];
                                int target = asBC_INTARG(bc);

                                //AddCode(arm_mov(cond1, reg, 1, IMM_BIT));
                                //AddCode(arm_mov(cond2, reg, 0, IMM_BIT));
                                //currBlock->WroteToRegister(AS_REGISTER1);
                                currBlock->Flush();
                                AddCode(arm_b(jcond, (bytecodePos+size+target)*4, REVISIT_JUMP_BIT));
                                break;
                            }
                        }
                    }

                    AddCode(arm_mov(COND_EQ, reg, 0, IMM_BIT));
                    AddCode(arm_mvn(COND_LT, reg, 1, IMM_BIT));
                    AddCode(arm_mov(COND_GT, reg, 1, IMM_BIT));
                    currBlock->WroteToRegister(AS_REGISTER1);

                    break;
                }
                case asBC_TP:
                case asBC_TNP:
                case asBC_TS:
                case asBC_TNS:
                case asBC_TZ:
                case asBC_TNZ:
                {
                    int reg = currBlock->GetNative(AS_REGISTER1, true);
                    int condtrue, condfalse;
                    GetConditions(opcode, condtrue, condfalse);
                    AddCode(arm_mov(COND_AL, reg, reg, SETCOND_BIT));
                    AddCode(arm_mov(condtrue, reg, 1, IMM_BIT));
                    AddCode(arm_mov(condfalse, reg, 0, IMM_BIT));
                    currBlock->WroteToRegister(AS_REGISTER1);

                    break;
                }
                case asBC_ClrHi:
                {
                    // Not needed
                    break;
                }
                case asBC_JZ:
                {
                    int target = asBC_INTARG(bytecode);
                    int reg = currBlock->GetNative(AS_REGISTER1);
                    AddCode(arm_mov(COND_AL, reg, reg, SETCOND_BIT));
                    currBlock->Flush();
                    AddCode(arm_b(COND_EQ, (bytecodePos+size+target)*4, REVISIT_JUMP_BIT));
                    break;
                }
                case asBC_JMP:
                {
                    int target = asBC_INTARG(bytecode);
                    currBlock->Flush();
                    AddCode(arm_b(COND_AL, (bytecodePos+size+target)*4, REVISIT_JUMP_BIT));
                    
                    break;
                }
                case asBC_IncVi:
                {
                    int dst = asBC_SWORDARG0(bytecode);
                    int r = currBlock->GetNative(dst); // It IS written to, but it is read from first
                    AddCode(arm_add(COND_AL, r, r, 1, IMM_BIT));
                    currBlock->WroteToRegister(dst);
                    break;
                }
                case asBC_DecVi:
                {
                    int dst = asBC_SWORDARG0(bytecode);
                    int r = currBlock->GetNative(dst); // It IS written to, but it is read from first
                    AddCode(arm_sub(COND_AL, r, r, 1, IMM_BIT));
                    currBlock->WroteToRegister(dst);
                    break;
                }
                case asBC_CpyVtoR4:
                {
                    int reg = asBC_SWORDARG0(bytecode);
                    int r = currBlock->GetNative(reg);
                    int r2 = currBlock->GetNative(AS_REGISTER1, true);
                    AddCode(arm_mov(COND_AL, r2, r, 0));
                    currBlock->WroteToRegister(AS_REGISTER1);
                    break;
                }
                case asBC_SetV1:
                case asBC_SetV2:
                case asBC_SetV4:
                {
                    int reg = asBC_SWORDARG0(bytecode);
                    int arg = asBC_DWORDARG(bytecode);
                    int r = currBlock->GetNative(reg, true);
                    LoadConstantValue(r, arg);
                    currBlock->WroteToRegister(reg);
                    break;
                }
                case asBC_SetG4:
                {

                    int reg = currBlock->GetNative(REGISTER_TEMP);
                    int reg2 = currBlock->GetNative(REGISTER_TEMP2);
                    LoadConstantValue(reg2, asBC_DWORDARG(bytecode));

                    AddCode(arm_ldr(COND_AL, reg, REG_R0, offsetof(asSVMRegisters, globalVarPointers), IMM_BIT|PRE_BIT));
                    AddCode(arm_ldr(COND_AL, reg, reg, asBC_WORDARG0(bytecode)*4, IMM_BIT|PRE_BIT));
                    AddCode(arm_str(COND_AL, reg2, reg, 0, IMM_BIT|PRE_BIT));

                    registerManager->FreeRegister(reg);
                    registerManager->FreeRegister(reg2);
                    break;
                }
                case asBC_ADDi:
                case asBC_SUBi:
                case asBC_MULi:
                {
                    int asd = asBC_SWORDARG0(bytecode);
                    int op1 = asBC_SWORDARG1(bytecode);
                    int op2 = asBC_SWORDARG2(bytecode);

                    
                    op1 = currBlock->GetNative(op1);
                    op2 = currBlock->GetNative(op2);
                    int d  = currBlock->GetNative(asd, true);

                    switch (opcode)
                    {
                        case asBC_ADDi: AddCode(arm_add(COND_AL, d, op1, op2, 0)); break;
                        case asBC_SUBi: AddCode(arm_sub(COND_AL, d, op1, op2, 0)); break;
                        case asBC_MULi: AddCode(arm_mul(COND_AL, d, op1, op2, 0)); break;
                    }
                    currBlock->WroteToRegister(asd);
                    break;
                }
                case asBC_ADDIi:
                case asBC_SUBIi:
                case asBC_MULIi:
                {
                    
                    int asd = asBC_SWORDARG0(bytecode);
                    int op1 = asBC_SWORDARG1(bytecode);
                    int op2 = currBlock->GetNative(REGISTER_TEMP);

                    op1 = currBlock->GetNative(op1);
                    int d = currBlock->GetNative(asd, true);
                    LoadConstantValue(op2, asBC_INTARG(bytecode+1));

                    switch (opcode)
                    {
                        case asBC_ADDIi: AddCode(arm_add(COND_AL, d, op1, op2, 0)); break;
                        case asBC_SUBIi: AddCode(arm_sub(COND_AL, d, op1, op2, 0)); break;
                        case asBC_MULIi: AddCode(arm_mul(COND_AL, d, op1, op2, 0)); break;
                    }
                    currBlock->WroteToRegister(asd);
                    registerManager->FreeRegister(op2);                    

		            break;
                }

                case asBC_SUSPEND:
                {
                    currBlock->Flush();
                    int reg = currBlock->GetNative(REGISTER_TEMP);
                    AddCode(arm_ldr(COND_AL, reg, REG_R0, offsetof(asSVMRegisters, doProcessSuspend), IMM_BIT|PRE_BIT));
                    AddCode(arm_cmp(COND_AL, reg, 0, IMM_BIT));
                    AddCode(arm_b(COND_NE, bytecodeLen*4, REVISIT_JUMP_BIT));
                    registerManager->FreeRegister(reg);
                    break;
                }
                case asBC_JitEntry:
                    break;

                case asBC_RET:
                {
                    implementedInstructionSize -= size;
                    currBlock->Return();
                    AddCode(arm_b(COND_AL, (bytecodeLen+1)*4, REVISIT_JUMP_BIT));
                    break;
                }

                default:
                    // currBlock->hasUnimplementedBytecode = true;
                    printf("888888888888888888888888888888888888888888888\n");
                    printf("UNIMPLEMENTED BYTECODE\n");
                    printf("888888888888888888888888888888888888888888888\n");
                    //if (currCodeOffset+codelen-prologueLen == currBlock->nativeOffset)
                    //{
                    //    currBlock->hasUnimplementedBytecode = true;
                    //}

                    // Unimplemented instruction. Flush native code
                    implementedInstructionSize -= size;
                    currBlock->Return();
                    AddCode(arm_b(COND_AL, bytecodeLen*4, REVISIT_JUMP_BIT));
                    break;

            }
        }

        bytecode += size;
        bytecodePos += size;
    }

    blocks[blocks.size()-2].nativeOffset = currCodeOffset+codelen-prologueLen;
    InsertFunctionEpilogue(registerUsage);

    // Patch up code as needed
    for (int i = 0; i < currCodeOffset+codelen; i++)
    {
        int id = (currMachine[i] >> 25) & 7;
        if ((id == 5) && (currMachine[i] & REVISIT_JUMP_BIT))
        {
            int target = (currMachine[i] & ~REVISIT_JUMP_BIT) & B_MASK;
            for (unsigned int j = 0; j < blocks.size(); j++)
            {
                Block *currBlock = &blocks[j];
                //printf("testing: %d, %d\n", target, blocks[j].byteCodeStart);
                if (currBlock->byteCodeStart == target)
                {
                    target = -(i-prologueLen-currBlock->nativeOffset+2);
                    if (!currBlock->isCrossSuspendJumpTarget)
                        target += currBlock->loadInstructions;
                    
                    //printf("found! Real offset is %d\n", target);
                    currMachine[i] = (currMachine[i]&~B_MASK) | (target & B_MASK);
                    break;
                }
            }
        }
#ifdef VERBOSE_DEBUG
        printf("0x%.8x ", i*4);
        disasm(currMachine[i]);
#endif
    }
    *output = (asJITFunction) currMachine;

    return asSUCCESS;
}

void asCJitArm::ReleaseJITFunction(asJITFunction jitFunction)
{
    CodeFree((void*) jitFunction);
}

int asCJitArm::GetCurrentBytecodePosition()
{
    return bytecodePos;
}

int asCJitArm::ResolveJitEntry(asUINT offset)
{
    for (std::vector<Block>::iterator it = blocks.begin(); it < blocks.end(); it++)
    {
        if (it->byteCodeStart == offset /*&& !it->hasUnimplementedBytecode*/)
        {
            printf("Returning: 0x%x\n", it->nativeOffset);
            return it->nativeOffset;
        }
    }
    printf("Block not found...\n");
    return -1;
}

void asCJitArm::EndCompile()
{
    blocks.clear();
    registerManager->FreeRegisters();
}

void asCJitArm::InsertFunctionEpilogue(std::vector<ASRegister> &registerUsage)
{
    if (settings.regHandling == GLOBAL_LOAD_STORE)
    {
        ASRegister vr(0, false);
        for (unsigned int i = 0; i < registerUsage.size(); i++)
        {
            if (registerUsage[i].nativeMapping == REGISTER_EMPTY)
                break;
            if (registerUsage[i].writtenTo)
            {
                if (registerUsage[i].id != AS_REGISTER1)
                    registerManager->SaveRegister(registerUsage[i].id, registerUsage[i].nativeMapping);
                else
                    vr = registerUsage[i];
            }
        }
        blocks[blocks.size()-1].nativeOffset = currCodeOffset+codelen-prologueLen;
        if (vr.id == AS_REGISTER1) // means it was indeed written to
            registerManager->SaveRegister(vr.id, vr.nativeMapping);
    }
    int registerUseMask = registerManager->GetUsedMask();
    AddCode(arm_ldm(COND_AL, REG_SP, (1<<REG_PC)|registerUseMask, IA_BIT|WRITEBACK_BIT));
    currMachine[currCodeOffset] |= registerUseMask;

    for (int i = 0; i < datalen; i++)
    {
        AddCode(currData[i]);
    }
    for (int i = 0; i < currCodeOffset+codelen-datalen; i++)
    {
        int id = (currMachine[i] >> 25) & 7;
        if ((id == 2 || id == 3) && !(currMachine[i] & IMM_BIT) && ((currMachine[i]>>REG3_SHIFT)&REG_MASK) == REG_PC)
        {
            // patch up ldr/str rX,[pc, #off] with the correct offset
            int opcode = currMachine[i] &~ (INC_BIT|LDRSTR_IMM_MASK);
            int off1 = currMachine[i] & LDRSTR_IMM_MASK;

            int offset = (codelen-datalen-(i-currCodeOffset)-2+off1)*4;
            if (offset > 0)
                opcode |= INC_BIT;
            // TODO: Need to make sure offset is within the available range.
            //       But it would be a REALLY hefty script function for the offset
            //       to be out of range in the first place.
            currMachine[i] = opcode | ABS(offset);
        }
    }
    currCodeOffset += codelen;
    codelen = 0;
    datalen = 0;
}

END_AS_NAMESPACE
