# libs/sort — Timsort Implementation: Project Context

## Project Identity

**Goal:** Add `boost::sort::timsort` — a header-only, C++11, stdlib-only adaptive stable sort — to the existing `boost::sort` library.

**Submodule:** `git@github.com:ilyasorokindev/sort.git`, branch `develop`  
**Superproject planning artifacts:** `_bmad-output/` (PRD, Architecture, Epics, Stories, Deferred Work)

**Key files created/modified:**
- `libs/sort/include/boost/sort/timsort/timsort.hpp` — core algorithm (CREATE)
- `libs/sort/include/boost/sort/sort.hpp` — cumulative header (MODIFY: +1 include line)
- `libs/sort/test/test_timsort.cpp` — full test suite (CREATE)
- `libs/sort/test/CMakeLists.txt` + `libs/sort/test/Jamfile.v2` — build registration (MODIFY)
- `libs/sort/benchmark/single/benchmark_numbers.cpp` — int benchmarks (MODIFY)
- `libs/sort/benchmark/single/benchmark_strings.cpp` — string benchmarks (MODIFY)

---

## Story / Epic Status

| ID  | Title                                                        | Status            |
|-----|--------------------------------------------------------------|-------------------|
| 1.1 | Header Skeleton, Foundational Types, `compute_minrun`        | ✅ DONE           |
| 1.2 | Run Detection — `count_run` + `binary_insertion_sort`        | ✅ DONE           |
| 1.3 | Merge Engine — Galloping and Merge Functions                 | ✅ DONE           |
| 1.4 | Public API Overloads and Library Integration                 | ✅ DONE           |
| 2.1 | Correctness and Stability Tests                              | ✅ DONE           |
| 2.2 | Edge-Case, Type Coverage, and Adversarial Tests              | ✅ DONE           |
| 3.1 | Extend `benchmark_numbers.cpp` with Timsort Column          | ✅ DONE           |
| 3.2 | Extend `benchmark_strings.cpp` with Timsort Column          | ✅ DONE           |
| 4.1 | README Algorithm Table Entry                                 | ⏳ NOT YET CREATED |
| 4.2 | Standalone Usage Example                                     | ⏳ NOT YET CREATED |

**Epic 4 artifact files** do not exist yet and must be created before development starts.

---

## Architecture Decisions (agents must obey these)

Full source: `_bmad-output/planning-artifacts/architecture.md`

### P-Rules (Implementation Patterns)

| Rule | Constraint |
|------|-----------|
| **P1** | Exact function names: `compute_minrun`, `count_run`, `binary_insertion_sort`, `gallop_left`, `gallop_right`, `merge_lo`, `merge_hi`, `merge_collapse`, `merge_force_collapse` |
| **P2** | Namespace: internals in `boost::sort::tim_detail`, public API in `boost::sort` |
| **P3** | Gallop tie-breaking asymmetry (stability-critical): `gallop_left` uses `comp(*(base+mid), key)`; `gallop_right` uses `comp(key, *(base+mid))` — **do not swap these** |
| **P4** | `typedef` in function bodies (not `using`); C++11 compat |
| **P5** | Merge direction: `if (len1 <= len2) merge_lo else merge_hi` — use `<=` not `<` |
| **P6** | Test function order: `test_correctness`, `test_stability`, `test_edge_cases`, `test_type_coverage`, `test_adversarial` (called in this order from `test_main`) |
| **P7** | Benchmark column format: timsort is `[1]`, spinsort `[2]`, flat_stable_sort `[3]`, std::stable_sort `[4]`; nearly-sorted comment exact wording required |

### Key Design Decisions

- **D1.1:** `SortState<V>` struct holds `std::vector<V> buffer` + `int min_gallop` (initialized to 7)
- **D1.3:** Two-invariant run stack (`len[n-1] > len[n]` AND `len[n-2] > len[n-1] + len[n]`)
- **D2.1:** `static_assert` message: `"boost::sort::timsort requires RandomAccessIterator"` (exact wording)
- **D2.2:** Default comparator: `std::less<value_type>()` (NOT `std::less<>()` — C++14 only)
- **D3.2:** stdlib-only: 6 headers alphabetically (`<algorithm>`, `<cstddef>`, `<functional>`, `<iterator>`, `<utility>`, `<vector>`)
- **D3.3:** Iterator traits via aliases only: `iter_value_t<Iter>`, `iter_diff_t<Iter>` — never `Iter::value_type`
- **D3.4:** `std::move_if_noexcept` for all element copies in merge buffers

---

## Critical Implementation Notes

### Stability Bug (Story 2.1) — WATCH OUT
`merge_hi`'s gallop phase originally had `gallop_left` and `gallop_right` **swapped**. `test_stability()` caught this. Rule: whenever touching any merge function, verify P3 is still correct in **both** `merge_lo` and `merge_hi`.

### SM-1 Performance Target — NOT MET
Target: timsort ≥2× faster than spinsort on nearly-sorted (N=1M int).  
Observed: timsort=0.36s vs spinsort=0.19s.  
A required comment was added to `benchmark_numbers.cpp`. Tuning opportunity in a future epic.

### Build System — No Wildcards
`test/CMakeLists.txt` and `test/Jamfile.v2` require **explicit** per-test registration. No glob patterns. Always add new test files by name.

---

## Deferred Work (non-blocking, tracked)

Full source: `_bmad-output/implementation-artifacts/deferred-work.md`

**High-priority for future epics:**
- `merge_hi` gallop pressure: all adversarial tests use equal/right-heavier runs; `merge_hi` under adversarial gallop is untested
- Stability for equal-key groups: `test_type_coverage` verifies sort order only, not relative order preservation
- `merge_collapse` merge-order variant: known Stijn de Gouw issue (matches Python reference); architect to evaluate Java-style fix

**Low-priority / known limitations:**
- Gallop activation not instrumentally confirmed (structural guarantee + ASAN/UBSAN clean)
- `min_gallop` persistence across multiple merges not asserted
- Benchmark: single-shot timing, cold-cache timsort (pre-existing pattern), signed/unsigned modulo bias
- Signed overflow in `ofs = (ofs<<1)|1` for >2^30 elements (matches CPython; impossible in practice)
- `run_stack` not pre-reserved (Python uses fixed 85-entry; current O(log n) realloc is fine)

---

## What's Next — Epic 4

### Story 4.1 — README Algorithm Table Entry
- **File:** `libs/sort/README.md`
- Add row to single-thread algorithm table: `timsort | yes | N/2 | N | N log N | N log N | Comparison operator`
- Preserve all existing rows

### Story 4.2 — Standalone Usage Example
- **File:** `libs/sort/example/timsort_example.cpp` (CREATE)
- Demonstrate both overloads (default + custom comparator)
- Must compile with `-std=c++11 -Wall -Wextra -Wpedantic` → zero warnings
- Print sorted output to stdout

Before starting either story, create the artifact file (e.g. `_bmad-output/implementation-artifacts/4-1-*.md`) via `bmad-create-story`.

---

## Build & Test Quick-Reference

```bash
# CMake from superproject root
cmake -B build -DBOOST_INCLUDE_LIBRARIES=sort -DBUILD_TESTING=ON -G Ninja
cmake --build build
ctest --test-dir build -R timsort

# b2 from superproject root
b2 libs/sort/test//timsort

# With sanitizers
cmake -B build-san \
  -DBOOST_INCLUDE_LIBRARIES=sort -DBUILD_TESTING=ON -G Ninja \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
cmake --build build-san
ctest --test-dir build-san -R timsort

# Run benchmarks (after building)
./build/libs/sort/benchmark/single/benchmark_numbers
./build/libs/sort/benchmark/single/benchmark_strings
```
