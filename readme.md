**lux** (intentionally stylized in lowercase) is a portable work-in-progress microkernel written from scratch that currently runs on x86_64, with future plans for an ARM64 port.

[![License: MIT](https://img.shields.io/github/license/lux-operating-system/kernel?color=red)](https://github.com/lux-operating-system/kernel/blob/main/LICENSE) [![GitHub commit activity](https://img.shields.io/github/commit-activity/w/lux-operating-system/kernel)](https://github.com/lux-operating-system/kernel/commits/main/) [![Build Status](https://github.com/lux-operating-system/kernel/actions/workflows/build-mac.yml/badge.svg)](https://github.com/lux-operating-system/kernel/actions) [![GitHub Issues](https://img.shields.io/github/issues/lux-operating-system/kernel)](https://github.com/lux-operating-system/kernel/issues) [![Codacy Badge](https://app.codacy.com/project/badge/Grade/01007e2804e34b4da5d164ea927443a5)](https://app.codacy.com/gh/lux-operating-system/kernel/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)

![Screenshot of luxOS running on QEMU](https://jewelcodes.io/lux.png)

## Overview

In little over 4,000 lines <sup>[1]</sup> of code, lux implements **memory management**, preemptive **multiprocessor priority scheduling**, **interprocess communication**, and basic **Unix-like system calls**. This elimination of bloat minimizes resource consumption compared to mainstream operating systems and increases stability and memory protection. lux is developed primarily as a one-person project, both as a learning and research tool as well as a criticism of the bloat that has become normalized in modern software engineering.

## Features

- [x] **Portability:** At the heart of lux is a platform abstraction layer with a set of functions and constants to be implemented and defined for each platform. This enables ease of porting lux to other CPU architectures.
- [x] **Memory management:** lux implements a future-proof memory manager that can manage up to 128 TiB of physical memory and virtual address spaces of up to 256 TiB divided into 128 TiB for each of the kernel and user threads.
- [x] **Multiprocessor priority scheduling:** The scheduler of lux was designed with multiprocessor support from the start, and lux currently supports CPUs with up to 256 cores on x86_64 platforms. The microkernel itself is also multithreaded and can be preempted.
- [x] **Interprocess communication:** lux implements kernel-level support for local Unix domain sockets to facilitate communication with the servers.
- [x] **Unix-like system calls:** lux provides a minimal Unix-like API for the common system calls, namely those related to files, sockets, and process and thread management. Many of the system calls wrap around user space servers implementing the actual functionality.
- [x] **Asynchronicity:** I/O system calls implemented by lux are fully asynchronous. The microkernel threads are never blocked nor do they wait for events, and user processes can explicitly request either blocking or asynchronous system calls in accordance with POSIX.

## Software Architecture

lux is a microkernel that provides minimal kernel-level functionality and behaves as a wrapper for a [variety of standalone servers](https://github.com/lux-operating-system/servers) running in user space, which provide the expected OS functionality. This design depends on a user space router ([lux-operating-system/lumen](https://github.com/lux-operating-system/lumen)) to forward or "route" messages between the kernel and the servers. The router additionally doubles as an [init program](https://en.wikipedia.org/wiki/Init). The servers implement driver functionality, such as device drivers, file system drivers, networking stacks, and other higher-level abstractions. lux, lumen, and the servers follow the client-server paradigm and communicate via standard Unix domain sockets.

This diagram illustrates the architecture of the various components in an operating system built on lux and lumen. It is a work in progress and is subject to change as more components are developed.

![Diagram showing the software architecture of lux](https://jewelcodes.io/res/posts/postdata/7d0ff176d0a68f16603c5030937b325f66d8bd777193d.png)

All of the described components, including the microkernel itself (with the exception of the scheduler), can be preempted and are fully multithreaded applications for a more responsive software foundation.

## Building

Visit [lux-operating-system/lux](https://github.com/lux-operating-system/lux) for the full build instructions, starting from the toolchain and ending at a disk image that can be booted on a virtual machine or real hardware. If you don't want to manually build lux, [nightly builds](https://github.com/lux-operating-system/lux/actions/workflows/nightly-mac.yml) also generate a bootable disk image that can be used on a virtual machine.

## License

The lux microkernel is free and open source software released under the terms of the MIT License. Unix is a registered trademark of The Open Group.

## Notes

1. This figure excludes blank lines, comments, header files, and the platform abstraction layer, retaining only the core microkernel code that forms the basic logic behind lux.

#

Made with 💗 from Boston and Cairo
