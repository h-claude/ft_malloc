# ft_malloc — Debug System

The debug system provides a live terminal counter that updates in-place (like an FPS counter), showing real-time memory usage statistics for the allocator.

```
[malloc] mem:24.3KB  live:53  alloc:142  free:89
```

The counter displays:
- **mem** — total memory currently held by live blocks
- **live** — number of currently allocated blocks
- **alloc** — total number of `malloc` / `realloc` calls since the process started
- **free** — total number of `free` / `realloc`-triggered frees since the process started

The line is rewritten in-place using ANSI escape codes (`\033[2K\r`) so it never scrolls the terminal. The entire line is built in a buffer and emitted with a **single `write()`** call to avoid flickering from interleaved output.

---

## Building with the debug system

The debug code lives in `debug/debug.c` and is **excluded from the standard build**. To include it:

```sh
make re DEBUG=1
```

This does two things:
1. Adds `debug/debug.c` to the compilation
2. Defines the `MALLOC_DEBUG_BUILD` preprocessor flag, which enables the debug fields inside `g_data` and the counter-increment calls in `malloc.c` / `free.c`

A normal `make` (without `DEBUG=1`) produces a binary with zero debug code — no extra fields, no threads, no overhead.

---

## Activating the counter at runtime

Compiling with `DEBUG=1` only *enables* the feature. To actually *display* the counter, set the environment variable `MALLOC_DEBUG=1` when running your program:

```sh
MALLOC_DEBUG=1 ./a.out
```

Without `MALLOC_DEBUG=1`, the debug thread is never created even if the library was built with `DEBUG=1`. This lets you ship a debug-capable binary and turn the output on/off without recompiling.

---

## Usage modes

### 1. Direct linking (recommended, works on macOS and Linux)

Compile your program against `libft_malloc.so` directly using its **absolute path**. This fully replaces the system allocator for that binary.

```sh
# Build the library (with debug support)
make re DEBUG=1

# Compile your program against ft_malloc (use absolute path)
gcc -o a.out test.c /absolute/path/to/libft_malloc.so -lpthread

# Run with the counter
MALLOC_DEBUG=1 ./a.out
```

> **Important (macOS):** always pass the absolute path to the `.so` at link time, not `-L/-lft_malloc`. The `.so` embeds its own install name as an absolute path so dyld knows where to find it at runtime. Using `-L/-l` with a relative path causes dyld to record only the bare filename and fail to load.

All `malloc` / `free` / `realloc` calls from your program go through ft_malloc, so the counters are accurate.

### 2. Library injection (Linux only — LD_PRELOAD)

On Linux you can inject ft_malloc into any existing binary without recompiling it:

```sh
MALLOC_DEBUG=1 LD_PRELOAD=/absolute/path/to/libft_malloc.so ./some_program
```

This intercepts all allocator calls at the dynamic linker level. The counters will reflect the target program's full memory usage.

### 3. Library injection (macOS — DYLD_INSERT_LIBRARIES)

```sh
MALLOC_DEBUG=1 \
DYLD_INSERT_LIBRARIES=/absolute/path/to/libft_malloc.so \
DYLD_FORCE_FLAT_NAMESPACE=1 \
./some_program
```

> **macOS limitation:** Starting with macOS 12 (Monterey), `libSystem.B.dylib` — which provides the system `malloc` — is protected by the dynamic linker and **cannot be interposed** via `DYLD_INSERT_LIBRARIES`. The library loads successfully and the debug thread starts, but all allocations still go through the system allocator. The counters will show **alloc:0 free:0**. This is an OS-level restriction with no workaround short of recompiling the target against ft_malloc directly.

---

## How it works internally

### Source layout

```
debug/
  debug.c       — counter thread, display loop, env check
  README.md     — this file
srcs/
  malloc.c      — increments dbg_malloc_calls, dbg_live_mem (guarded by #ifdef MALLOC_DEBUG_BUILD)
  free.c        — increments dbg_free_calls,   dbg_live_mem (guarded by #ifdef MALLOC_DEBUG_BUILD)
includes/
  ft_malloc.h   — _Atomic debug fields in t_data, start_debug_thread() declaration
                  (all guarded by #ifdef MALLOC_DEBUG_BUILD)
```

### Thread lifecycle

When the library is loaded, `__attribute__((constructor))` causes `debug_auto_init()` to run automatically before `main()`. It calls `start_debug_thread()`, which:

1. Checks for `MALLOC_DEBUG=1` in the environment (using `_NSGetEnviron()` on macOS, which works correctly inside injected dylibs — plain `extern char **environ` does not)
2. If set, spawns a detached `pthread` running `debug_loop()`

`debug_loop()` wakes every **33 ms** (~30 fps), reads the counters, then writes the formatted line to stderr via a single `write(2, ...)` call.

### Lock-free counter reads

The three counters (`dbg_malloc_calls`, `dbg_free_calls`, `dbg_live_mem`) are declared `_Atomic size_t`. The display thread reads them with `atomic_load_explicit(..., memory_order_relaxed)` — no mutex, no blocking, a single CPU instruction per read.

The writes in `malloc` / `free` happen under `g_mutex` (required for allocator correctness), so the counters are always consistent individually. The display thread may read them at slightly different instants, meaning the four displayed values are not guaranteed to be a perfectly coherent snapshot — but any inconsistency self-corrects at the next refresh 33 ms later.

`live_blocks` is still read under `g_mutex` because it is derived from plain `int` fields in `g_data` that are not atomic.

### `dbg_live_mem` — incremental tracking

`dbg_live_mem` is maintained incrementally: `malloc` adds the allocated size, `free` subtracts the original allocation size (captured before any defragmentation merge). This avoids an O(n) traversal of the big-blocks list on every display tick, which would hold `g_mutex` and slow down the allocator on programs with many large allocations.

Arena blocks count the bucket size minus the header (e.g., 16 B for bucket 0). Big blocks count the exact size passed to `mmap`. Both are approximations of the user-requested size since the exact requested size is not stored in the block header.

### Overhead

- **Without `MALLOC_DEBUG=1`**: one env scan at lib load, then nothing
- **With `MALLOC_DEBUG=1`**: one atomic increment per `malloc`/`free` call + a brief mutex acquisition every 33 ms to read `live_blocks` — negligible in practice
