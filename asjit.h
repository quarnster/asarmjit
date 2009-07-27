#ifndef ASJIT_H
#define ASJIT_H

#include "angelscript.h"
#include "vector"
#include <jit/jit.h>

class Section;
class asJIT : public asIJITCompiler
{
public:
    asJIT(asIScriptEngine *engine);
    virtual ~asJIT();
    
    virtual int StartCompile(const asDWORD *bytecode, asUINT bytecodeLen, asJITFunction *output);
    virtual int ResolveJitEntry(asUINT bytecodeOffset);
    virtual void EndCompile();
    
    virtual void ReleaseJITFunction(asJITFunction jitFunction);
    
private:
    
    void    SplitIntoSections(const asDWORD* bytecode, int bytecodeLen);
    int bytecodePos;
    Section *currSection;
    std::vector<Section> sections;
    int     m_refCount;
    asIScriptEngine *m_engine;
    
    const asDWORD *bytecode;

    jit_context_t context;
    friend class Section;
};


#endif
