#ifndef __INCLUDED_ARMREGISTERMANAGER_H
#define __INCLUDED_ARMREGISTERMANAGER_H

#include "registermanager.h"

enum
{
    CALLCONV_FREE_REGISTERMASK = (1<<REG_R0)|(1<<REG_R1)|(1<<REG_R2)|(1<<REG_R3)|(1<<REG_R12)|(1<<REG_R14)
};

class ARMRegisterManager : public RegisterManager
{
public:
    ARMRegisterManager(asCJitArm *j);
    virtual ~ARMRegisterManager();

    virtual void LoadRegister(int asRegister, int native);
    virtual void SaveRegister(int asRegister, int native);
};



#endif
