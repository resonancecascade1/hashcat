use std::os::raw::{c_char, c_int, c_void};

#[repr (C)]
pub struct generic_global_ctx_t
{
  pub workc: i32,
  pub workv: *mut *mut c_char,

  pub profile_dir: *mut c_char,
  pub cache_dir: *mut c_char,

  pub error: bool,
  pub error_msg: [c_char; 256],

  pub gbldata: *mut c_void,
}

#[repr (C)]
pub struct generic_thread_ctx_t
{
  pub thrdata: *mut c_void,
}

#[unsafe(no_mangle)]
pub extern "C" fn global_init (_global_ctx: *mut generic_global_ctx_t, _thread_ctx: *mut generic_thread_ctx_t) -> bool
{
  true
}

#[unsafe(no_mangle)]
pub extern "C" fn global_term (_global_ctx: *mut generic_global_ctx_t, _thread_ctx: *mut generic_thread_ctx_t)
{
}

#[unsafe(no_mangle)]
pub extern "C" fn global_keyspace (_global_ctx: *mut generic_global_ctx_t, _thread_ctx: *mut generic_thread_ctx_t) -> u64
{
  0xffffffffffffffff
}

#[unsafe(no_mangle)]
pub extern "C" fn thread_init (_global_ctx: *mut generic_global_ctx_t, _thread_ctx: *mut generic_thread_ctx_t) -> bool
{
  true
}

#[unsafe(no_mangle)]
pub extern "C" fn thread_term (_global_ctx: *mut generic_global_ctx_t, _thread_ctx: *mut generic_thread_ctx_t)
{
}

#[unsafe(no_mangle)]
pub extern "C" fn thread_seek (_global_ctx: *mut generic_global_ctx_t, _thread_ctx: *mut generic_thread_ctx_t, _offset: u64) -> bool
{
  true
}

#[unsafe(no_mangle)]
pub extern "C" fn thread_next (_global_ctx: *mut generic_global_ctx_t, _thread_ctx: *mut generic_thread_ctx_t, _out_buf: *mut u8) -> c_int
{
  9
}
