#ifndef __INCLUDED_SECTION_H
#define __INCLUDED_SECTION_H

#include <jit/jit.h>

class Section
{
public:
    Section(int start, int end);
    ~Section();

    int byteCodeStart;
    int byteCodeEnd;

    bool isJitEntry;
    jit_label_t label;
};



#endif
