#include "koala_manifest.h"

#include <stdlib.h>
#include <string.h>

typedef struct KoalaManifestEntry {
    char* key;
    char* value;
    struct KoalaManifestEntry* next;
} KoalaManifestEntry;

struct KoalaManifest {
    KoalaManifestEntry* entries;
};

static char* duplicateRange(const char* start, size_t length) {
    char* result = malloc(length + 1);

    if (!result) {
        return NULL;
    }

    memcpy(result, start, length);
    result[length] = '\0';

    return result;
}

static void trim(const char** start, size_t* length) {
    while (*length > 0 && ((*start)[0] == ' ' || (*start)[0] == '\t' || (*start)[0] == '\r')) {
        (*start)++;
        (*length)--;
    }

    while (*length > 0 && ((*start)[*length - 1] == ' ' || (*start)[*length - 1] == '\t' || (*start)[*length - 1] == '\r')) {
        (*length)--;
    }
}

static int addEntry(KoalaManifest* manifest, const char* key, size_t key_size, const char* value, size_t value_size) {
    KoalaManifestEntry* entry = malloc(sizeof(KoalaManifestEntry));

    if (!entry) {
        return 0;
    }

    entry->key = duplicateRange(key, key_size);

    if (!entry->key) {
        free(entry);
        return 0;
    }

    entry->value = duplicateRange(value, value_size);

    if (!entry->value) {
        free(entry->key);
        free(entry);
        return 0;
    }

    entry->next = NULL;

    if (!manifest->entries) {
        manifest->entries = entry;
        return 1;
    }

    KoalaManifestEntry* current = manifest->entries;

    while (current->next) {
        current = current->next;
    }

    current->next = entry;

    return 1;
}

KoalaManifest* KoalaManifestInit(const char* data) {
    if (!data) {
        return NULL;
    }

    return KoalaManifestInitWithSize(data, strlen(data));
}

KoalaManifest* KoalaManifestInitWithSize(const void* data, size_t size) {
    if (!data && size != 0) {
        return NULL;
    }

    KoalaManifest* manifest = calloc(1, sizeof(KoalaManifest));

    if (!manifest) {
        return NULL;
    }

    const char* buffer = data;
    size_t position = 0;

    while (position < size) {
        size_t line_start = position;

        while (position < size && buffer[position] != '\n') {
            position++;
        }

        size_t line_size = position - line_start;

        if (position < size) {
            position++;
        }

        const char* line = buffer + line_start;

        trim(&line, &line_size);

        if (line_size == 0 || line[0] == '#') {
            continue;
        }

        const char* separator = memchr(line, '=', line_size);

        if (!separator) {
            KoalaManifestRelease(manifest);
            return NULL;
        }

        const char* key = line;
        size_t key_size = (size_t)(separator - line);

        const char* value = separator + 1;
        size_t value_size = line_size - key_size - 1;

        trim(&key, &key_size);
        trim(&value, &value_size);

        if (key_size == 0) {
            KoalaManifestRelease(manifest);
            return NULL;
        }

        if (!addEntry(manifest, key, key_size, value, value_size)) {
            KoalaManifestRelease(manifest);
            return NULL;
        }
    }

    return manifest;
}

void KoalaManifestRelease(KoalaManifest* manifest) {
    if (!manifest) {
        return;
    }

    KoalaManifestEntry* entry = manifest->entries;

    while (entry) {
        KoalaManifestEntry* next = entry->next;

        free(entry->key);
        free(entry->value);
        free(entry);

        entry = next;
    }

    free(manifest);
}

const char* KoalaManifestGetValue(KoalaManifest* manifest, const char* key) {
    if (!manifest || !key) {
        return NULL;
    }

    KoalaManifestEntry* entry = manifest->entries;

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }

        entry = entry->next;
    }

    return NULL;
}
