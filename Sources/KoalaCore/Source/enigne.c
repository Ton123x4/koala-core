#define _CRT_SECURE_NO_WARNINGS

#include "koala_engine.h"
#include "koala_bundle.h"

#include <stdlib.h>
#include <string.h>

#include <quickjs.h>

typedef struct KoalaEngineHandler {
    char* name;
    void* callback;
    struct KoalaEngineHandler* next;
} KoalaEngineHandler;

struct KoalaEngine {
    KoalaBundle* bundle;
    char* script;

    JSRuntime* runtime;
    JSContext* context;

    KoalaEngineHandler* handlers;

    KoalaEngineOutputHandler standard_output;
    KoalaEngineOutputHandler standard_error;
};

static char* duplicateString(const char* string) {
    if (!string) {
        return NULL;
    }

    size_t size = strlen(string);
    char* result = malloc(size + 1);

    if (!result) {
        return NULL;
    }

    memcpy(result, string, size + 1);

    return result;
}

static KoalaEngineHandler* findHandler(KoalaEngine* engine, const char* name) {
    KoalaEngineHandler* handler = engine->handlers;

    while (handler) {
        if (strcmp(handler->name, name) == 0) {
            return handler;
        }

        handler = handler->next;
    }

    return NULL;
}

void* KoalaEngineGetHandler(KoalaEngine* engine, const char* name) {
    if (!engine || !name) {
        return NULL;
    }

    KoalaEngineHandler* handler = findHandler(engine, name);

    if (!handler) {
        return NULL;
    }

    return handler->callback;
}

int KoalaEngineAddHandler(KoalaEngine* engine, const char* name, void* value) {
    if (!engine || !name) {
        return 0;
    }

    if (findHandler(engine, name)) {
        return 0;
    }

    KoalaEngineHandler* handler = malloc(sizeof(KoalaEngineHandler));

    if (!handler) {
        return 0;
    }

    handler->name = duplicateString(name);

    if (!handler->name) {
        free(handler);
        return 0;
    }

    handler->callback = value;
    handler->next = NULL;

    if (!engine->handlers) {
        engine->handlers = handler;
    }
    else {
        KoalaEngineHandler* current = engine->handlers;

        while (current->next) {
            current = current->next;
        }

        current->next = handler;
    }

    return 1;
}

void KoalaEngineSetStandardOutput(KoalaEngine* engine, KoalaEngineOutputHandler handler) {
    if (!engine) {
        return;
    }

    engine->standard_output = handler;
}

void KoalaEngineSetStandardError(KoalaEngine* engine, KoalaEngineOutputHandler handler) {
    if (!engine) {
        return;
    }

    engine->standard_error = handler;
}

static void writeStandardOutput(KoalaEngine* engine, const char* message) {
    if (!engine || !message) {
        return;
    }

    if (engine->standard_output) {
        engine->standard_output(message);
    }
}

static void writeStandardError(KoalaEngine* engine, const char* message) {
    if (!engine || !message) {
        return;
    }

    if (engine->standard_error) {
        engine->standard_error(message);
    }
}

static JSValue consoleLog(JSContext* context, JSValueConst this_val, int argc, JSValueConst* argv) {
    KoalaEngine* engine = JS_GetContextOpaque(context);

    if (!engine) {
        return JS_ThrowInternalError(context, "engine is not available");
    }

    size_t size = 1;

    for (int i = 0; i < argc; i++) {
        const char* string = JS_ToCString(context, argv[i]);

        if (!string) {
            return JS_EXCEPTION;
        }

        size += strlen(string);

        if (i > 0) {
            size++;
        }

        JS_FreeCString(context, string);
    }

    char* message = malloc(size);

    if (!message) {
        return JS_ThrowOutOfMemory(context);
    }

    message[0] = '\0';

    for (int i = 0; i < argc; i++) {
        const char* string = JS_ToCString(context, argv[i]);

        if (!string) {
            free(message);
            return JS_EXCEPTION;
        }

        if (i > 0) {
            strcat(message, " ");
        }

        strcat(message, string);

        JS_FreeCString(context, string);
    }

    writeStandardOutput(engine, message);

    free(message);

    return JS_UNDEFINED;
}

static JSValue consoleError(JSContext* context, JSValueConst this_val, int argc, JSValueConst* argv) {
    KoalaEngine* engine = JS_GetContextOpaque(context);

    if (!engine) {
        return JS_ThrowInternalError(context, "engine is not available");
    }

    size_t size = 1;

    for (int i = 0; i < argc; i++) {
        const char* string = JS_ToCString(context, argv[i]);

        if (!string) {
            return JS_EXCEPTION;
        }

        size += strlen(string);

        if (i > 0) {
            size++;
        }

        JS_FreeCString(context, string);
    }

    char* message = malloc(size);

    if (!message) {
        return JS_ThrowOutOfMemory(context);
    }

    message[0] = '\0';

    for (int i = 0; i < argc; i++) {
        const char* string = JS_ToCString(context, argv[i]);

        if (!string) {
            free(message);
            return JS_EXCEPTION;
        }

        if (i > 0) {
            strcat(message, " ");
        }

        strcat(message, string);

        JS_FreeCString(context, string);
    }

    writeStandardError(engine, message);

    free(message);

    return JS_UNDEFINED;
}

static const JSCFunctionListEntry consoleFunctions[] = {
    JS_CFUNC_DEF("log", 1, consoleLog),
    JS_CFUNC_DEF("error", 1, consoleError)
};

static int createConsole(JSContext* context) {
    JSValue global = JS_GetGlobalObject(context);
    JSValue console = JS_NewObject(context);

    if (JS_IsException(console)) {
        JS_FreeValue(context, global);
        return 0;
    }

    if (JS_SetPropertyFunctionList(context, console, consoleFunctions, sizeof(consoleFunctions) / sizeof(consoleFunctions[0])) < 0) {
        JS_FreeValue(context, console);
        JS_FreeValue(context, global);
        return 0;
    }

    if (JS_SetPropertyStr(context, global, "console", console) < 0) {
        JS_FreeValue(context, console);
        JS_FreeValue(context, global);
        return 0;
    }

    JS_FreeValue(context, global);

    return 1;
}

static JSValue metalGetHandler(JSContext* context, JSValueConst this_val, int argc, JSValueConst* argv) {
    KoalaEngine* engine = JS_GetContextOpaque(context);

    if (!engine || argc < 1 || !JS_IsString(argv[0])) {
        return JS_ThrowTypeError(context, "handler name must be a string");
    }

    const char* name = JS_ToCString(context, argv[0]);

    if (!name) {
        return JS_EXCEPTION;
    }

    KoalaEngineHandler* handler = findHandler(engine, name);

    JS_FreeCString(context, name);

    if (!handler) {
        return JS_UNDEFINED;
    }

    if (!handler->callback) {
        return JS_NULL;
    }

    return JS_NewCFunction(context, handler->callback, handler->name, 0);
}

static JSValue metalGetResource(JSContext* context, JSValueConst this_val, int argc, JSValueConst* argv) {
    KoalaEngine* engine = JS_GetContextOpaque(context);

    if (!engine || argc < 1 || !JS_IsString(argv[0])) {
        return JS_ThrowTypeError(context, "resource name must be a string");
    }

    const char* name = JS_ToCString(context, argv[0]);

    if (!name) {
        return JS_EXCEPTION;
    }

    KoalaBundleEntry* entry = KoalaBundleLoadResource(engine->bundle, name);

    JS_FreeCString(context, name);

    if (!entry) {
        return JS_UNDEFINED;
    }

    JSValue result = JS_NewArrayBufferCopy(context, entry->data, entry->size);

    KoalaBundleReleaseEntry(entry);

    return result;
}

static JSValue metalGetTextResource(JSContext* context, JSValueConst this_val, int argc, JSValueConst* argv) {
    KoalaEngine* engine = JS_GetContextOpaque(context);

    if (!engine || argc < 1 || !JS_IsString(argv[0])) {
        return JS_ThrowTypeError(context, "resource name must be a string");
    }

    const char* name = JS_ToCString(context, argv[0]);

    if (!name) {
        return JS_EXCEPTION;
    }

    char* string = KoalaBundleLoadResourceString(engine->bundle, name);

    JS_FreeCString(context, name);

    if (!string) {
        return JS_UNDEFINED;
    }

    JSValue result = JS_NewString(context, string);

    KoalaBundleReleaseString(string);

    return result;
}

static JSValue metalHasResource(JSContext* context, JSValueConst this_val, int argc, JSValueConst* argv) {
    KoalaEngine* engine = JS_GetContextOpaque(context);

    if (!engine || argc < 1 || !JS_IsString(argv[0])) {
        return JS_ThrowTypeError(context, "resource name must be a string");
    }

    const char* name = JS_ToCString(context, argv[0]);

    if (!name) {
        return JS_EXCEPTION;
    }

    KoalaBundleEntry* entry = KoalaBundleLoadResource(engine->bundle, name);

    JS_FreeCString(context, name);

    if (!entry) {
        return JS_FALSE;
    }

    KoalaBundleReleaseEntry(entry);

    return JS_TRUE;
}

static const JSCFunctionListEntry metalFunctions[] = {
    JS_CFUNC_DEF("getHandler", 1, metalGetHandler),
    JS_CFUNC_DEF("getResource", 1, metalGetResource),
    JS_CFUNC_DEF("getTextResource", 1, metalGetTextResource),
    JS_CFUNC_DEF("hasResource", 1, metalHasResource)
};

static int createMetal(JSContext* context) {
    JSValue global = JS_GetGlobalObject(context);
    JSValue metal = JS_NewObject(context);

    if (JS_IsException(metal)) {
        JS_FreeValue(context, global);
        return 0;
    }

    if (JS_SetPropertyFunctionList(context, metal, metalFunctions, sizeof(metalFunctions) / sizeof(metalFunctions[0])) < 0) {
        JS_FreeValue(context, metal);
        JS_FreeValue(context, global);
        return 0;
    }

    if (JS_SetPropertyStr(context, global, "KoalaMetal", metal) < 0) {
        JS_FreeValue(context, metal);
        JS_FreeValue(context, global);
        return 0;
    }

    JS_FreeValue(context, global);

    return 1;
}

static JSModuleDef* moduleLoader(JSContext* context, const char* module_name, void* opaque) {
    KoalaEngine* engine = opaque;

    if (!engine || !module_name) {
        return NULL;
    }

    char* source = KoalaBundleLoadPayload(engine->bundle, module_name);

    if (!source) {
        JS_ThrowReferenceError(context, "module '%s' not found", module_name);
        return NULL;
    }

    size_t size = strlen(source);
    JSValue module = JS_Eval(context, source, size, module_name, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

    KoalaBundleReleaseString(source);

    if (JS_IsException(module)) {
        return NULL;
    }

    JSModuleDef* definition = JS_VALUE_GET_PTR(module);

    JS_FreeValue(context, module);

    return definition;
}

KoalaEngine* KoalaEngineInit(KoalaBundle* bundle, const char* script) {
    if (!bundle || !script || !script[0]) {
        return NULL;
    }

    KoalaEngine* engine = calloc(1, sizeof(KoalaEngine));

    if (!engine) {
        return NULL;
    }

    engine->bundle = bundle;
    engine->script = duplicateString(script);

    if (!engine->script) {
        free(engine);
        return NULL;
    }

    engine->runtime = JS_NewRuntime();

    if (!engine->runtime) {
        free(engine->script);
        free(engine);
        return NULL;
    }

    engine->context = JS_NewContext(engine->runtime);

    if (!engine->context) {
        JS_FreeRuntime(engine->runtime);
        free(engine->script);
        free(engine);
        return NULL;
    }

    JS_SetContextOpaque(engine->context, engine);
    JS_SetModuleLoaderFunc(engine->runtime, NULL, moduleLoader, engine);

    if (!createConsole(engine->context)) {
        JS_FreeContext(engine->context);
        JS_FreeRuntime(engine->runtime);
        free(engine->script);
        free(engine);
        return NULL;
    }

    if (!createMetal(engine->context)) {
        JS_FreeContext(engine->context);
        JS_FreeRuntime(engine->runtime);
        free(engine->script);
        free(engine);
        return NULL;
    }

    return engine;
}

void KoalaEngineRelease(KoalaEngine* engine) {
    if (!engine) {
        return;
    }

    KoalaEngineHandler* handler = engine->handlers;

    while (handler) {
        KoalaEngineHandler* next = handler->next;

        free(handler->name);
        free(handler);

        handler = next;
    }

    if (engine->context) {
        JS_FreeContext(engine->context);
    }

    if (engine->runtime) {
        JS_FreeRuntime(engine->runtime);
    }

    free(engine->script);
    free(engine);
}

int KoalaEngineRun(KoalaEngine* engine) {
    if (!engine || !engine->context || !engine->script) {
        return 0;
    }

    char* source = KoalaBundleLoadPayload(engine->bundle, engine->script);

    if (!source) {
        writeStandardError(engine, "Failed to load entrypoint\n");
        return 0;
    }

    size_t size = strlen(source);
    JSValue result = JS_Eval(engine->context, source, size, engine->script, JS_EVAL_TYPE_MODULE);

    KoalaBundleReleaseString(source);

    if (JS_IsException(result)) {
        JSValue exception = JS_GetException(engine->context);
        const char* error = JS_ToCString(engine->context, exception);

        if (error) {
            writeStandardError(engine, error);
            writeStandardError(engine, "\n");
            JS_FreeCString(engine->context, error);
        }

        JS_FreeValue(engine->context, exception);
        JS_FreeValue(engine->context, result);

        return 0;
    }

    JS_FreeValue(engine->context, result);

    while (JS_IsJobPending(engine->runtime)) {
        JSContext* context = NULL;

        if (JS_ExecutePendingJob(engine->runtime, &context) < 0) {
            JSContext* errorContext = context ? context : engine->context;
            JSValue exception = JS_GetException(errorContext);
            const char* error = JS_ToCString(errorContext, exception);

            if (error) {
                writeStandardError(engine, error);
                writeStandardError(engine, "\n");
                JS_FreeCString(errorContext, error);
            }

            JS_FreeValue(errorContext, exception);

            return 0;
        }
    }

    return 1;
}
