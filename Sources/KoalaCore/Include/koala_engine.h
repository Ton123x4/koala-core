#ifndef __KOALA_ENGINE_HEADER__
#define __KOALA_ENGINE_HEADER__

typedef struct KoalaEngine KoalaEngine;
typedef struct KoalaBundle KoalaBundle;

typedef void (*KoalaEngineOutputHandler)(const char* message);

KoalaEngine* KoalaEngineInit(KoalaBundle* bundle, const char* script);

void KoalaEngineRelease(KoalaEngine* engine);

int KoalaEngineRun(KoalaEngine* engine);

int KoalaEngineAddHandler(KoalaEngine* engine, const char* name, void* handler);

void* KoalaEngineGetHandler(KoalaEngine* engine, const char* name);

void KoalaEngineSetStandardOutput(KoalaEngine* engine, KoalaEngineOutputHandler handler);

void KoalaEngineSetStandardError(KoalaEngine* engine, KoalaEngineOutputHandler handler);

#endif
