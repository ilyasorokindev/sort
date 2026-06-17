# Boost.Sort — Agent Context

Boost.Sort is a header-only C++ library providing modern, fast, and memory-efficient sorting
algorithms — both single-threaded and parallel. It is part of the Boost C++ Libraries.

## Repository layout

```
include/boost/sort/       # public headers (all algorithms live here)
  sort.hpp                # single include: pulls in all algorithms
  spreadsort/             # hybrid radix sort (integer_sort, float_sort, string_sort)
  pdqsort/                # pattern-defeating quicksort
  spinsort/               # stable comparison sort
  flat_stable_sort/       # stable sort with ~1% extra memory
  block_indirect_sort/    # parallel unstable sort (low memory)
  sample_sort/            # parallel stable sort (samplesort)
  parallel_stable_sort/   # parallel stable sort (N/2 memory)
  insert_sort/            # insertion sort (used internally)
  common/                 # shared utilities (merge, scheduler, spinlock, etc.)
test/                     # unit tests (one .cpp per algorithm)
example/                  # usage examples for each algorithm
doc/                      # QuickBook documentation sources (.qbk)
benchmark/                # benchmark programs
CMakeLists.txt            # standalone or superproject CMake entry point
build.jam                 # Boost.Build (b2) entry point
```

## Environment

- **Language:** C++11 minimum (C++17 recommended); parallel algorithms require C++11
- **Compiler:** Apple clang (this machine), also tested with GCC
- **Build system:** CMake (preferred) or Boost.Build (b2)
- **Platform:** macOS (darwin)
- **Dependencies:** header-only; no linking required except Boost.Test for tests

## Algorithms

### Single-thread

| Algorithm        | Stable | Extra memory               | Complexity                         | Method           |
|------------------|--------|----------------------------|------------------------------------|------------------|
| spreadsort       | no     | key_length                 | N, N√(logN), min(NlogN, N·keylen)  | Hybrid radix     |
| pdqsort          | no     | O(log N)                   | N, NlogN, NlogN                    | Comparison       |
| spinsort         | yes    | N/2                        | N, NlogN, NlogN                    | Comparison       |
| flat_stable_sort | yes    | data_size/256 + 8 KB       | N, NlogN, NlogN                    | Comparison       |

- **spreadsort** — extremely fast hybrid radix sort for integers, floats, and strings (Steven Ross).
- **pdqsort** — pattern-defeating quicksort; drop-in replacement for `std::sort` (Orson Peters).
- **spinsort** — best for nearly-sorted data; N/2 additional memory (Francisco Tapia).
- **flat_stable_sort** — stable sort with ~1% extra memory; ~80–90% speed of spinsort (Francisco Tapia).

### Parallel (require C++11, use `std::thread` internally)

| Algorithm            | Stable | Extra memory            | Complexity        |
|----------------------|--------|-------------------------|-------------------|
| block_indirect_sort  | no     | block_size × num_threads| N, NlogN, NlogN   |
| sample_sort          | yes    | N                       | N, NlogN, NlogN   |
| parallel_stable_sort | yes    | N/2                     | N, NlogN, NlogN   |

All parallel algorithms accept an optional final `num_threads` argument; default is
`std::thread::hardware_concurrency()`. Passing 0 or 1 runs single-threaded.

`block_size` scales automatically with object size:

| Object size (bytes) | 1–15 | 16–31 | 32–63 | 64–127 | 128–255 | 256–511 | 512+ |
|---------------------|------|-------|-------|--------|---------|---------|------|
| block_size (elems)  | 4096 | 2048  | 1024  | 768    | 512     | 256     | 128  |

Strings always use block_size 128.

## Build (CMake via Boost superproject)

This library is header-only; there is nothing to compile except tests.
The sort CMakeLists.txt uses `find_package(Boost CONFIG)`, which requires the Boost superproject
(located at `../Boost` relative to this repo on this machine).

**Configure and build tests:**

```sh
cd /Users/ilyasorokin/Work/Temp/Boost/Boost

cmake -S . -B build/sort-full \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_CXX_STANDARD=17 \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON \
  -DBOOST_INCLUDE_LIBRARIES="sort;config;core;range;type_traits;test"

cmake --build build/sort-full --target tests -j$(sysctl -n hw.logicalcpu)
```

The test binaries are written to `build/sort-full/stage/bin/boost_sort_*`.

**Run tests:**

```sh
ctest --test-dir build/sort-full -R boost_sort --output-on-failure
```

All 11 tests should pass (total runtime ~100 s on a typical Mac due to parallel sort benchmarks).

## Build (Boost.Build / b2)

From the Boost superproject root:

```sh
./b2 toolset=clang cxxstd=17 libs/sort/test
```

## Testing

One test file per algorithm in `test/`:

| Test binary                        | Source file                        |
|------------------------------------|------------------------------------|
| boost_sort_float_sort_test         | float_sort_test.cpp                |
| boost_sort_integer_sort_test       | integer_sort_test.cpp              |
| boost_sort_sort_detail_test        | sort_detail_test.cpp               |
| boost_sort_string_sort_test        | string_sort_test.cpp               |
| boost_sort_test_block_indirect_sort| test_block_indirect_sort.cpp       |
| boost_sort_test_flat_stable_sort   | test_flat_stable_sort.cpp          |
| boost_sort_test_insert_sort        | test_insert_sort.cpp               |
| boost_sort_test_parallel_stable_sort| test_parallel_stable_sort.cpp     |
| boost_sort_test_pdqsort            | test_pdqsort.cpp                   |
| boost_sort_test_sample_sort        | test_sample_sort.cpp               |
| boost_sort_test_spinsort           | test_spinsort.cpp                  |

## Usage

```cpp
#include <boost/sort/sort.hpp>   // all algorithms

// Single-thread
boost::sort::pdqsort(v.begin(), v.end());
boost::sort::spreadsort::spreadsort(v.begin(), v.end());
boost::sort::spinsort(v.begin(), v.end());
boost::sort::flat_stable_sort(v.begin(), v.end());

// Parallel (last arg = thread count, default = hardware_concurrency())
boost::sort::block_indirect_sort(v.begin(), v.end());
boost::sort::sample_sort(v.begin(), v.end(), 4);
boost::sort::parallel_stable_sort(v.begin(), v.end(), comp, 8);
```

All parallel call signatures:
```cpp
algorithm(first, last, comp, num_threads)
algorithm(first, last, comp)
algorithm(first, last, num_threads)
algorithm(first, last)
```

## Code conventions

- All public headers under `include/boost/sort/`
- C++11 minimum; no Boost dependencies at runtime (header-only)
- Internal utilities in `include/boost/sort/common/`
- Namespace: `boost::sort` (spreadsort lives in `boost::sort::spreadsort`)
- Parallel algorithms are exception-safe: object integrity is preserved, but order is not
  guaranteed if an exception is thrown inside a move/copy constructor

## Authors

- **Steven Ross** — spreadsort
- **Orson Peters** — pdqsort
- **Francisco Tapia** — spinsort, flat_stable_sort, block_indirect_sort, sample_sort, parallel_stable_sort

Copyright 2014–2017. Distributed under the Boost Software License, Version 1.0.
