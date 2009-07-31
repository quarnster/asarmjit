#ifndef __INCLUDED_SETTINGS_H
#define __INCLUDED_SETTINGS_H

typedef enum
{
    GLOBAL_LOAD_STORE,
    LOCAL_LOAD_STORE
} RegisterHandling;

class Settings
{
public:
    Settings()
    : regHandling(GLOBAL_LOAD_STORE)
    {
    }

    RegisterHandling regHandling;
};

#endif
