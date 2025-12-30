/**
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2025 Bardia Moshiri <bardia@furilabs.com>
 */

#ifndef TEEC_H
#define TEEC_H

#include <glib.h>

#include "tee_client_api.h"

/**
 * Initialize the TEEC backend implementation.
 *
 * This loads libTEECommon.so.
 *
 * @return TRUE on success, FALSE on failure.
 */
gboolean
teec_init(void);

/**
 * Deinitialize the TEEC backend implementation.
 *
 * This unloads libTEECommon.so and clears all resolved symbols.
 *
 * @return TRUE on success (or if already deinitialized), FALSE on failure.
 */
gboolean
teec_deinit(void);

/**
 * Check whether the TEEC backend implementation is loaded.
 *
 * @return TRUE if teec_init() has successfully loaded the backend, FALSE otherwise.
 */
gboolean
teec_is_loaded(void);

/**
 * Initialize a context holding connection information on the specific TEE.
 *
 * This is a wrapper around TEEC_InitializeContext().
 *
 * @param name TEE name, expects NULL for default.
 * @param context Context to initialize.
 * @return TEEC_SUCCESS on success, or a TEEC_* error code on failure.
 */
TEEC_Result
teec_initialize_context(const char *name, struct TEEC_Context *context);

/**
 * Destroy a previously initialized TEE context.
 *
 * This is a wrapper around TEEC_FinalizeContext().
 *
 * @param context Context to destroy.
 * @return TRUE on success, FALSE if the TEEC backend isn't loaded.
 */
gboolean
teec_finalize_context(struct TEEC_Context *context);

/**
 * Open a new session with a Trusted Application.
 *
 * This is a wrapper around TEEC_OpenSession().
 *
 * @param context Initialized context.
 * @param session Session to initialize.
 * @param destination UUID of the Trusted Application.
 * @param connectionMethod Login method (TEEC_LOGIN_*).
 * @param connectionData Optional login data (typically NULL).
 * @param operation Optional operation (may be NULL).
 * @param returnOrigin Receives TEEC_ORIGIN_* on failure (may be NULL).
 * @return TEEC_SUCCESS on success, or a TEEC_* error code on failure.
 */
TEEC_Result
teec_open_session(struct TEEC_Context *context,
                  struct TEEC_Session *session,
                  const struct TEEC_UUID *destination,
                  uint32_t connectionMethod,
                  const void *connectionData,
                  struct TEEC_Operation *operation,
                  uint32_t *returnOrigin);

/**
 * Close a previously opened session.
 *
 * This is a wrapper around TEEC_CloseSession().
 *
 * @param session Session to close.
 * @return TRUE on success, FALSE if the TEEC backend isn't loaded.
 */
gboolean
teec_close_session(struct TEEC_Session *session);

/**
 * Invoke a command in the specified Trusted Application session.
 *
 * This is a wrapper around TEEC_InvokeCommand().
 *
 * @param session Open session handle.
 * @param commandID Command identifier in the Trusted Application.
 * @param operation Optional operation (may be NULL).
 * @param returnOrigin Receives TEEC_ORIGIN_* on failure (may be NULL).
 * @return TEEC_SUCCESS on success, or a TEEC_* error code on failure.
 */
TEEC_Result
teec_invoke_command(struct TEEC_Session *session,
                    uint32_t commandID,
                    struct TEEC_Operation *operation,
                    uint32_t *returnOrigin);

/**
 * Register a block of existing memory as shared memory.
 *
 * This is a wrapper around TEEC_RegisterSharedMemory().
 *
 * @param context Initialized context.
 * @param sharedMem Shared memory descriptor to register.
 * @return TEEC_SUCCESS on success, or a TEEC_* error code on failure.
 */
TEEC_Result
teec_register_shared_memory(struct TEEC_Context *context,
                            struct TEEC_SharedMemory *sharedMem);

/**
 * Register shared memory backed by a file descriptor.
 *
 * This is a wrapper around TEEC_RegisterSharedMemoryFileDescriptor().
 *
 * @param context Initialized context.
 * @param sharedMem Shared memory descriptor to register.
 * @param fd File descriptor backing the shared memory.
 * @return TEEC_SUCCESS on success, or a TEEC_* error code on failure.
 */
TEEC_Result
teec_register_shared_memory_file_descriptor(TEEC_Context *context,
                                            TEEC_SharedMemory *sharedMem,
                                            int fd);

/**
 * Allocate a shared memory block for use with the TEE.
 *
 * This is a wrapper around TEEC_AllocateSharedMemory().
 *
 * @param context Initialized context.
 * @param sharedMem Shared memory descriptor to allocate.
 * @return TEEC_SUCCESS on success, or a TEEC_* error code on failure.
 */
TEEC_Result
teec_allocate_shared_memory(struct TEEC_Context *context,
                            struct TEEC_SharedMemory *sharedMem);

/**
 * Release (free or deregister) a shared memory block.
 *
 * This is a wrapper around TEEC_ReleaseSharedMemory().
 *
 * @param sharedMemory Shared memory descriptor to release.
 * @return TRUE on success, FALSE if the TEEC backend isn't loaded.
 */
gboolean
teec_release_shared_memory(struct TEEC_SharedMemory *sharedMemory);

/**
 * Request cancellation of a pending open session or command invocation.
 *
 * This is a wrapper around TEEC_RequestCancellation().
 *
 * @param operation Operation previously passed to open session or invoke.
 * @return TRUE on success, FALSE if the TEEC backend isn't loaded.
 */
gboolean
teec_request_cancellation(struct TEEC_Operation *operation);

#endif /* TEEC_H */
