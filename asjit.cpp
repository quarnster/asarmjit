
#include "asjit.h"

#include "section.h"

#include <jit/jit-dump.h>

#define VERBOSE_DEBUG


asJIT::asJIT(asIScriptEngine *engine)
: m_engine(engine)
{
    m_refCount = 1;
    m_engine->AddRef();
    context = jit_context_create();
}

asJIT::~asJIT()
{
    m_engine->Release();
    jit_context_destroy(context);
}

class JumpTarget
    {
    public:
        int pos;
        bool isCrossSuspendTarget;
    };

void asJIT::SplitIntoSections(const asDWORD* bytecode, int bytecodeLen)
{
    int i = 0;
    int start = 0;
    int lastPos = 0;
    currSection = NULL;
    

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
    
    // Split bytecode into logical sections, starting at a jump target or a SUSPEND bytecode,
    // and ending at the bytecode before a SUSPEND or including a cross suspend jump bytecode.
    i = 0;
    int nextJump = 0;
    JumpTarget t;
    t.pos = 0xfffffff;// Something ridiculous so that atleast it isn't empty
    jumpTargets.push_back(t); 
    
    Section s(0, 0);
    sections.push_back(s);
    currSection = &sections[sections.size()-1];
    start = 0;
    
    while (i <= bytecodeLen)
    {
        asDWORD opcode = asBC_SUSPEND;
        if (i < bytecodeLen)
            opcode = *(unsigned char*)&bytecode[i];
        int size = asBCTypeSize[asBCInfo[opcode].type];
        bool doSectionBreak = false;
        switch (opcode)
        {
            case asBC_JitEntry:
                if (i == 0)
                    currSection->isJitEntry = true;
                break;
            case asBC_JMP:
            case asBC_JZ:
            case asBC_JNZ:
            case asBC_JS:
            case asBC_JNS:
            case asBC_JP:
            case asBC_JNP:
            {
                doSectionBreak = true;                
                break;
            }
            default:
                doSectionBreak = false;
                break;
        }
        
        bool isJumpTarget = false;
        bool isCrossSuspendJumpTarget = false;
        
        if (jumpTargets[nextJump].pos == i+size)
        {
            isCrossSuspendJumpTarget = jumpTargets[nextJump].isCrossSuspendTarget;
            nextJump++;
            doSectionBreak = true;
            isJumpTarget = true;
        }
        bool isJitEntry = false;
        if (i+size < bytecodeLen && *(unsigned char*)&bytecode[i+size] == asBC_JitEntry)
        {
            doSectionBreak = true;
            isJitEntry = true;
        }
        
        if (doSectionBreak || (currSection && currSection->byteCodeEnd && i >= currSection->byteCodeEnd))
        {
            currSection->byteCodeEnd = i;
            int off = i;
            off += size;
            Section s(off, 0);
            s.isJitEntry = isJitEntry;
            sections.push_back(s);
            currSection = &sections[sections.size()-1];
            start = i;
        }
        lastPos = i;
        i += size;
    }
    currSection->byteCodeEnd = i;
    for (unsigned int i = 0; i < sections.size(); i++)
    {
        printf("sections[%d]: %d, %d\n",
               i, sections[i].byteCodeStart, sections[i].byteCodeEnd
               );
    }
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

jit_function_t function;
void testfunc(asSVMRegisters* regs, asDWORD suspendId)
{
    // printf("calling jit\n");
    jit_type_t result;
    void *args[2];
    args[0] = &regs;
    args[1] = &suspendId;
    jit_function_apply(function, args, &result);
    // printf("jit returned: %d\n", result);
}

class Register
{
public:
    jit_value_t native;
    int asID;
    bool writtenTo;
};
#define AS_REGISTER1 (1 << 16)
class RegisterManager
{
public:
    RegisterManager()
    {
    }
    ~RegisterManager()
    {
    }

    void AddRegister(int id, bool writtenTo = false)
    {
        int count = registers.size();
        for (int i = 0; i < count; i++)
        {
            if (registers[i].asID == id)
            {
                registers[i].writtenTo |= writtenTo;
                return;
            }
        }
        printf("adding %d\n", id);
        Register r;
        r.asID = id;
        r.writtenTo = writtenTo;
        registers.push_back(r);
    }
    jit_value_t GetRegister(int id)
    {
        int count = registers.size();
        for (int i = 0; i < count; i++)
        {
            if (registers[i].asID == id)
            {
                return registers[i].native;
            }
        }
        printf("Couldn't find reg (%d)!!!\n", id);
        return 0;
    }
    void LoadRegisters(jit_value_t regs)
    {
        jit_value_t fp = 0;
        int count = registers.size();
        for (int i = 0; i < count; i++)
        {
            if (registers[i].asID == AS_REGISTER1)
            {
                registers[i].native = jit_insn_load_relative(function, regs, offsetof(asSVMRegisters, valueRegister), jit_type_int);
            }
            else
            {
                if (!fp)
                    fp = jit_insn_load_relative(function, regs, offsetof(asSVMRegisters, stackFramePointer), jit_type_void_ptr);
                registers[i].native = jit_insn_load_relative(function, fp, -registers[i].asID * 4, jit_type_int);
            }
        }
    }

    void SaveRegisters(jit_value_t regs)
    {
        jit_value_t fp = 0;
        int count = registers.size();
        for (int i = 0; i < count; i++)
        {
            if (!registers[i].writtenTo)
                continue;

            if (registers[i].asID != AS_REGISTER1)
            {
                if (!fp)
                    fp = jit_insn_load_relative(function, regs, offsetof(asSVMRegisters, stackFramePointer), jit_type_void_ptr);
                jit_insn_store_relative(function, fp, -registers[i].asID * 4, registers[i].native);
            }
        }
        
    }
private:
    std::vector<Register> registers;
};
RegisterManager registerManager;
void AllocateRegisters(const asDWORD * bytecode, asUINT bytecodeLen)
{
    int i = 0;
    while (i < bytecodeLen)
    {
        asDWORD opcode = *(unsigned char*)&bytecode[i];

        int size = asBCTypeSize[asBCInfo[opcode].type];

         switch (asBCInfo[opcode].type)
         {
             case asBCTYPE_wW_ARG:
             case asBCTYPE_wW_W_ARG:
             case asBCTYPE_wW_QW_ARG:
             case asBCTYPE_wW_DW_ARG:
             {
                 registerManager.AddRegister(asBC_SWORDARG0(&bytecode[i]), true);
                 if (opcode == asBC_CpyRtoV4 || opcode == asBC_LdGRdR4)
                     registerManager.AddRegister(AS_REGISTER1);
                 break;
             }
             case asBCTYPE_rW_ARG:
             case asBCTYPE_rW_DW_ARG:
             {
                 if (opcode == asBC_CMPIi || opcode == asBC_CMPi)
                 {
                     registerManager.AddRegister(AS_REGISTER1, true);
                 }
                 else if (opcode == asBC_CpyVtoR4)
                 {
                     registerManager.AddRegister(AS_REGISTER1, true);
                 }
                 
                 registerManager.AddRegister(asBC_SWORDARG0(&bytecode[i]));
                 if (opcode == asBC_IncVi)
                     registerManager.AddRegister(asBC_SWORDARG0(&bytecode[i]), true);
                 break;
             }
             case asBCTYPE_wW_rW_rW_ARG:
             {
                 registerManager.AddRegister(asBC_SWORDARG2(&bytecode[i]));
                 registerManager.AddRegister(asBC_SWORDARG1(&bytecode[i]));
                 registerManager.AddRegister(asBC_SWORDARG0(&bytecode[i]), true);
                 
                 break;
             }
             case asBCTYPE_wW_rW_DW_ARG:
             case asBCTYPE_wW_rW_ARG:
             {
                 registerManager.AddRegister(asBC_SWORDARG1(&bytecode[i]));
                 registerManager.AddRegister(asBC_SWORDARG0(&bytecode[i]), true);
                 break;
             }
             case asBCTYPE_rW_rW_ARG:
             {
                 registerManager.AddRegister(asBC_SWORDARG1(&bytecode[i]));
                 registerManager.AddRegister(asBC_SWORDARG0(&bytecode[i]));
                 break;
             }
             case asBCTYPE_DW_ARG:
             {
                 switch (opcode)
                 {
                     case asBC_JZ:
                         // registerManager.AddRegister(AS_BC);
                         registerManager.AddRegister(AS_REGISTER1);
                         break;
                 }
                 break;
             }
             case asBCTYPE_NO_ARG:
             {
                 if (opcode == asBC_TS || opcode == asBC_ClrHi)
                     registerManager.AddRegister(AS_REGISTER1, true);
                 break;
             }
         }
        i += size;
    }
        
}
int asJIT::StartCompile(const asDWORD * bytecode, asUINT bytecodeLen, asJITFunction *output)
{
    this->bytecode = bytecode;
    currSection = NULL;
    
    bytecodePos = 0;
    
    SplitIntoSections(bytecode, bytecodeLen);
    int sectionPos = 0;

    currSection = &sections[sectionPos];
    Section * lastSection = NULL;

    /* Lock the context while we build and compile the function */
    jit_context_build_start(context);

    /* Build the function signature */
    jit_type_t params[2];
    params[0] = jit_type_void_ptr;
    params[1] = jit_type_int;
    jit_type_t signature = jit_type_create_signature(jit_abi_cdecl, jit_type_int, params, 2, 1);

    /* Create the function object */
    /*jit_function_t */function = jit_function_create(context, signature);
    jit_function_set_optimization_level(function, 0);
    printf("allocate regs\n");
    AllocateRegisters(bytecode, bytecodeLen);

    jit_value_t vmregs = jit_value_get_param(function, 0);
    jit_label_t endFlush = jit_label_undefined;
    jit_label_t endLabel = jit_label_undefined;
    jit_label_t *sectionLabels = new jit_label_t[sections.size()];
    for (int i = 0; i < sections.size(); i++)
    {
        sectionLabels[i] = jit_label_undefined;
    }
    printf("load regs\n");
    registerManager.LoadRegisters(vmregs);
    printf("regs loaded\n");

    //jit_value_t offset = jit_value_get_param(function, 1);
    //jit_insn_jump_table(function, offset, sectionLabels, sections.size());

    while (bytecodePos < bytecodeLen)
    {
        asDWORD opcode = *(unsigned char*)&bytecode[bytecodePos];
        int size = asBCTypeSize[asBCInfo[opcode].type];
        
        if (bytecodePos > currSection->byteCodeEnd)
        {
            lastSection = currSection;
            sectionPos++;
            currSection = &sections[sectionPos];            
        }
        if (bytecodePos == currSection->byteCodeStart)
        {
            printf("new section: %d\n", sectionPos);
            printf("------------------------------------\n");
            jit_insn_label(function, &sectionLabels[sectionPos]);
        }
#ifdef VERBOSE_DEBUG
        PrintByteCode(&bytecode[bytecodePos]);
#endif
        switch (opcode)
        {
            case asBC_PUSH:
            {
                jit_value_t sp = jit_insn_load_relative(function, vmregs, offsetof(asSVMRegisters, stackPointer), jit_type_int);
                jit_value_t amt = jit_value_create_nint_constant(function, jit_type_ushort, asBC_WORDARG0(&bytecode[bytecodePos]));
                jit_value_t sub = jit_insn_sub(function, sp, amt);
                jit_insn_store_relative(function, vmregs,  offsetof(asSVMRegisters, stackPointer), sub);
                break;
            }
            case asBC_SUSPEND:
            {
                break;
            }
            case asBC_SetV4:
            {
                jit_value_t a = registerManager.GetRegister(asBC_SWORDARG0(&bytecode[bytecodePos]));
                jit_value_t b = jit_value_create_nint_constant(function, jit_type_int, asBC_DWORDARG(&bytecode[bytecodePos]));
                jit_insn_store(function, a, b);
                break;
            }
            case asBC_CMPIi:
            {
                jit_value_t a = registerManager.GetRegister(asBC_SWORDARG0(&bytecode[bytecodePos]));
                jit_value_t b = jit_value_create_nint_constant(function, jit_type_int, asBC_INTARG(&bytecode[bytecodePos]));
                jit_value_t op = jit_insn_cmpl(function, a, b);
                jit_insn_store(function, registerManager.GetRegister(AS_REGISTER1), op);
                
                break;
            }
            case asBC_ClrHi:
            {
                break;
            }
            case asBC_TS:
            {
                jit_value_t a = registerManager.GetRegister(AS_REGISTER1);
                jit_value_t b = jit_value_create_nint_constant(function, jit_type_int, 0);
                jit_value_t op = jit_insn_lt(function, a, b);
                jit_insn_store(function, registerManager.GetRegister(AS_REGISTER1), op);
                break;
                                                               
            }
            case asBC_JitEntry:
                break;
            case asBC_IncVi:
            {
                jit_value_t src = registerManager.GetRegister(asBC_SWORDARG0(&bytecode[bytecodePos]));
                jit_value_t val = jit_value_create_nint_constant(function, jit_type_uint, 1);
                jit_value_t op = jit_insn_add(function, src, val);
                jit_insn_store(function, src, op);
                break;
            }

            case asBC_JZ:
            case asBC_JMP:
            {
                int bcTarget = bytecodePos + size + asBC_INTARG(&bytecode[bytecodePos]);
                printf("want target: %d (%d + %d + %d)\n", bcTarget, bytecodePos, size, asBC_INTARG(&bytecode[bytecodePos]));
                int target = -1;
                for (int i = 0; i < sections.size(); i++)
                {
                    if (sections[i].byteCodeStart == bcTarget)
                    {
                        printf("sections %d matched\n", i);
                        target = i;
                        break;
                    }
                }
                printf("target: %d\n", target);
                switch (opcode)
                {
                    case asBC_JMP:
                        jit_insn_branch(function, &sectionLabels[target]);
                        break;
                    case asBC_JZ:
                    {
                        jit_value_t a = registerManager.GetRegister(AS_REGISTER1);
                        jit_insn_branch_if_not(function, a, &sectionLabels[target]);
                        break;
                    }
                }
                break;
            }
            case asBC_ADDIi:
            case asBC_SUBIi:
            case asBC_MULIi:
            {
                jit_value_t a = registerManager.GetRegister(asBC_SWORDARG0(&bytecode[bytecodePos]));
                jit_value_t b = registerManager.GetRegister(asBC_SWORDARG1(&bytecode[bytecodePos]));
                jit_value_t c = jit_value_create_nint_constant(function, jit_type_int, asBC_INTARG(&bytecode[bytecodePos+1]));
                jit_value_t op;
                switch (opcode)
                {
                    case asBC_MULIi: op = jit_insn_mul(function, b, c); break;
                    case asBC_ADDIi: op = jit_insn_add(function, b, c); break;
                    case asBC_SUBIi: op = jit_insn_sub(function, b, c); break;
                }
                printf("muli %d, %d\n", asBC_SWORDARG0(&bytecode[bytecodePos]), asBC_SWORDARG1(&bytecode[bytecodePos]));
                printf("op: %d, arg: %d\n", op, asBC_INTARG(&bytecode[bytecodePos+1]));
                jit_insn_store(function, a, op);
                
                break;
            }
            case asBC_MULi:
            case asBC_ADDi:
            case asBC_SUBi:
            case asBC_DIVi:
            {
                jit_value_t a = registerManager.GetRegister(asBC_SWORDARG0(&bytecode[bytecodePos]));
                jit_value_t b = registerManager.GetRegister(asBC_SWORDARG1(&bytecode[bytecodePos]));
                jit_value_t c = registerManager.GetRegister(asBC_SWORDARG2(&bytecode[bytecodePos]));
                jit_value_t op;
                switch (opcode)
                {
                    case asBC_MULi: op = jit_insn_mul(function, b, c); break;
                    case asBC_ADDi: op = jit_insn_add(function, b, c); break;
                    case asBC_SUBi: op = jit_insn_sub(function, b, c); break;
                    case asBC_DIVi: op = jit_insn_div(function, b, c); break;
                }
                jit_insn_store(function, a, op);
                break;
            }
            case asBC_CpyVtoR4:
            {
                jit_value_t src = registerManager.GetRegister(asBC_SWORDARG0(&bytecode[bytecodePos]));
                jit_value_t dst = registerManager.GetRegister(AS_REGISTER1);
                jit_insn_store(function, dst, src);
                break;
            }
            case asBC_RET:
            {
                // we are returning, so we know that all the temporary variables will be discarded
                jit_value_t bc = jit_value_create_nint_constant(function, jit_type_uint, (jit_nint) &bytecode[bytecodePos]);
                jit_insn_store_relative(function, vmregs, offsetof(asSVMRegisters, programPointer), bc);
                jit_insn_branch(function, &endLabel);
                break;
            }

            default:
            {
                printf("888888888888888888888888888888888888888888888\n");
                printf("UNIMPLEMENTED BYTECODE\n");
                printf("888888888888888888888888888888888888888888888\n");
                // Unimplemented instruction. Flush native code
                jit_value_t bc = jit_value_create_nint_constant(function, jit_type_uint, (jit_nint) &bytecode[bytecodePos]);
                jit_insn_store_relative(function, vmregs, offsetof(asSVMRegisters, programPointer), bc);
                jit_insn_branch(function, &endFlush);
                 break;
            }
        }
        
        bytecodePos += size;
    }

    jit_insn_label(function, &endFlush);
    registerManager.SaveRegisters(vmregs);
    jit_insn_label(function, &endLabel);
    // always save the value register
    jit_insn_store_relative(function, vmregs, offsetof(asSVMRegisters, valueRegister), registerManager.GetRegister(AS_REGISTER1));
    jit_insn_return(function, NULL);

    jit_dump_function(stdout, function, "test");

    if (jit_uses_interpreter())
    {
        jit_function_compile(function);
        printf("uses interpreter!!\n");
        *output = testfunc;
    }
    else
    {
        jit_function_compile_entry(function, (void**) output);
        jit_function_setup_entry(function, (void**) output);
    }

    // Unlock the context
    jit_context_build_end(context);

    delete[] sectionLabels;
    jit_dump_function(stdout, function, "test");

    return asSUCCESS;
}

void asJIT::ReleaseJITFunction(asJITFunction jitFunction)
{
}

int asJIT::ResolveJitEntry(asUINT offset)
{
    int count = sections.size();
    for (int i = 0; i < count; i++)
    {
        if (sections[i].isJitEntry)
        {
            if (sections[i].byteCodeStart == offset)
                return i;
        }
    }
    printf("Section not found...\n");
    return -1;
}

void asJIT::EndCompile()
{
    sections.clear();
}
