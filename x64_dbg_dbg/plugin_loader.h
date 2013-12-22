#ifndef _PLUGIN_LOADER_H
#define _PLUGIN_LOADER_H

#include "_global.h"
#include "_plugins.h"

//typedefs
typedef bool (*PLUGINIT)(PLUG_INITSTRUCT* initStruct);
typedef bool (*PLUGSTOP)();

//structures
struct PLUG_DATA
{
    HINSTANCE hPlugin;
    PLUGINIT pluginit;
    PLUGSTOP plugstop;
    PLUG_INITSTRUCT initStruct;
};

struct PLUG_CALLBACK
{
    int pluginHandle;
    CBTYPE cbType;
    CBPLUGIN cbPlugin;
};

struct PLUG_COMMAND
{
    int pluginHandle;
    char command[deflen];
};

//plugin management functions
void pluginload(const char* pluginDir);
void pluginunload();
void pluginregistercallback(int pluginHandle, CBTYPE cbType, CBPLUGIN cbPlugin);
bool pluginunregistercallback(int pluginHandle, CBTYPE cbType);
void plugincbcall(CBTYPE cbType, void* callbackInfo);
bool plugincmdregister(int pluginHandle, const char* command, CBPLUGINCOMMAND cbCommand, bool debugonly);
bool plugincmdunregister(int pluginHandle, const char* command);

#endif // _PLUGIN_LOADER_H