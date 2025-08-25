/**
 * Author......: See docs/credits.txt
 * License.....: MIT
 */

#ifndef HC_GENERIC_H
#define HC_GENERIC_H

#define HC_LOAD_FUNC_GENERIC(ptr, name, type)                                                \
do                                                                                           \
{                                                                                            \
  (ptr)->name = (type) hc_dlsym ((ptr)->lib, #name);                                         \
  if ((ptr)->name == NULL)                                                                   \
  {                                                                                          \
    event_log_error (hashcat_ctx, "%s is missing from %s shared library.", #name, (ptr)->dynlib_filename); \
    return -1;                                                                               \
  }                                                                                          \
} while (0)

bool generic_global_init     (hashcat_ctx_t *hashcat_ctx);
void generic_global_term     (hashcat_ctx_t *hashcat_ctx);
u64  generic_global_keyspace (hashcat_ctx_t *hashcat_ctx);
bool generic_thread_init     (hashcat_ctx_t *hashcat_ctx, const int backend_device_idx);
void generic_thread_term     (hashcat_ctx_t *hashcat_ctx, const int backend_device_idx);
int  generic_thread_next     (hashcat_ctx_t *hashcat_ctx, const int backend_device_idx, u8 *out_buf);
bool generic_thread_seek     (hashcat_ctx_t *hashcat_ctx, const int backend_device_idx, const u64 offset);

int  generic_ctx_init        (hashcat_ctx_t *hashcat_ctx);
void generic_ctx_destroy     (hashcat_ctx_t *hashcat_ctx);

#endif // HC_GENERIC_H
