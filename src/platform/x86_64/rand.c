/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <platform/platform.h>
#include <platform/apic.h>
#include <kernel/logger.h>

static uint64_t seed;

/* platformInitialSeed(): sets the initial seed for a random number generator
 * this is platform-dependent because the source of the seed can differ */

void platformInitialSeed() {
    seed = (uint64_t)lapicRead(LAPIC_TIMER_CURRENT);
    seed *= platformCountCPU();
    KDEBUG("initialized random number seed %d\n", seed);

    for(int i = 0; i < 8; i++) {
        KDEBUG("%d\n", platformRand());
    }
}

/* platformRand(): generates a random number
 * params: none
 * returns: 64-bit random number
 */

uint64_t platformRand() {
    seed *= (uint64_t)lapicRead(LAPIC_TIMER_CURRENT);

    uint8_t *bytes = (uint8_t *)&seed;
    for(int i = 0; i < 4; i++) {
        bytes[i] ^= bytes[7-i];
        bytes[7-i] ^= bytes[i+4];
    }

    return ~seed;
}