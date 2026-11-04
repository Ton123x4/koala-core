#ifndef __KOALA_BUNDLE_HEADER__
#define __KOALA_BUNDLE_HEADER__

#include <stddef.h>

typedef struct KoalaBundle KoalaBundle;

typedef struct {
    void* data;
    size_t size;
} KoalaBundleEntry;

KoalaBundle* KoalaBundleInit(const char* path);

void KoalaBundleRelease(KoalaBundle* bundle);


KoalaBundleEntry* KoalaBundleLoadEntry(KoalaBundle* bundle, const char* entry_name);

void KoalaBundleReleaseEntry(KoalaBundleEntry* entry);


char* KoalaBundleLoadString(KoalaBundle* bundle, const char* entry_name);

void KoalaBundleReleaseString(char* string);


char* KoalaBundleLoadPayload(KoalaBundle* bundle, const char* name);

KoalaBundleEntry* KoalaBundleLoadResource(KoalaBundle* bundle, const char* name);

char* KoalaBundleLoadResourceString(KoalaBundle* bundle, const char* name);

#endif
