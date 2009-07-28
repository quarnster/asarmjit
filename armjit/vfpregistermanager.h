#ifndef __INCLUDED_VFPREGISTERMANAGER_H
#define __INCLUDED_VFPREGISTERMANAGER_H

#include "registermanager.h"

class VFPRegisterManager : public RegisterManager
{
public:
    VFPRegisterManager(asCJitArm *j);
    virtual ~VFPRegisterManager();

    virtual void LoadRegister(int asRegister, int native);
    virtual void SaveRegister(int asRegister, int native);
};



#endif
