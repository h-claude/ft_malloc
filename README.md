# ft_malloc

> A drop-in replacement for `malloc(3)`/`free(3)`/`realloc(3)`, implemented as a thread-safe shared library backed exclusively by `mmap(2)`/`munmap(2)`.

---

## 📌 Problem Overview & Classification

- **Domain / Project Category:** Low-Level Systems Programming / Custom Dynamic Memory Allocator (shared library)
- **Theoretical / Engineering Problem:** Memory Management — a segregated free-list (slab/pool) allocator with block splitting and adjacent-block coalescing, exposed as a binary-compatible replacement for the libc allocator (42 school "malloc" project)
- **Core Objective:** Provide `malloc`, `free`, `realloc` and a diagnostic `show_alloc_mem` entirely on top of `mmap`/`munmap` — with no dependency on the system allocator — so that existing dynamically-linked binaries can adopt it without modification or recompilation (via `LD_PRELOAD` / `DYLD_INSERT_LIBRARIES`), or link against it directly.
- **Key Constraints & Challenges:**
  - Exactly **two global variables**: one for allocation bookkeeping (`g_data`), one for thread-safety (`g_mutex`) — a hard limit imposed by the assignment.
  - Minimize `mmap()`/`munmap()` syscalls: each arena must pre-allocate room for at least 100 blocks of a given size class, with a size that is a multiple of the page size.
  - `malloc()` must return properly aligned memory.
  - Thread-safety under concurrent `malloc`/`free`/`realloc` calls from multiple `pthread`s.
  - No invalid pointer, double-free, or integer-overflow input may crash or corrupt allocator state.

---

## ⚙️ Architecture & Technical Design

- **Component / Modular Design:**
  - [`srcs/malloc.c`](srcs/malloc.c) — arena/zone bootstrapping (`init_zone`), bump allocation (`carve_bump`), free-list reuse and block splitting (`get_more_blocks`), and the public `malloc`/`malloc_unlocked`.
  - [`srcs/free.c`](srcs/free.c) — pointer-ownership validation, list removal, adjacent-block coalescing (`try_merge`), and `munmap`-based release of large blocks.
  - [`srcs/realloc.c`](srcs/realloc.c) — exact-header validation of the incoming pointer (`is_valid_block`), in-place reuse when the existing block is already large enough, otherwise allocate/copy/free via the internal unlocked API.
  - [`srcs/show_alloc_mem.c`](srcs/show_alloc_mem.c) — allocator-state dump sorted by increasing address (zones, blocks and large blocks alike; the lists are unsorted, so each step selects the next-lowest address instead of allocating a sort buffer), written entirely with `write(2)` and hand-rolled integer/hex formatting (no `stdio`).
  - [`srcs/utils.c`](srcs/utils.c) — size-class lookup, linked-list removal helpers, and pointer-membership tests.
  - [`includes/ft_malloc.h`](includes/ft_malloc.h) — the single shared state struct `t_data`, block header `t_block` (16 bytes: `next` pointer + tagged `size_t size`, with bit 0 as a free/allocated flag for TINY/SMALL blocks), and per-size-class bookkeeping structs.
- **Core Primitives & Mechanisms:**
  - `Size classes (segregated free lists)`: six classes with total block sizes `32 / 64 / 128 / 256 / 512 / 1040` bytes (16-byte header + usable payload of `16 / 48 / 112 / 240 / 496 / 1024` bytes). Reporting groups them as **TINY** (payload ≤ 112 B) and **SMALL** (240 B ≤ payload ≤ 1024 B); requests with payload above 1024 bytes bypass the arenas entirely and become **LARGE** blocks. Every class size is a multiple of 16, so returned pointers are 16-byte aligned by construction.
  - `Arenas (zone_bump / zone_end)`: each size class owns a bump-allocated region inside an `mmap`'d `t_arena`, sized via `pages = ceil((sizeof(t_arena) + 100 * block_size) / pagesize)` to guarantee at least 100 blocks per zone, per the assignment's mmap-minimization requirement. Zones are not pre-split: a block header is only written when `carve_bump` hands that block out, so pages beyond the bump pointer are never touched and never become resident (no page faults for memory that is reserved but unused).
  - `Allocation path`: pop from the class's free list → else carve from the bump pointer → else pull a block from the next larger class's free list and split it down to the target class (`get_more_blocks`) → else `mmap` a fresh arena for that class. Splitting produces two blocks sized `block_size_for_index(tmp_index - 1)`, which is an exact halving for every class except the top one: `1040` (class 5) donates to class 4 (`512`) as two `512`-byte blocks plus 16 wasted trailing bytes, since `1040` isn't `2 * 512`.
  - `Coalescing (try_merge)`: performed inline inside `free()` for classes under 512 bytes once a class holds more than `MIN_BLOCKS_TO_DEFRAG` (3) free blocks — it merges an address-adjacent, same-size, same-arena free neighbor back into a block of the next class up, undoing splits made by `get_more_blocks`. Class 4 (512) is capped out of this path on purpose: two adjacent freed 512-byte blocks are never merged back together, because the result (1024 bytes) doesn't match class 5's actual size (1040) — merging them would misrepresent a block's real capacity to any later allocation that reused it as "class 5". This is a one-way donation, not a reversible split/merge pair.
  - `Large blocks (alloc_big_block)`: requests with payload > 1024 bytes are individually `mmap`'d, tracked in a flat linked list (`t_big_blocks`), store their raw size in the header (they are never in a free state, so no `FLAG_FREE` bit is needed — odd sizes are preserved exactly), and `munmap`'d one-for-one on `free()`.
  - `mmap(2)` / `munmap(2)`: sole mechanism for acquiring and releasing memory from the OS — used both for per-size-class arenas and standalone large blocks; no `sbrk`/`brk` is used.
  - `getpagesize()`: every arena's `mmap` size is rounded up to a multiple of the system page size.
  - `pthread_mutex_lock` / `pthread_mutex_unlock`: a single global `PTHREAD_MUTEX_INITIALIZER` mutex serializes all access to `g_data`. `malloc()`/`free()`/`show_alloc_mem()` lock around their internals; `realloc()` locks once and calls the `_unlocked` variants directly to avoid self-deadlock on the non-recursive mutex.
  - `write(2)`: `show_alloc_mem()` and its helpers format all output with raw `write()` instead of `printf`, keeping the diagnostic path free of libc stdio buffering.
  - `-fvisibility=hidden`: only `malloc`, `free`, `realloc` and `show_alloc_mem` are exported (`PUBLIC` attribute); every internal helper stays private to the library.
- **Lifecycle & Error Handling:**
  - Every block (TINY/SMALL/LARGE) carries an inline 16-byte header; for TINY/SMALL blocks the free/allocated state is packed into bit 0 of `size` (`FLAG_FREE`), so header size and payload alignment never change. LARGE headers hold the raw size.
  - `Is_In_ArenA()` / `Is_In_BigBlocks()` first check that a pointer passed to `free()`/`realloc()` lies in allocator-owned memory; then `ptr - sizeof(t_block)` must be exactly a header present in the allocated list of its class (or in the large-block list). There is no backward search for a header: pointers into the middle of a block, already-freed blocks and foreign addresses are rejected (no-op / `NULL`) instead of corrupting state.
  - `free()` is idempotent against double-free: a block already carrying `FLAG_FREE` is detected via `IS_FREE(header->size)` and the call becomes a no-op.
  - `malloc()` rejects `size == 0` and sizes that would overflow the header addition (`size > SIZE_MAX - sizeof(t_block)`) by returning `NULL`; `realloc(ptr, 0)` frees `ptr` and returns `NULL`; a failed `realloc()` returns `NULL` and leaves the original block intact.
  - A failed `mmap` leaves `g_data` untouched: the allocation simply returns `NULL`.
  - Arenas are never individually `munmap`'d back to the OS (only large blocks are); this favors fewer `mmap`/`munmap` calls, consistent with the "≥100 allocations per zone" pre-allocation strategy. No signal handlers are installed — the library is passive, and the OS reclaims all `mmap`'d memory automatically at process exit.

---

## 🛠️ Tech Stack & Dependencies

| Category | Specification |
| :--- | :--- |
| **Language & Runtime** | C (GNU C via `gcc`, POSIX.1-2008 primitives), compiled with `-Wall -Wextra -Werror -fPIC -fvisibility=hidden -g3` |
| **Frameworks & Core Libraries** | None beyond the OS: `mmap(2)` / `munmap(2)`, `getpagesize()`, POSIX Threads (`pthread_mutex_t`), `write(2)` |
| **Build & Package Management** | GNU Make, producing a shared object (`-shared`) |
| **Diagnostics & Testing** | Valgrind (`memcheck`, Linux); macOS `leaks(1)` as the native equivalent |

---

## 🚀 Getting Started

### Prerequisites
- Linux or macOS (built and verified here on macOS/Darwin, `arm64`).
- `gcc` (or a compatible C compiler) supporting `-fPIC` and `-fvisibility=hidden`.
- POSIX threads (`pthread`) and a `mmap`/`munmap`-capable kernel.

### Installation & Setup
```bash
# Clone the repository
git clone git@github.com:h-claude/ft_malloc.git
cd ft_malloc

# Build libft_malloc_$(HOSTTYPE).so and the libft_malloc.so symlink
make

# Other targets
make clean    # remove object files (objs/*.o)
make fclean   # clean + objs/, the versioned .so and the symlink
make re       # fclean followed by all
```
This produces `libft_malloc_$(HOSTTYPE).so` (e.g. `libft_malloc_arm64_Darwin.so` on this machine, or `libft_malloc_x86_64_Linux.so` on x86-64 Linux) and a symbolic link `libft_malloc.so` pointing to it.

### Usage & Execution
The project builds a **shared library**, not a standalone executable. It is meant to be linked against, or interposed at runtime over an existing binary's libc allocator:

```bash
# Direct linking
gcc -Iincludes -o my_program my_program.c -L. -lft_malloc
DYLD_LIBRARY_PATH=. ./my_program   # macOS
LD_LIBRARY_PATH=.  ./my_program    # Linux

# Runtime interposition over an already-compiled binary, without recompiling it
DYLD_INSERT_LIBRARIES=./libft_malloc.so DYLD_FORCE_FLAT_NAMESPACE=1 ./some_binary   # macOS
LD_PRELOAD=./libft_malloc.so ./some_binary                                          # Linux
```

*Example / Quick Demonstration:*
```bash
cat > demo.c << 'EOF'
#include <stdlib.h>
#include <string.h>
void show_alloc_mem(void);

int main(void)
{
    char *a = malloc(20);
    char *b = malloc(200);
    char *c = malloc(2000);
    strcpy(a, "hello");
    b = realloc(b, 300);
    free(a);
    show_alloc_mem();
    free(b);
    free(c);
    return (0);
}
EOF
gcc -Iincludes -o demo demo.c -L. -lft_malloc
DYLD_LIBRARY_PATH=. ./demo
```
Output:
```
SMALL : 0x100588000
0x100588020 - 0x100588210 : 496 bytes
LARGE : 0x100584000
0x100584010 - 0x1005847E0 : 2000 bytes
Total : 2496 bytes
```

### Configuration / Environment Variables
| Variable | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `HOSTTYPE` | String | `` `uname -m`_`uname -s` `` | Suffix of the built library name (`libft_malloc_$HOSTTYPE.so`); read by the Makefile, defaulted when empty or unset. |
| `LD_PRELOAD` | Path | — | Linux: interposes the library over a binary's libc allocator. |
| `DYLD_INSERT_LIBRARIES` | Path | — | macOS: interposes the library; must be combined with `DYLD_FORCE_FLAT_NAMESPACE=1`. |
| `LD_LIBRARY_PATH` / `DYLD_LIBRARY_PATH` | Path | — | Lets a directly-linked program locate `libft_malloc.so` at runtime. |

---

## 🧪 Testing & Validation

No test sources are bundled in this repository. Functional and concurrency behavior was independently verified against this build using ad hoc programs linked with `-L. -lft_malloc`, exercising `malloc`/`realloc`/`free` across all three zones (TINY, SMALL, LARGE) and `show_alloc_mem`.

```bash
# Linux — verify zero memory leaks
valgrind --leak-check=full --show-leak-kinds=all ./my_program [sample_args]

# macOS — native equivalent (used here: 0 leaks, 0 bytes, across malloc/realloc/free/show_alloc_mem)
DYLD_LIBRARY_PATH=. leaks --atExit -- ./my_program [sample_args]

# Concurrency: 8 threads x 2000 malloc/realloc/free cycles each, all through the shared mutex
DYLD_LIBRARY_PATH=. ./thread_stress_test   # exits 0, no crash, no leaks
```

- [x] Zero memory leaks confirmed with macOS `leaks(1)` across a TINY/SMALL/LARGE allocation, realloc and free cycle (0 leaks, 0 bytes lost).
- [x] Boundary checks confirmed: `size == 0`, sizes that would overflow the header addition, `NULL`/foreign/already-freed/mid-block pointers passed to `free`/`realloc` are all rejected without crashing.
- [x] Odd-sized LARGE blocks keep their exact size: the last byte survives a `realloc` to a bigger size.
- [x] `show_alloc_mem()` output verified in increasing address order across TINY, SMALL and LARGE.
- [x] Thread-safety confirmed under contention: 8 concurrent threads performing 2000 malloc/realloc/free cycles each through the shared mutex, clean exit.
- [x] Real-world interposition: system binaries (e.g. `ls`) run to completion with the library injected.

---

## 👤 Author

- GitHub: [@h-claude](https://github.com/h-claude)
