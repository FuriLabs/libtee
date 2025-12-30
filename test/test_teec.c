/*
 * Keymaster TEEC wrapper test for libee.so
 *
 * Build:
 *   gcc -Iinclude test_teec.c -L. -ltee \
 *     $(pkg-config --cflags --libs glib-2.0 gio-2.0) \
 *     -Wl,-rpath,'$ORIGIN' -o test_teec -lhybris-common
 *
 * Run:
 *   sudo ./test_teec
 *
 * This calls:
 *   - KM_GET_VERSION            (0x1c)
 *   - KM_GET_SUPPORTED_ALGORITHMS (0x24)
 *
 * This is just a simple test to try the transport.
 */

#include <glib.h>
#include <stdio.h>

#include "tee_client_api.h"
#include "teec.h"

/* Comes from BSP */
#define KEYMASTER_REQ_SHIFT 2
#define KM_GET_VERSION             (7  << KEYMASTER_REQ_SHIFT)  /* 0x1c */
#define KM_GET_SUPPORTED_ALGORITHMS (9 << KEYMASTER_REQ_SHIFT)  /* 0x24 */

#define KM_TA_UUID_STR "c09c9c5d-aa50-4b78-b0e4-6eda61556c3a"

#define KM_BUF_SIZE (1024 * 70)

static const char*
teec_result_str(uint32_t r)
{
    switch (r) {
    case TEEC_SUCCESS: return "TEEC_SUCCESS";
    case TEEC_ERROR_GENERIC: return "TEEC_ERROR_GENERIC";
    case TEEC_ERROR_ACCESS_DENIED: return "TEEC_ERROR_ACCESS_DENIED";
    case TEEC_ERROR_CANCEL: return "TEEC_ERROR_CANCEL";
    case TEEC_ERROR_ACCESS_CONFLICT: return "TEEC_ERROR_ACCESS_CONFLICT";
    case TEEC_ERROR_EXCESS_DATA: return "TEEC_ERROR_EXCESS_DATA";
    case TEEC_ERROR_BAD_FORMAT: return "TEEC_ERROR_BAD_FORMAT";
    case TEEC_ERROR_BAD_PARAMETERS: return "TEEC_ERROR_BAD_PARAMETERS";
    case TEEC_ERROR_BAD_STATE: return "TEEC_ERROR_BAD_STATE";
    case TEEC_ERROR_ITEM_NOT_FOUND: return "TEEC_ERROR_ITEM_NOT_FOUND";
    case TEEC_ERROR_NOT_IMPLEMENTED: return "TEEC_ERROR_NOT_IMPLEMENTED";
    case TEEC_ERROR_NOT_SUPPORTED: return "TEEC_ERROR_NOT_SUPPORTED";
    case TEEC_ERROR_NO_DATA: return "TEEC_ERROR_NO_DATA";
    case TEEC_ERROR_OUT_OF_MEMORY: return "TEEC_ERROR_OUT_OF_MEMORY";
    case TEEC_ERROR_BUSY: return "TEEC_ERROR_BUSY";
    case TEEC_ERROR_COMMUNICATION: return "TEEC_ERROR_COMMUNICATION";
    case TEEC_ERROR_SECURITY: return "TEEC_ERROR_SECURITY";
    case TEEC_ERROR_SHORT_BUFFER: return "TEEC_ERROR_SHORT_BUFFER";
    case TEEC_ERROR_EXTERNAL_CANCEL: return "TEEC_ERROR_EXTERNAL_CANCEL";
    case TEEC_ERROR_TARGET_DEAD: return "TEEC_ERROR_TARGET_DEAD";
    default: return "TEEC_<unknown>";
    }
}

static void
hexdump_first(const void *data, size_t len, size_t max)
{
    const uint8_t *p = (const uint8_t*)data;
    size_t n = (len < max) ? len : max;

    for (size_t i = 0; i < n; i++) {
        g_print("%02x", p[i]);
        if ((i + 1) % 16 == 0)
            g_print("\n");
        else
            g_print(" ");
    }
    if (n % 16 != 0)
        g_print("\n");
    if (len > max)
        g_print("\n... (%zu bytes total)\n", len);
}

static gboolean
parse_uuid(const char *s, struct TEEC_UUID *out)
{
    unsigned int tl = 0, tm = 0, thv = 0;
    unsigned int n0 = 0, n1 = 0, n2 = 0, n3 = 0, n4 = 0, n5 = 0, n6 = 0, n7 = 0;

    if (!s || !out)
        return FALSE;

    if (sscanf(s,
               "%8x-%4x-%4x-%2x%2x-%2x%2x%2x%2x%2x%2x",
               &tl, &tm, &thv,
               &n0, &n1,
               &n2, &n3, &n4, &n5, &n6, &n7) != 11) {
        return FALSE;
    }

    out->timeLow = (uint32_t)tl;
    out->timeMid = (uint16_t)tm;
    out->timeHiAndVersion = (uint16_t)thv;
    out->clockSeqAndNode[0] = (uint8_t)n0;
    out->clockSeqAndNode[1] = (uint8_t)n1;
    out->clockSeqAndNode[2] = (uint8_t)n2;
    out->clockSeqAndNode[3] = (uint8_t)n3;
    out->clockSeqAndNode[4] = (uint8_t)n4;
    out->clockSeqAndNode[5] = (uint8_t)n5;
    out->clockSeqAndNode[6] = (uint8_t)n6;
    out->clockSeqAndNode[7] = (uint8_t)n7;

    return TRUE;
}

static void
km_call_and_dump(const char *label,
                 TEEC_Session *sess,
                 TEEC_SharedMemory *in_shm,
                 TEEC_SharedMemory *out_shm,
                 uint32_t cmd)
{
    struct TEEC_Operation op;
    TEEC_Result r;

    memset(&op, 0, sizeof(op));
    op.started = 1;
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT,
                                     TEEC_MEMREF_PARTIAL_INPUT,
                                     TEEC_MEMREF_PARTIAL_OUTPUT,
                                     TEEC_NONE);

    op.params[0].value.a = 0;

    /* No request payload for these queries */
    in_shm->size = 0;
    op.params[1].memref.parent = in_shm;
    op.params[1].memref.offset = 0;
    op.params[1].memref.size = in_shm->size;

    out_shm->size = KM_BUF_SIZE;
    memset(out_shm->buffer, 0, KM_BUF_SIZE);
    op.params[2].memref.parent = out_shm;
    op.params[2].memref.offset = 0;
    op.params[2].memref.size = out_shm->size;

    g_print("\n%s (cmd=0x%08x)\n", label, cmd);

    r = teec_invoke_command(sess, cmd, &op, NULL);
    if (r != TEEC_SUCCESS) {
        g_warning("TEEC_InvokeCommand failed: 0x%08x (%s)", r, teec_result_str(r));
        return;
    }

    if (op.params[0].value.a != 0) {
        g_warning("Keymaster error (value.a) = %u (0x%08x)",
                  op.params[0].value.a, op.params[0].value.a);
    } else {
        g_print("Keymaster error (value.a) = 0\n");
    }

    g_print("response size = %u bytes\n", (unsigned)op.params[2].memref.size);
    g_print("response hexdump (first 256 bytes):\n");
    hexdump_first(out_shm->buffer, (size_t)op.params[2].memref.size, 256);
}

int
main(void)
{
    /* Taken from BSP, tested with microtrust only */
    const char *hostname = "bta_loader";

    if (!teec_init()) {
        g_warning("teec_init() failed");
        return 1;
    }

    g_print("teec_init() ok, teec_is_loaded=%s\n", teec_is_loaded() ? "true" : "false");

    struct TEEC_UUID ta_uuid;
    if (!parse_uuid(KM_TA_UUID_STR, &ta_uuid)) {
        g_warning("bad Keymaster UUID string: %s", KM_TA_UUID_STR);
        teec_deinit();
        return 1;
    }

    TEEC_Context ctx;
    TEEC_Session sess;
    TEEC_Result r;
    uint32_t origin = 0;

    memset(&ctx, 0, sizeof(ctx));
    memset(&sess, 0, sizeof(sess));

    r = teec_initialize_context(hostname, &ctx);
    if (r != TEEC_SUCCESS) {
        g_warning("teec_initialize_context(\"%s\") failed: 0x%08x (%s)",
                  hostname, r, teec_result_str(r));
        teec_deinit();
        return 1;
    }
    g_print("teec_initialize_context(\"%s\") ok\n", hostname);

    r = teec_open_session(&ctx, &sess, &ta_uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, &origin);
    if (r != TEEC_SUCCESS) {
        g_warning("teec_open_session: failed: 0x%08x (%s), origin=0x%08x",
                  r, teec_result_str(r), origin);
        teec_finalize_context(&ctx);
        teec_deinit();
        return 1;
    }
    g_print("teec_open_session ok\n");

    /* Most of the sequence is translated from beanpod KM in BSP */
    TEEC_SharedMemory inputSM;
    TEEC_SharedMemory outputSM;

    memset(&inputSM, 0, sizeof(inputSM));
    memset(&outputSM, 0, sizeof(outputSM));

    inputSM.size = KM_BUF_SIZE;
    inputSM.flags = TEEC_MEM_INPUT;
    r = teec_allocate_shared_memory(&ctx, &inputSM);
    if (r != TEEC_SUCCESS || !inputSM.buffer) {
        g_warning("teec_allocate_shared_memory input failed: 0x%08x (%s)", r, teec_result_str(r));
        teec_close_session(&sess);
        teec_finalize_context(&ctx);
        teec_deinit();
        return 1;
    }

    outputSM.size = KM_BUF_SIZE;
    outputSM.flags = TEEC_MEM_OUTPUT;
    r = teec_allocate_shared_memory(&ctx, &outputSM);
    if (r != TEEC_SUCCESS || !outputSM.buffer) {
        g_warning("teec_allocate_shared_memory output failed: 0x%08x (%s)", r, teec_result_str(r));
        teec_release_shared_memory(&inputSM);
        teec_close_session(&sess);
        teec_finalize_context(&ctx);
        teec_deinit();
        return 1;
    }

    km_call_and_dump("KM_GET_VERSION", &sess, &inputSM, &outputSM, KM_GET_VERSION);
    km_call_and_dump("KM_GET_SUPPORTED_ALGORITHMS", &sess, &inputSM, &outputSM, KM_GET_SUPPORTED_ALGORITHMS);

    teec_release_shared_memory(&outputSM);
    teec_release_shared_memory(&inputSM);
    teec_close_session(&sess);
    teec_finalize_context(&ctx);
    teec_deinit();

    g_print("\ndone\n");
    return 0;
}
