#ifndef __KOALA_MANIFEST_HEADER__
#define __KOALA_MANIFEST_HEADER__

#include <stddef.h>

typedef struct KoalaManifest KoalaManifest;

KoalaManifest* KoalaManifestInit(const char* data);

KoalaManifest* KoalaManifestInitWithSize(const void* data, size_t size);

void KoalaManifestRelease(KoalaManifest* manifest);

const char* KoalaManifestGetValue(KoalaManifest* manifest, const char* key);

#endif
