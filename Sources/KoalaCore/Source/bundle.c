#define _CRT_SECURE_NO_WARNINGS

#include "koala_bundle.h"

#include <stdlib.h>
#include <string.h>

#include <miniz.h>

struct KoalaBundle {
    mz_zip_archive archive;
};

KoalaBundle* KoalaBundleInit(const char* path) {
    if (!path) {
        return NULL;
    }

    KoalaBundle* bundle = malloc(sizeof(KoalaBundle));

    if (!bundle) {
        return NULL;
    }

    memset(bundle, 0, sizeof(KoalaBundle));

    if (!mz_zip_reader_init_file(&bundle->archive, path, 0)) {
        free(bundle);
        return NULL;
    }

    return bundle;
}

void KoalaBundleRelease(KoalaBundle* bundle) {
    if (!bundle) {
        return;
    }

    mz_zip_reader_end(&bundle->archive);
    free(bundle);
}

KoalaBundleEntry* KoalaBundleLoadEntry(KoalaBundle* bundle, const char* entry_name) {
    if (!bundle || !entry_name) {
        return NULL;
    }

    int file_index = mz_zip_reader_locate_file(&bundle->archive, entry_name, NULL, 0);

    if (file_index < 0) {
        return NULL;
    }

    size_t size = 0;
    void* data = mz_zip_reader_extract_to_heap(&bundle->archive, (mz_uint)file_index, &size, 0);

    if (!data && size != 0) {
        return NULL;
    }

    KoalaBundleEntry* entry = malloc(sizeof(KoalaBundleEntry));

    if (!entry) {
        mz_free(data);
        return NULL;
    }

    entry->data = data;
    entry->size = size;

    return entry;
}

void KoalaBundleReleaseEntry(KoalaBundleEntry* entry) {
    if (!entry) {
        return;
    }

    mz_free(entry->data);
    free(entry);
}

char* KoalaBundleLoadString(KoalaBundle* bundle, const char* entry_name) {
    if (!bundle || !entry_name) {
        return NULL;
    }

    int file_index = mz_zip_reader_locate_file(&bundle->archive, entry_name, NULL, 0);

    if (file_index < 0) {
        return NULL;
    }

    mz_zip_archive_file_stat stat;

    if (!mz_zip_reader_file_stat(&bundle->archive, (mz_uint)file_index, &stat)) {
        return NULL;
    }

    if (stat.m_uncomp_size > SIZE_MAX - 1) {
        return NULL;
    }

    size_t size = (size_t)stat.m_uncomp_size;

    char* string = malloc(size + 1);

    if (!string) {
        return NULL;
    }

    if (size > 0 && !mz_zip_reader_extract_to_mem(&bundle->archive, (mz_uint)file_index, string, size, 0)) {
        free(string);
        return NULL;
    }

    string[size] = '\0';

    return string;
}

void KoalaBundleReleaseString(char* string) {
    free(string);
}

char* KoalaBundleLoadPayload(KoalaBundle* bundle, const char* name) {
    if (!bundle || !name) {
        return NULL;
    }

    size_t path_size = strlen("Payload/") + strlen(name) + 1;
    char* path = malloc(path_size);

    if (!path) {
        return NULL;
    }

    strcpy(path, "Payload/");
    strcat(path, name);

    char* string = KoalaBundleLoadString(bundle, path);

    free(path);

    return string;
}

KoalaBundleEntry* KoalaBundleLoadResource(KoalaBundle* bundle, const char* name) {
    if (!bundle || !name) {
        return NULL;
    }

    size_t path_size = strlen("Resources/") + strlen(name) + 1;
    char* path = malloc(path_size);

    if (!path) {
        return NULL;
    }

    strcpy(path, "Resources/");
    strcat(path, name);

    KoalaBundleEntry* entry = KoalaBundleLoadEntry(bundle, path);

    free(path);

    return entry;
}

char* KoalaBundleLoadResourceString(KoalaBundle* bundle, const char* name) {
    if (!bundle || !name) {
        return NULL;
    }

    size_t path_size = strlen("Resources/") + strlen(name) + 1;
    char* path = malloc(path_size);

    if (!path) {
        return NULL;
    }

    strcpy(path, "Resources/");
    strcat(path, name);

    char* string = KoalaBundleLoadString(bundle, path);

    free(path);

    return string;
}
