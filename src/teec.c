/**
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2025 Bardia Moshiri <bardia@furilabs.com>
 */

#include <hybris/common/dlfcn.h>
#include <dlfcn.h>

#include "teec.h"

typedef struct {
    void *handle;

    TEEC_Result (*TEEC_InitializeContext)(const char *name, struct TEEC_Context *context);
    void        (*TEEC_FinalizeContext)(struct TEEC_Context *context);

    TEEC_Result (*TEEC_OpenSession)(struct TEEC_Context *context,
                                    struct TEEC_Session *session,
                                    const struct TEEC_UUID *destination,
                                    uint32_t connectionMethod,
                                    const void *connectionData,
                                    struct TEEC_Operation *operation,
                                    uint32_t *returnOrigin);

    void        (*TEEC_CloseSession)(struct TEEC_Session *session);

    TEEC_Result (*TEEC_InvokeCommand)(struct TEEC_Session *session,
                                      uint32_t commandID,
                                      struct TEEC_Operation *operation,
                                      uint32_t *returnOrigin);

    TEEC_Result (*TEEC_RegisterSharedMemory)(struct TEEC_Context *context,
                                             struct TEEC_SharedMemory *sharedMem);

    TEEC_Result (*TEEC_RegisterSharedMemoryFileDescriptor)(TEEC_Context *context,
                                                           TEEC_SharedMemory *sharedMem,
                                                           int fd);

    TEEC_Result (*TEEC_AllocateSharedMemory)(struct TEEC_Context *context,
                                             struct TEEC_SharedMemory *sharedMem);

    void        (*TEEC_ReleaseSharedMemory)(struct TEEC_SharedMemory *sharedMemory);

    void        (*TEEC_RequestCancellation)(struct TEEC_Operation *operation);
} TeecImpl;

static TeecImpl teec_impl;
static gboolean teec_loaded = FALSE;
static GMutex teec_load_mutex;

gboolean
teec_is_loaded(void)
{
    return teec_loaded;
}

gboolean
teec_init(void)
{
    g_mutex_lock(&teec_load_mutex);

    if (teec_loaded) {
        g_mutex_unlock(&teec_load_mutex);
        return TRUE;
    }

    teec_impl.handle = hybris_dlopen("libTEECommon.so", RTLD_LAZY);
    if (!teec_impl.handle) {
        g_warning("teec: hybris_dlopen(libTEECommon.so) failed");
        g_mutex_unlock(&teec_load_mutex);
        return FALSE;
    }

    teec_impl.TEEC_InitializeContext =
        (TEEC_Result (*)(const char*, struct TEEC_Context*))
        hybris_dlsym(teec_impl.handle, "TEEC_InitializeContext");

    teec_impl.TEEC_FinalizeContext =
        (void (*)(struct TEEC_Context*))
        hybris_dlsym(teec_impl.handle, "TEEC_FinalizeContext");

    teec_impl.TEEC_OpenSession =
        (TEEC_Result (*)(struct TEEC_Context*, struct TEEC_Session*, const struct TEEC_UUID*,
                         uint32_t, const void*, struct TEEC_Operation*, uint32_t*))
        hybris_dlsym(teec_impl.handle, "TEEC_OpenSession");

    teec_impl.TEEC_CloseSession =
        (void (*)(struct TEEC_Session*))
        hybris_dlsym(teec_impl.handle, "TEEC_CloseSession");

    teec_impl.TEEC_InvokeCommand =
        (TEEC_Result (*)(struct TEEC_Session*, uint32_t, struct TEEC_Operation*, uint32_t*))
        hybris_dlsym(teec_impl.handle, "TEEC_InvokeCommand");

    teec_impl.TEEC_RegisterSharedMemory =
        (TEEC_Result (*)(struct TEEC_Context*, struct TEEC_SharedMemory*))
        hybris_dlsym(teec_impl.handle, "TEEC_RegisterSharedMemory");

    teec_impl.TEEC_RegisterSharedMemoryFileDescriptor =
        (TEEC_Result (*)(TEEC_Context*, TEEC_SharedMemory*, int))
        hybris_dlsym(teec_impl.handle, "TEEC_RegisterSharedMemoryFileDescriptor");

    teec_impl.TEEC_AllocateSharedMemory =
        (TEEC_Result (*)(struct TEEC_Context*, struct TEEC_SharedMemory*))
        hybris_dlsym(teec_impl.handle, "TEEC_AllocateSharedMemory");

    teec_impl.TEEC_ReleaseSharedMemory =
        (void (*)(struct TEEC_SharedMemory*))
        hybris_dlsym(teec_impl.handle, "TEEC_ReleaseSharedMemory");

    teec_impl.TEEC_RequestCancellation =
        (void (*)(struct TEEC_Operation*))
        hybris_dlsym(teec_impl.handle, "TEEC_RequestCancellation");

    if (!teec_impl.TEEC_InitializeContext ||
        !teec_impl.TEEC_FinalizeContext ||
        !teec_impl.TEEC_OpenSession ||
        !teec_impl.TEEC_CloseSession ||
        !teec_impl.TEEC_InvokeCommand ||
        !teec_impl.TEEC_RegisterSharedMemory ||
        !teec_impl.TEEC_RegisterSharedMemoryFileDescriptor ||
        !teec_impl.TEEC_AllocateSharedMemory ||
        !teec_impl.TEEC_ReleaseSharedMemory ||
        !teec_impl.TEEC_RequestCancellation) {

        g_warning("teec: missing one or more TEEC symbols in libTEECommon.so");

        hybris_dlclose(teec_impl.handle);
        teec_impl.handle = NULL;

        teec_impl.TEEC_InitializeContext = NULL;
        teec_impl.TEEC_FinalizeContext = NULL;
        teec_impl.TEEC_OpenSession = NULL;
        teec_impl.TEEC_CloseSession = NULL;
        teec_impl.TEEC_InvokeCommand = NULL;
        teec_impl.TEEC_RegisterSharedMemory = NULL;
        teec_impl.TEEC_RegisterSharedMemoryFileDescriptor = NULL;
        teec_impl.TEEC_AllocateSharedMemory = NULL;
        teec_impl.TEEC_ReleaseSharedMemory = NULL;
        teec_impl.TEEC_RequestCancellation = NULL;

        teec_loaded = FALSE;

        g_mutex_unlock(&teec_load_mutex);
        return FALSE;
    }

    teec_loaded = TRUE;
    g_debug("teec: loaded and resolved libTEECommon.so symbols");

    g_mutex_unlock(&teec_load_mutex);
    return TRUE;
}

gboolean
teec_deinit(void)
{
    g_mutex_lock(&teec_load_mutex);

    if (!teec_loaded) {
        g_mutex_unlock(&teec_load_mutex);
        return TRUE;
    }

    if (teec_impl.handle) {
        hybris_dlclose(teec_impl.handle);
        teec_impl.handle = NULL;
    }

    teec_impl.TEEC_InitializeContext = NULL;
    teec_impl.TEEC_FinalizeContext = NULL;
    teec_impl.TEEC_OpenSession = NULL;
    teec_impl.TEEC_CloseSession = NULL;
    teec_impl.TEEC_InvokeCommand = NULL;
    teec_impl.TEEC_RegisterSharedMemory = NULL;
    teec_impl.TEEC_RegisterSharedMemoryFileDescriptor = NULL;
    teec_impl.TEEC_AllocateSharedMemory = NULL;
    teec_impl.TEEC_ReleaseSharedMemory = NULL;
    teec_impl.TEEC_RequestCancellation = NULL;

    teec_loaded = FALSE;

    g_debug("teec: deinitialized");

    g_mutex_unlock(&teec_load_mutex);
    return TRUE;
}

TEEC_Result
teec_initialize_context(const char *name, struct TEEC_Context *context)
{
    if (!teec_is_loaded())
        return TEEC_ERROR_NOT_SUPPORTED;

    return teec_impl.TEEC_InitializeContext(name, context);
}

gboolean
teec_finalize_context(struct TEEC_Context *context)
{
    if (!teec_is_loaded())
        return FALSE;

    teec_impl.TEEC_FinalizeContext(context);
    return TRUE;
}

TEEC_Result
teec_open_session(struct TEEC_Context *context,
                  struct TEEC_Session *session,
                  const struct TEEC_UUID *destination,
                  uint32_t connectionMethod,
                  const void *connectionData,
                  struct TEEC_Operation *operation,
                  uint32_t *returnOrigin)
{
    if (!teec_is_loaded())
        return TEEC_ERROR_NOT_SUPPORTED;

    return teec_impl.TEEC_OpenSession(context, session, destination,
                                     connectionMethod, connectionData,
                                     operation, returnOrigin);
}

gboolean
teec_close_session(struct TEEC_Session *session)
{
    if (!teec_is_loaded())
        return FALSE;

    teec_impl.TEEC_CloseSession(session);
    return TRUE;
}

TEEC_Result
teec_invoke_command(struct TEEC_Session *session,
                    uint32_t commandID,
                    struct TEEC_Operation *operation,
                    uint32_t *returnOrigin)
{
    if (!teec_is_loaded())
        return TEEC_ERROR_NOT_SUPPORTED;

    return teec_impl.TEEC_InvokeCommand(session, commandID, operation, returnOrigin);
}

TEEC_Result
teec_register_shared_memory(struct TEEC_Context *context,
                            struct TEEC_SharedMemory *sharedMem)
{
    if (!teec_is_loaded())
        return TEEC_ERROR_NOT_SUPPORTED;

    return teec_impl.TEEC_RegisterSharedMemory(context, sharedMem);
}

TEEC_Result
teec_register_shared_memory_file_descriptor(TEEC_Context *context,
                                            TEEC_SharedMemory *sharedMem,
                                            int fd)
{
    if (!teec_is_loaded())
        return TEEC_ERROR_NOT_SUPPORTED;

    return teec_impl.TEEC_RegisterSharedMemoryFileDescriptor(context, sharedMem, fd);
}

TEEC_Result
teec_allocate_shared_memory(struct TEEC_Context *context,
                            struct TEEC_SharedMemory *sharedMem)
{
    if (!teec_is_loaded())
        return TEEC_ERROR_NOT_SUPPORTED;

    return teec_impl.TEEC_AllocateSharedMemory(context, sharedMem);
}

gboolean
teec_release_shared_memory(struct TEEC_SharedMemory *sharedMemory)
{
    if (!teec_is_loaded())
        return FALSE;

    teec_impl.TEEC_ReleaseSharedMemory(sharedMemory);
    return TRUE;
}

gboolean
teec_request_cancellation(struct TEEC_Operation *operation)
{
    if (!teec_is_loaded())
        return FALSE;

    teec_impl.TEEC_RequestCancellation(operation);
    return TRUE;
}
