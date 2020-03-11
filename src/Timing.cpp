#include "Timing.hpp"

#include <thread>
#include <chrono>

void TSC_MarkStart (uint64_t &ret) // Takes ~220 cycles
{
    __asm__ __volatile__ ("cpuid\n\t"
                          "rdtsc\n\t"
                          "salq $32, %%rdx\n\t"
                          "addq %%rdx, %%rax\n\t"
                          : "=a" (ret) : : "%rbx", "%rcx", "%rdx" );
}
                  

void TSC_MarkStop (uint64_t &ret) // Takes ~250 cycles
{
    __asm__ __volatile__ ("rdtscp\n\t"
                          "salq $32, %%rdx\n\t"
                          "addq %%rdx, %%rax\n\t"
                          "movq %%rax, %0\n\t"
                          "cpuid\n\t" : "=r" (ret) : : "%rax", "%rbx", "%rcx", "%rdx" );
}

void TSC_MarkMid (uint64_t &t0, uint64_t &t1)
{
    __asm__ __volatile__ ("rdtscp\n\t"
                          "salq $32, %%rdx\n\t"
                          "addq %%rdx, %%rax\n\t"
                          "movq %%rax, %0\n\t"
                          "cpuid\n\t"
                          "rdtsc\n\t"
                          "salq $32, %%rdx\n\t"
                          "addq %%rdx, %%rax\n\t"
                          "movq %%rax, %1\n\t"
                          : "=r" (t0), "=r" (t1) : : "%rax", "%rbx", "%rcx", "%rdx" );
}

uint64_t MicrosecondsSinceEpoch (void)
{
    using Clock = std::chrono::high_resolution_clock;
    using Resolution = std::chrono::microseconds;
    return (std::chrono::duration_cast<Resolution> ((std::chrono::time_point_cast<Resolution> (Clock::now ())).time_since_epoch ())).count ();
}

uint64_t wait_until (uint64_t when)
{
    uint64_t now = MicrosecondsSinceEpoch ();
    while (when > (now + 2000))
    {
	std::this_thread::sleep_for (std::chrono::microseconds (when - (now + 2000)));
	now = MicrosecondsSinceEpoch ();
    }

    while (when > now)
    {
	now = MicrosecondsSinceEpoch ();
    }
    return now;
}
