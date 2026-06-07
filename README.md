# adaptive-radix-tree

to build and test:
```
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

to run unit tests
```
make gtest
```

to test memory:
```
make valgrind
```

to run benchmarks with hardware performance counters
```
make run_benchmarks
```

The benchmark target uses a Zen 4-friendly default counter set:
`branch-instructions,branch-misses,cache-references,cache-misses`.
Those are the counters that have been reliable on this machine; adding `cycles`
and `instructions` at the same time can exceed the available PMU counter slots.