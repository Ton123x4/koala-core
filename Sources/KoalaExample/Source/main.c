#include "koala_bundle.h"
#include "koala_manifest.h"
#include "koala_engine.h"

#include <quickjs.h>
#include <stdio.h>

static JSValue printHandler(JSContext* context, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(context, "print expects a message");
    }

    const char* message = JS_ToCString(context, argv[0]);

    if (!message) {
        return JS_EXCEPTION;
    }

    printf("%s\n", message);

    JS_FreeCString(context, message);

    return JS_UNDEFINED;
}

static JSValue echoHandler(JSContext* context, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(context, "echo expects a message");
    }

    return JS_DupValue(context, argv[0]);
}

static void logHandler(const char* message) {
    printf("[LOG] %s\n", message);
}

static void warnHandler(const char* message) {
    printf("[WARN] %s\n", message);
}

static void errorHandler(const char* message) {
    fprintf(stderr, "[ERROR] %s\n", message);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        return 1;
    }

    KoalaBundle* bundle = KoalaBundleInit(argv[1]);

    if (!bundle) {
        printf("Failed to load %s\n", argv[1]);
        return 1;
    }

    KoalaBundleEntry* manifest_entry = KoalaBundleLoadEntry(bundle, "Manifest.conf");

    if (!manifest_entry) {
        printf("Failed to load manifest.conf\n");
        KoalaBundleRelease(bundle);
        return 1;
    }

    KoalaManifest* manifest = KoalaManifestInitWithSize(manifest_entry->data, manifest_entry->size);

    KoalaBundleReleaseEntry(manifest_entry);

    if (!manifest) {
        printf("Failed to parse manifest\n");
        KoalaBundleRelease(bundle);
        return 1;
    }

    const char* entrypoint = KoalaManifestGetValue(manifest, "entrypoint");

    if (!entrypoint || !entrypoint[0]) {
        printf("Manifest does not define an entrypoint\n");
        KoalaManifestRelease(manifest);
        KoalaBundleRelease(bundle);
        return 1;
    }

    KoalaEngine* engine = KoalaEngineInit(bundle, entrypoint);

    if (!engine) {
        printf("Failed to initialize KoalaEngine\n");
        KoalaManifestRelease(manifest);
        KoalaBundleRelease(bundle);
        return 1;
    }

    if (!KoalaEngineAddHandler(engine, "print", printHandler)) {
        printf("Failed to register print handler\n");
    }

    if (!KoalaEngineAddHandler(engine, "echo", echoHandler)) {
        printf("Failed to register echo handler\n");
    }

    KoalaEngineSetStandardOutput(engine, logHandler);
    KoalaEngineSetStandardError(engine, logHandler);

    if (!KoalaEngineRun(engine)) {
        printf("Failed to run application\n");
        KoalaEngineRelease(engine);
        KoalaManifestRelease(manifest);
        KoalaBundleRelease(bundle);
        return 1;
    }

    printf("\nApplication finished successfully\n");

    KoalaEngineRelease(engine);
    KoalaManifestRelease(manifest);
    KoalaBundleRelease(bundle);

    return 0;
}
