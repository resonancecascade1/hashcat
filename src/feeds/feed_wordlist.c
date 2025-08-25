/**
 * Author......: See docs/credits.txt
 * License.....: MIT
 */

#include "common.h"
#include "types.h"
#include "memory.h"
#include "convert.h"
#include "filehandling.h"
#include "shared.h"
#include "feed_wordlist.h"

#if defined (_WIN)
#include "mmap_windows.c"
#else
#include <sys/mman.h>
#endif

static void error_set (generic_global_ctx_t *global_ctx, const char *fmt, ...)
{
  global_ctx->error = true;

  va_list ap;
  va_start (ap, fmt);

  vsnprintf (global_ctx->error_msg, sizeof (global_ctx->error_msg), fmt, ap);

  va_end (ap);
}

static size_t process_word (const u8 *buf, const size_t len, u8 *out_buf)
{
  size_t word_len = len;

  while ((word_len > 0) && (buf[word_len - 1] == '\r')) word_len--;

  size_t report_len = MIN (word_len, PW_MAX);

  if (report_len) memcpy (out_buf, buf, report_len);

  return report_len;
}

bool global_init (MAYBE_UNUSED generic_global_ctx_t *global_ctx, MAYBE_UNUSED generic_thread_ctx_t *thread_ctx)
{
  // create our own context

  feed_global_t *feed_global = hcmalloc (sizeof (feed_global_t));

  global_ctx->gbldata = feed_global;

  // check arguments

  if (global_ctx->workc < 2)
  {
    error_set (global_ctx, "Invalid parameter count: %d. Count must be at least 2.", global_ctx->workc);

    return false;
  }

  feed_global->wordlist = global_ctx->workv[1];

  return true;
}

void global_term (MAYBE_UNUSED generic_global_ctx_t *global_ctx, MAYBE_UNUSED generic_thread_ctx_t *thread_ctx)
{
  feed_global_t *feed_global = global_ctx->gbldata;

  hcfree (feed_global);

  global_ctx->gbldata = NULL;
}

u64 global_keyspace (MAYBE_UNUSED generic_global_ctx_t *global_ctx, MAYBE_UNUSED generic_thread_ctx_t *thread_ctx)
{
  thread_init (global_ctx, thread_ctx); // will occupy thread slot 0, fine for us temporary

  feed_thread_t *feed_thread = thread_ctx->thrdata;

  u64 lines = 0;

  const u8 *fd_mem = feed_thread->fd_mem;

  size_t fd_len = feed_thread->fd_len;

  while (fd_len)
  {
    const u8 *next = memchr (fd_mem, '\n', fd_len);

    if (next == NULL)
    {
      // this should be fine as meassurement to detect if there's a newline at the end of file or not,
      // because we limit ourself with fd_len and if there's a newline as last byte of the file, the while loop will break naturally

      lines++;

      break;
    }

    const size_t step_size = (size_t) (next - fd_mem) + 1;

    fd_mem += step_size;
    fd_len -= step_size;

    lines++;
  }

  thread_term (global_ctx, thread_ctx);

  return lines;
}

bool thread_init (MAYBE_UNUSED generic_global_ctx_t *global_ctx, MAYBE_UNUSED generic_thread_ctx_t *thread_ctx)
{
  feed_global_t *feed_global = global_ctx->gbldata;

  thread_ctx->thrdata = feed_global;

  // create our own context

  feed_thread_t *feed_thread = hcmalloc (sizeof (feed_thread_t));

  if (feed_thread == NULL)
  {
    error_set (global_ctx, "hcmalloc failed");

    return false;
  }

  thread_ctx->thrdata = feed_thread;

  // open wordlist

  if (hc_fopen_raw (&feed_thread->hcfile, feed_global->wordlist, "rb") == false)
  {
    error_set (global_ctx, "%s: %s", feed_global->wordlist, strerror (errno));

    return false;
  }

  // check size

  struct stat s;

  if (hc_fstat (&feed_thread->hcfile, &s) == -1)
  {
    error_set (global_ctx, "%s: %s", feed_global->wordlist, strerror (errno));

    return false;
  }

  if (s.st_size == 0)
  {
    error_set (global_ctx, "%s: zero size", feed_global->wordlist);

    return false;
  }

  feed_thread->fd_off = 0;
  feed_thread->fd_len = s.st_size;

  void *fd_mem = mmap (NULL, feed_thread->fd_len, PROT_READ, MAP_PRIVATE, feed_thread->hcfile.fd, 0);

  if (fd_mem == MAP_FAILED)
  {
    error_set (global_ctx, "%s: mmap failed", feed_global->wordlist);

    return false;
  }

  feed_thread->fd_mem = fd_mem;
  feed_thread->fd_line = 0;

  // kernel advice

  #if !defined (_WIN)
  #ifdef POSIX_MADV_SEQUENTIAL
  posix_madvise (feed_thread->fd_mem, feed_thread->fd_len, POSIX_MADV_SEQUENTIAL);
  #endif
  #ifdef POSIX_MADV_WILLNEED
  posix_madvise (feed_thread->fd_mem, feed_thread->fd_len, POSIX_MADV_WILLNEED);
  #endif
  #endif

  return true;
}

void thread_term (MAYBE_UNUSED generic_global_ctx_t *global_ctx, MAYBE_UNUSED generic_thread_ctx_t *thread_ctx)
{
  feed_thread_t *feed_thread = thread_ctx->thrdata;

  munmap (feed_thread->fd_mem, feed_thread->fd_len);

  hc_fclose (&feed_thread->hcfile);

  hcfree (feed_thread);

  thread_ctx->thrdata = NULL;
}

int thread_next (MAYBE_UNUSED generic_global_ctx_t *global_ctx, MAYBE_UNUSED generic_thread_ctx_t *thread_ctx, u8 *out_buf)
{
  feed_thread_t *feed_thread = thread_ctx->thrdata;

  const u8     *fd_mem = feed_thread->fd_mem;
  const size_t  fd_len = feed_thread->fd_len;
  const size_t  fd_off = feed_thread->fd_off;

  if (fd_off >= fd_len)
  {
    error_set (global_ctx, "next fd_off >= fd_len: %zu:%zu", fd_off, fd_len);

    return -1;
  }

  const u8 *next = memchr (fd_mem + fd_off, '\n', fd_len - fd_off);

  if (next == NULL) // if wordlist doesn't end with a newline
  {
    const size_t step_size = fd_len - fd_off;

    const size_t word_len = process_word (fd_mem + fd_off, step_size, out_buf);

    feed_thread->fd_off += step_size;

    feed_thread->fd_line++;

    return (int) word_len;
  }

  const size_t step_size = (size_t) ((next - fd_mem) - fd_off);

  const size_t word_len = process_word (fd_mem + fd_off, step_size, out_buf);

  feed_thread->fd_off += step_size + 1; // +1 = \n

  feed_thread->fd_line++;

  return (int) word_len;
}

bool thread_seek (MAYBE_UNUSED generic_global_ctx_t *global_ctx, MAYBE_UNUSED generic_thread_ctx_t *thread_ctx, const u64 offset)
{
  feed_thread_t *feed_thread = thread_ctx->thrdata;

  const u8     *fd_mem = feed_thread->fd_mem;
  const size_t  fd_len = feed_thread->fd_len;

  size_t  fd_off  = feed_thread->fd_off;
  u64     fd_line = feed_thread->fd_line;

  if (fd_off >= fd_len)
  {
    error_set (global_ctx, "seek fd_off >= fd_len: %zu:%zu", fd_off, fd_len);

    return false;
  }

  while (fd_off < fd_len)
  {
    if (fd_line == offset)
    {
      feed_thread->fd_off  = fd_off;
      feed_thread->fd_line = fd_line;

      return true;
    }

    const u8 *next = memchr (fd_mem + fd_off, '\n', fd_len - fd_off);

    if (next == NULL) // if wordlist doesn't end with a newline
    {
      const size_t step_size = fd_len - fd_off;

      fd_off += step_size;

      fd_line++;

      continue;
    }

    const size_t step_size = (size_t) ((next - fd_mem) - fd_off);

    fd_off += step_size + 1; // +1 = \n

    fd_line++;
  }

  return false;
}

