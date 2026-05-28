#include <x86intrin.h>
#include <time.h>
#include <unistd.h>
#include <iostream>
#include <stdint.h>

// Returns the conversion multiplier from TSC ticks to nanoseconds
inline double get_tsc_multiplier() {
    struct timespec start_time, end_time;
    
    // 1. Snapshot the exact Linux wall-clock time
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
    // 2. Snapshot the exact CPU hardware ticks
    uint64_t start_tsc = __rdtsc();
    
    // 3. Force the thread to sleep for 10,000 microseconds (10 milliseconds)
    // The CPU ticks will keep flying by in the background
    usleep(10000); 
    
    // 4. Snapshot both clocks again
    clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);
    uint64_t end_tsc = __rdtsc();
    
    // 5. Calculate elapsed nanoseconds using the wall clock
    uint64_t elapsed_ns = (end_time.tv_sec - start_time.tv_sec) * 1'000'000'000ULL + 
                          (end_time.tv_nsec - start_time.tv_nsec);
                          
    // 6. Calculate elapsed hardware ticks
    uint64_t elapsed_tsc = end_tsc - start_tsc;
    
    // 7. Calculate the exact multiplier
    double tsc_to_nanos_multiplier = (double)elapsed_ns / (double)elapsed_tsc; 
    
    std::cout << "[*] TSC Calibration Complete.\n";
    std::cout << "    -> Hardware Ticks per second: " 
              << (elapsed_tsc * 100) << " (" << ((elapsed_tsc * 100) / 1e9) << " GHz)\n";

    return tsc_to_nanos_multiplier;
}