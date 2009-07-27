
//
// as_jit_arm_op.h
//
// Opcode helper defines for the ARM machine code translator
// Written by Fredrik Ehnbom in June-August 2009

#ifndef AS_JIT_ARM_OP_H
#define AS_JIT_ARM_OP_H


#include <stdio.h>

#pragma warning(disable: 4996)

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

enum {
    REG_R0 = 0,
    REG_R1,
    REG_R2,
    REG_R3,
    REG_R4,
    REG_R5,
    REG_R6,
    REG_R7,
    REG_R8,
    REG_R9,
    REG_R10,
    REG_R11,
    REG_R12,
    REG_R13,
    REG_R14,
    REG_R15,
    REG_SP = REG_R13,
    REG_LR = REG_R14,
    REG_PC = REG_R15,
    REG_MASK = 0xf,

    REG3_SHIFT = 16,
    REG2_SHIFT = 12,
    REG4_SHIFT = 8,
    REG1_SHIFT = 0, 

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

enum
{
    OP_FMAC = 0,
    OP_FMSC,
    OP_FMUL,
    OP_FADD,
    OP_FDIV = 8
};

enum
{
    OP_AND = 0,
    OP_EOR,
    OP_SUB,
    OP_RSB,
    OP_ADD,
    OP_ADC,
    OP_SBC,
    OP_RSC,
    OP_TST,
    OP_TEQ,
    OP_CMP,
    OP_CMN,
    OP_ORR,
    OP_MOV,
    OP_BIC,
    OP_MVN,

    OP_MUL = 0,
    OP_MLA,

    OP_ST = 0,
    OP_LD
};

static char condnames[16][3] = {"eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc", "hi", "ls", "ge", "lt", "gt", "le", "al", "nv"};
enum
{
    COND_EQ = 0, COND_NE,
    COND_CS,
    COND_CC,
    COND_MI,
    COND_PL,
    COND_VS,
    COND_VC,
    COND_HI,
    COND_LS,
    COND_GE,
    COND_LT,
    COND_GT,
    COND_LE,
    COND_AL,
    COND_NV,
};

static char shiftnames[4][4] = {"lsl", "lsr", "asr", "ror" };
enum
{
    SHIFT_LSL = 0,
    SHIFT_LSR,
    SHIFT_ASR,
    SHIFT_ROR,
    SHIFT_TYPE_SHIFT = 5,
    SHIFT_TYPE_MASK = 0x3,

    SHIFT_BY_REG_SHIFT = 4,
    SHIFT_BY_REG_BIT = 1<<SHIFT_BY_REG_SHIFT,
    SHIFT_REG_SHIFT = 8,
    SHIFT_IMM_SHIFT = 7,
    SHIFT_IMM_MASK = 0x1f
};

enum
{
    COND_SHIFT = 28,
    COND_BITS = 0xf,

    OPCODE_SHIFT = 21,
    SETCOND_SHIFT = 20,

    LDRSTR_SHIFT = 20,
    WRITEBACK_SHIFT = 21,
    BYTE_SHIFT = 22,
    INCDEC_SHIFT = 23,
    PREPOST_SHIFT = 24,

    LDRSTR_IMM_MASK = 0xfff,

    IMM_SHIFT = 25,
    IMM_BIT = (1 << IMM_SHIFT),
    SETCOND_BIT = (1 << SETCOND_SHIFT),
    LDRSTR_BIT = (1<< LDRSTR_SHIFT),
    WRITEBACK_BIT = (1 << WRITEBACK_SHIFT),
    BYTE_BIT = (1 << BYTE_SHIFT),
    INC_BIT = (1 << INCDEC_SHIFT),
    PRE_BIT = (1 << PREPOST_SHIFT),

    BLX_SHIFT = 5,
    BLX_BIT = 1 << BLX_SHIFT,
    BX_BITS = 1 << 24 | 1 << 21 | 0xfff << 8 | 1 << 4,
    B_MASK = 0xffffff,
    LINK_SHIFT = 24,
    LINK_BIT = 1 << LINK_SHIFT,

    IA_BIT = (INC_BIT),
    DB_BIT = (PRE_BIT),

};

enum
{
    REG_S0 = 0,
    REG_S1,
    REG_S2,
    REG_S3,
    REG_S4,
    REG_S5,
    REG_S6,
    REG_S7,
    REG_S8,
    REG_S9,
    REG_S10,
    REG_S11,
    REG_S12,
    REG_S13,
    REG_S14,
    REG_S15,
    REG_S16,
    REG_S17,
    REG_S18,
    REG_S19,
    REG_S20,
    REG_S21,
    REG_S22,
    REG_S23,
    REG_S24,
    REG_S25,
    REG_S26,
    REG_S27,
    REG_S28,
    REG_S29,
    REG_S30,
    REG_S31,

    REG_D0 = 0,
    REG_D1 = 2,
    REG_D2 = 4,
    REG_D3 = 6,
    REG_D4 = 8,
    REG_D5 = 10,
    REG_D6 = 12,
    REG_D7 = 14,
    REG_D8 = 16,
    REG_D9 = 18,
    REG_D10 = 20,
    REG_D11 = 22,
    REG_D12 = 24,
    REG_D13 = 26,
    REG_D14 = 28,
    REG_D15 = 30
};

enum
{
    FID = ((3 << 26) | (1 << 11) | (1 << 9)),
    FSUB_SHIFT = 6,
    FDOUBLE_SHIFT = 8,
    FLD_SHIFT = 20,
    FMUL_SHIFT = 21,
    FDIV_SHIFT = 23,

    FDOUBLE_BIT = (1 << FDOUBLE_SHIFT),
    FSUB_BIT = (1 << FSUB_SHIFT),
    FMUL_BIT = (1 << FMUL_SHIFT),
    FDIV_BIT = (1 << FDIV_SHIFT),
    FLD_BIT = (1 << FLD_SHIFT),

    FOPCODE_SHIFT = 20,
    FOPCODE_MASK = 0x1b,

    FREG3_ODD_SHIFT = 7,
    FREG3_ODD_BIT = (1 << FREG3_ODD_SHIFT),

    FREG2_ODD_SHIFT = 22,
    FREG2_ODD_BIT = (1 << FREG2_ODD_SHIFT),

    FREG1_ODD_SHIFT = 5,
    FREG1_ODD_BIT = (1 << FREG1_ODD_SHIFT),

    VFP_TO_ARM_SHIFT = 20,
    VFP_TO_ARM_BIT = (1 << VFP_TO_ARM_SHIFT),

    FOP_TRANSFER_SHIFT = 4,
    FOP_TRANSFER_BIT = (1 << FOP_TRANSFER_SHIFT)
};

#define dataop(op, cond, dst, op1, op2, extra) \
    (((cond)    << COND_SHIFT   ) | \
    ((op)       << OPCODE_SHIFT ) | \
    ((dst)      << REG2_SHIFT   ) | \
    ((op1)      << REG3_SHIFT   ) | \
    ((op2)      << REG1_SHIFT   ) | \
    (extra))

#define mulop(op, cond,dst, op1,op2, acc, extra) \
    (((cond)    << COND_SHIFT   ) | \
    ((op)       << OPCODE_SHIFT ) | \
    ((dst)      << REG3_SHIFT   ) | \
    ((op1)      << REG1_SHIFT   ) | \
    ((op2)      << REG4_SHIFT   ) | \
    ((acc)      << REG2_SHIFT   ) | \
    (1          << 7            ) | \
    (1          << 4            ) | \
    (extra) )

#define ldrstr(op, cond, srcdst, base, off, extra) \
    (((cond)        << COND_SHIFT   ) | \
    ((op)           << LDRSTR_SHIFT ) | \
    ((srcdst)       << REG2_SHIFT   ) | \
    ((base)         << REG3_SHIFT   ) | \
    (ABS(off)       << REG1_SHIFT   ) | \
    (1              << 26           ) | \
    (((((off) >> 31)&1)^1)<< INCDEC_SHIFT ) | \
    ((extra) ^ (IMM_BIT)))

#define ldmstm(cond, base, mask, extra) \
    (((cond)        << COND_SHIFT   ) | \
    ((base)         << REG3_SHIFT   ) | \
    (4              << 25           ) | \
    ((mask)         << REG1_SHIFT   ) | \
    ((extra)))


#define arm_add(cond, dst, op1, op2, extra) dataop(OP_ADD, cond, dst, op1, op2, extra)
#define arm_and(cond, dst, op1, op2, extra) dataop(OP_AND, cond, dst, op1, op2, extra)
#define arm_mov(cond, dst, op1, extra) dataop(OP_MOV, cond, dst, 0, op1, extra)
#define arm_cmp(cond, op1, op2, extra) dataop(OP_CMP, cond, 0, op1, op2, (extra)|(1<<20))
#define arm_cmn(cond, op1, op2, extra) dataop(OP_CMN, cond, 0, op1, op2, (extra)|(1<<20))
#define arm_mvn(cond, dst, op1, extra) dataop(OP_MVN, cond, dst, 0, op1, extra)
#define arm_mul(cond, dst, op1, op2, extra) mulop(OP_MUL, cond, dst, op1, op2, 0, extra)
#define arm_mla(cond, dst, op1, op2, acc, extra) mulop(OP_MLA, cond, dst, op1, op2, acc, extra)
#define arm_str(cond, src, base, off, extra) ldrstr(OP_ST, cond, src, base, off, extra)
#define arm_strb(cond, src, base, off, extra) arm_str(cond, src, base, off, extra)
#define arm_ldr(cond, dst, base, off, extra) ldrstr(OP_LD, cond, dst, base, off, extra)
#define arm_ldm(cond, base, mask, extra) ldmstm(cond, base, mask, extra|LDRSTR_BIT)
#define arm_stm(cond, base, mask, extra) ldmstm(cond, base, mask, extra)
#define arm_sub(cond, dst, op1, op2, extra) dataop(OP_SUB, cond, dst, op1, op2, extra)
#define arm_b(cond, target, extra) \
    (((cond)      << COND_SHIFT       ) | \
    (5            << 25               ) | \
    ((((target)>>2)&B_MASK)           ) | \
    (extra                            ))

#define arm_bx(cond, target, extra) \
    (((cond)      << COND_SHIFT       ) | \
    (BX_BITS                          ) | \
    (((target)&REG_MASK)              ) | \
    (extra                            ))

#define arm_shift_op2reg(op, amount, extra) \
    (((op)        << SHIFT_TYPE_SHIFT ) | \
    (((extra)&SHIFT_BY_REG_BIT)? ((amount)<< SHIFT_REG_SHIFT) : ((amount)<<SHIFT_IMM_SHIFT)) | \
    (extra                            ))

#define fmath(op, cond, dst, op1, op2, extra) \
    (((cond)      << COND_SHIFT       ) | \
    (7            << 25               ) | \
    (5            << 9                ) | \
    ((op)         << FOPCODE_SHIFT    ) | \
    (((dst) &  1) << FREG2_ODD_SHIFT  ) | \
    (((dst) >> 1) << REG2_SHIFT       ) | \
    (((op1) &  1) << FREG3_ODD_SHIFT  ) | \
    (((op1) >> 1) << REG3_SHIFT       ) | \
    (((op2) &  1) << FREG1_ODD_SHIFT  ) | \
    (((op2) >> 1) << REG1_SHIFT       ) | \
    (extra))

#define ftrans(op, cond, dst, src, extra) \
    (((cond)    << COND_SHIFT       ) | \
    ((op)       << FOPCODE_SHIFT    ) | \
    (7          << 25               ) | \
    (5          << 9                ) | \
    (1          << FOP_TRANSFER_SHIFT) | \
    ((src)      << REG2_SHIFT       ) | \
    (((dst) &  1) << FREG3_ODD_SHIFT) | \
    (((dst) >> 1) << REG3_SHIFT     ) | \
    (extra))

#define ABS(x) ((x) < 0 ? -((int)(x)) : (x))
#define vfp_fadd( cond, dst, op1, op2, extra)    fmath(OP_FADD, cond, dst, op1, op2, extra)
#define vfp_fsub( cond, dst, op1, op2, extra)    fmath(OP_FADD, cond, dst, op1, op2, extra|FSUB_BIT)
#define vfp_fmac( cond, dst, op1, op2, extra)    fmath(OP_FMAC, cond, dst, op1, op2, extra)
#define vfp_fnmac(cond, dst, op1, op2, extra)    fmath(OP_FMAC, cond, dst, op1, op2, extra|FSUB_BIT)
#define vfp_fmsc( cond, dst, op1, op2, extra)    fmath(OP_FMSC, cond, dst, op1, op2, extra)
#define vfp_fnmsc(cond, dst, op1, op2, extra)    fmath(OP_FMSC, cond, dst, op1, op2, extra|FSUB_BIT)
#define vfp_fmul( cond, dst, op1, op2, extra)    fmath(OP_FMUL, cond, dst, op1, op2, extra)
#define vfp_fdiv( cond, dst, op1, op2, extra)    fmath(OP_FDIV, cond, dst, op1, op2, extra)
#define vfp_fmrs( cond, dst, src, extra) ftrans(1, cond, src, dst, extra)
#define vfp_fmsr( cond, dst, src, extra) ftrans(0, cond, dst, src, extra)
#define vfp_fst( cond, freg, areg, offset, extra) \
    (((cond)        << COND_SHIFT     ) | \
    (16             << FOPCODE_SHIFT  ) | \
    (6              << 25             ) | \
    (5              << 9              ) | \
    ((areg)         << REG3_SHIFT     ) | \
    (((freg) &  1)  << FREG2_ODD_SHIFT) | \
    (((freg) >> 1)  << REG2_SHIFT     ) | \
    (((((offset)>>31)&1)^1) << 23     ) | \
    ((ABS(offset)>>2)&0xff            ) | \
    (extra))

#define vfp_fld( cond, freg, areg, offset, extra) vfp_fst(cond, freg, areg, offset, extra | (1 << FOPCODE_SHIFT))



#include <string.h>

static void disasm(int machine)
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
            for (int i = 0; i <= REG_R15; i++)
            {
                if (arglist & (1 << i))
                {
                    if (!firstReg)
                        strcat(arg, ", ");
                    else
                        firstReg = 0;
                    strcat(arg, regnames[i]);
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
            sprintf(arg, " 0x%x", machine & B_MASK);
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

#endif
