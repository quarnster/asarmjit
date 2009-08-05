#include "as_jit_arm_op.h"
#include <string.h>
#include <stdio.h>

static char regnames[16][4] =
{
"r0","r1","r2","r3","r4","r5","r6","r7",
"r8","r9","r10","r11","r12","sp","lr","pc"
};

static char fregnames[32][4] =
{
"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
"s8", "s9", "s10", "s11", "s12", "s13", "s14", "s15",
"s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23",
"s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31"
};
static char dregnames[16][4] = 
{
"d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
"d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15"
};


static char dataopnames[16][4] ={
"and", "eor", "sub", "rsb", "add", "adc", "sbc", "rsc", "tst", "teq", "cmp", "cmn", "orr", "mov", "bic", "mvn"
};

static char fmathopnames[16][6] =
{
"fmac",
"fmsc",
"fmul",
"fadd",
"fnmac",
"fnmsc",
"",
"fsub",
"fdiv"
};

static char condnames[16][3] = {"eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc", "hi", "ls", "ge", "lt", "gt", "le", "al", "nv"};
static char shiftnames[4][4] = {"lsl", "lsr", "asr", "ror" };

void arm_disasm(int machine)
{
    // This isn't 100% correct, but for the opcodes we spit out
    // it'll manage to parse them correctly
    char opname[100] = "";
    char arg[200] = "";
    
    int cond = (machine >> COND_SHIFT) & COND_BITS;
    int id = (machine >> 25) & 7;
    switch (id)
    {
        case 0:
            if (!((machine & ~(BLX_BIT|(COND_NV<<COND_SHIFT)|REG_MASK))^BX_BITS))
            {  
                strcpy(opname, (machine & BLX_BIT) ? "blx" : "bx");
                sprintf(arg, "%s", regnames[machine&REG_MASK]);
                break;
            }
            else if ((machine >> 7) & 0x1)
            {
                // mul
                int multype  = (machine >> OPCODE_SHIFT) & 0xf;
                int setcond  = (machine >> SETCOND_SHIFT) & 0x1;
                int destreg  = (machine >> REG3_SHIFT) & REG_MASK;
                int accreg   = (machine >> REG2_SHIFT) & REG_MASK;
                int operand2 = (machine >> REG4_SHIFT) & REG_MASK;
                int operand1 = (machine >> REG1_SHIFT) & REG_MASK;
                
                sprintf(opname, "%s%s%s",
                        multype ? "mla" : "mul",
                        cond != COND_AL ? condnames[cond] : "",
                        setcond ? "s" : ""
                        
                        );
                sprintf(arg, "%s, %s, %s%s%s",
                        regnames[destreg],
                        regnames[operand1],
                        regnames[operand2],
                        multype ? ", " : "",
                        multype ? regnames[accreg] : ""
                        );
                
                break;
            }
            // fall through
        case 1:
        {
            // data proccessing
            int imm         = (machine >> IMM_SHIFT) & 0x1;
            int opcode      = (machine >> OPCODE_SHIFT) & 0xf;
            int setcond     = (machine >> SETCOND_SHIFT) & 0x1;
            int operand2    = (machine >> REG3_SHIFT) & REG_MASK;
            int destreg     = (machine >> REG2_SHIFT) & 0xf;
            int shiftamt    = (machine >>  SHIFT_IMM_SHIFT) & SHIFT_IMM_MASK;
            int shiftreg    = (machine >>  SHIFT_REG_SHIFT) & REG_MASK;
            int shifttype   = (machine >>  SHIFT_TYPE_SHIFT) & 0x3;
            int shiftbyreg  = (machine >>  SHIFT_BY_REG_SHIFT) & 0x1;
            int operand1    = (machine >>  REG1_SHIFT) & REG_MASK;
            int op2imm      = (machine >>  0) & 0xff;
            
            strcpy(opname, dataopnames[opcode]);
            if (cond != COND_AL)
                strcat(opname, condnames[cond]);
            if (opcode != OP_CMP && setcond)
                strcat(opname, "s");
            
            char op2immname[20];
            sprintf(op2immname, "#0x%x", op2imm);
            
            sprintf(arg, "%s%s%s, %s",
                    (opcode < OP_TST || opcode > OP_CMN) ? regnames[destreg] : "",
                    ((opcode < OP_TST || opcode > OP_CMN) && opcode != OP_MOV && opcode != OP_MVN) ? ", " : "",
                    (opcode != OP_MOV && opcode != OP_MVN) ? regnames[operand2] : "",
                    imm ? op2immname : regnames[operand1]
                    );
            if (!imm && (shifttype != SHIFT_LSL || shiftbyreg || shiftamt != 0))
            {
                char shift[100];
                char shiftarg[100];
                if (shiftbyreg)
                    sprintf(shiftarg, regnames[shiftreg]);
                else
                    sprintf(shiftarg, "#0x%x", shiftamt);
                sprintf(shift, ", %s %s", shiftnames[shifttype], shiftarg);
                strcat(arg, shift);
            }
            
            break;
        }
        case 2:
        case 3:
        {
            int imm     = !((machine >> IMM_SHIFT)&1);
            int pre     = (machine >> PREPOST_SHIFT)&1;
            int up      = (machine >> INCDEC_SHIFT)&1;
            int byte    = (machine >> BYTE_SHIFT)&1;
            int wb_mem  = (machine >> WRITEBACK_SHIFT)&1;
            int ls      = (machine >> LDRSTR_SHIFT)&1;
            int base    = (machine >> REG3_SHIFT)& REG_MASK;
            int srcdst  = (machine >> REG2_SHIFT) & REG_MASK;
            int immvalue = machine & LDRSTR_IMM_MASK;
            //int shiftamt = (machine >> 7)&0x1f;
            //int shiftt  = (machine >> 5)&0x3;
            int offreg   = machine & 0xf;
            
            
            char myarg[20];
            if (imm)
                sprintf(myarg, ", #%s0x%x", up ? "" : "-", immvalue);
            else
                sprintf(myarg, ", %s%s%s", up ? "" : "-", regnames[offreg], "");
            
            sprintf(opname, "%s%s%s",
                    ls ? "ldr" : "str",
                    cond != COND_AL ? condnames[cond] : "",
                    byte ? "b" : ""
                    );
            
            
            sprintf(arg, "%s, [%s%s]%s%s",
                    regnames[srcdst],
                    regnames[base],
                    pre ? (imm ? (immvalue ? myarg : "") : myarg) : "",
                    !pre ? (imm ? (immvalue ? myarg : "") : myarg) : "",
                    wb_mem ? "!" : ""
                    );
            
            break;
        }
        case 4:
        {
            int pre     = (machine >> PREPOST_SHIFT)&1;
            int inc     = (machine >> INCDEC_SHIFT)&1;
            
            int wb_mem  = (machine >> WRITEBACK_SHIFT)&1;
            int ls      = (machine >> LDRSTR_SHIFT)&1;
            int base    = (machine >> REG3_SHIFT)& REG_MASK;
            int arglist = (machine & 0xffff);
            
            sprintf(opname, "%s%s%s%s",
                    ls ? "ldm" : "stm",
                    inc ? "i" : "d",
                    pre ? "b" : "a",
                    cond != COND_AL ? condnames[cond] : ""
                    );
            sprintf(arg, "%s%s, {",
                    regnames[base],
                    wb_mem ? "!" : ""
                    );
            int firstReg = 1;
            int startReg = 0;
            int endReg = 0;
            for (int i = 0; i <= REG_R15; i++)
            {
                if (arglist & (1 << i))
                {
                    if (firstReg)
                    {
                        startReg = i;
                    }
                    firstReg = 0;
                    endReg = i;
                }
                if (!(arglist & (1 << i)) && !firstReg || i == REG_R15)
                {
                    if (arg[strlen(arg)-1] != '{')
                        strcat(arg, ",");
                    strcat(arg, regnames[startReg]);
                    if (endReg != startReg)
                    {
                        strcat(arg, "-");
                        strcat(arg, regnames[endReg]);
                    }
                    firstReg = 1;
                }
            }
            strcat(arg, "}");
            
            break;
        }
        case 5:
            // branch
            strcpy(opname, "b");
            if (machine & LINK_BIT)
                strcat(opname, "l");
            
            
            if (cond != COND_AL)
                strcat(opname, condnames[cond]);
            sprintf(arg, "0x%x", machine & B_MASK);
            break;
        case 6:
        {
            int code    = ((machine >> FOPCODE_SHIFT) & FOPCODE_MASK);
            
            if ((code&~(1|8)) == 16)
            {
                sprintf(opname, "%s%s%s",
                        (machine & LDRSTR_BIT) ? "fld" : "fst",
                        (machine & FDOUBLE_BIT) ? "d" : "s",
                        cond != COND_AL ? condnames[cond] : ""
                        );
                int freg = ((machine >> REG2_SHIFT) & REG_MASK);
                int areg = ((machine >> REG3_SHIFT) & REG_MASK);
                sprintf(arg, "%s, [%s, #%s%d]",
                        (machine & FDOUBLE_BIT) ? dregnames[freg] : fregnames[freg *2 + ((machine>>FREG2_ODD_SHIFT)&1)],
                        regnames[areg],
                        (machine & (1 << 23)) ? "" : "-",
                        (machine&0xff) << 2
                        );
                
            }
            break;
        }
        case 7:
        {
            int code    = ((machine >> 20) & FOPCODE_MASK);
            if (machine & FOP_TRANSFER_BIT)
            {
                int vfptoarm = (machine >> FOPCODE_SHIFT) & 1;
                sprintf(opname, "%s%s", 
                        vfptoarm ? "fmrs" : "fmsr",
                        cond == COND_AL ? "" :condnames[cond]
                        );
                int dest    = (machine >> REG3_SHIFT) & REG_MASK;
                int src     = (machine >> REG2_SHIFT) & REG_MASK;
                
                if (machine & FDOUBLE_BIT)
                {
                    sprintf(arg, "%s, %s",
                            vfptoarm ? regnames[src] : dregnames[dest],
                            vfptoarm ? dregnames[dest] : regnames[src]
                            );
                }
                else
                {
                    sprintf(arg, "%s, %s",
                            vfptoarm ? regnames[src] : fregnames[dest*2 + ((machine >> FREG3_ODD_SHIFT)&1)],
                            vfptoarm ? fregnames[dest*2 + ((machine >> FREG3_ODD_SHIFT)&1)] : regnames[src]
                            );
                }
                
            }
            else
                if (code < 9)
                {
                    code |= ((machine & FSUB_BIT) >> (FSUB_SHIFT-2));
                    sprintf(opname, "%s%s%s",
                            fmathopnames[code],
                            (machine & FDOUBLE_BIT) ? "d" : "s",
                            cond == COND_AL ? "" :condnames[cond]
                            );
                    int dest    = (machine >> REG2_SHIFT) & REG_MASK;
                    int op1     = (machine >> REG3_SHIFT) & REG_MASK;
                    int op2     = (machine >> REG1_SHIFT) & REG_MASK;
                    
                    if (machine & FDOUBLE_BIT)
                    {
                        sprintf(arg, "%s, %s, %s",
                                dregnames[dest],
                                dregnames[op1],
                                dregnames[op2]
                                );
                    }
                    else
                    {
                        sprintf(arg, "%s, %s, %s",
                                fregnames[dest*2 + ((machine >> FREG2_ODD_SHIFT)&1)],
                                fregnames[op1 *2 + ((machine >> FREG3_ODD_SHIFT)&1)],
                                fregnames[op2 *2 + ((machine >> FREG1_ODD_SHIFT)&1)]
                                );
                    }
                }
            break;
        }
            
    }
    
    printf("0x%.8x %-8s    %s", machine, opname, arg);
    printf("\n");
}