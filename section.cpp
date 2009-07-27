#include "section.h"

Section::Section(int start, int end)
{
    byteCodeStart = start;
    byteCodeEnd = end;
    isJitEntry = false;
    label = jit_label_undefined;
}

Section::~Section()
{
}
