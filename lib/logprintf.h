// To initialize logprintf do:
// logprintf = (logprintf_t)ppPluginData[PLUGIN_DATA_LOGPRINTF];

#ifndef _LOGPRINTF_H_INCLUDED
#define _LOGPRINTF_H_INCLUDED

typedef void (*logprintf_t)(const char *format, ...);

extern logprintf_t logprintf;

#endif
