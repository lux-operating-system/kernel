/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <platform/platform.h>

#define PLATFORM

#ifdef PLATFORM_X86_64
#undef PLATFORM
#define PLATFORM            "x86_64"
#endif

#ifdef PLATFORM_ARM64
#undef PLATFORM
#define PLATFORM            "arm64"
#endif

#ifndef RELEASE_VERSION
#define KERNEL_VERSION      "lux microkernel " PLATFORM " (built " __DATE__ ")"
#else
#define KERNEL_VERSION      "lux microkernel " RELEASE_VERSION " " PLATFORM " (built " __DATE__ ")"
#endif
