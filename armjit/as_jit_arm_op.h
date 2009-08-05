
//
// as_jit_arm_op.h
//
// Opcode helper defines for the ARM machine code translator
// Written by Fredrik Ehnbom in June-August 2009

#ifndef AS_JIT_ARM_OP_H
#define AS_JIT_ARM_OP_H

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

void arm_disasm(int machine);

#endif
