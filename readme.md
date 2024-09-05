**lux** (intentionally stylized in lowercase) is a portable work-in-progress microkernel written from scratch that currently runs on x86_64, with future plans for an ARM64 port.

[![License: MIT](https://img.shields.io/github/license/lux-operating-system/kernel?color=red)](https://github.com/lux-operating-system/kernel/blob/main/LICENSE) [![Build status](https://github.com/lux-operating-system/kernel/actions/workflows/build-mac.yml/badge.svg)](https://github.com/lux-operating-system/kernel/actions) [![GitHub Issues](https://img.shields.io/github/issues/lux-operating-system/kernel)](https://github.com/lux-operating-system/kernel/issues) [![Codacy Badge](https://app.codacy.com/project/badge/Grade/3e2b1b7fdc9346988ec577bcbede86b2)](https://app.codacy.com?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)

## Overview
In less than 2,500 lines <sup>[1]</sup> of code, lux implements **memory management**, preemptive **multiprocessor scheduling**, and (several) **Unix-like system calls**. This elimination of bloat minimizes resource consumption compared to mainstream operating systems and increases stability and memory protection. lux is developed primarily as a one-person project, both as a learning and research tool as well as a criticism of the bloat that has become normalized in modern software engineering.

## Roadmap
- [x] **Portability:** lux is built on a platform abstraction layer with a set of functions and constants to be defined and implemented for each platform. This enables ease of porting lux to other CPU architectures.
- [x] **Memory management:** lux implements a memory manager that can manage up to 128 TiB of physical memory (assuming hardware support is present) and virtual address spaces of up to 256 TiB divided into 128 TiB for each of the kernel and user threads.
- [x] **Multiprocessor scheduling:** The scheduler of lux was designed with multiprocessor support from the start, and lux currently supports CPUs with up to 256 cores on x86_64 platforms. The microkernel itself is also multithreaded and can be preempted.
- [ ] **Unix-like system calls _(partial progress)_:** lux will provide a minimal Unix-like API for the common system calls, namely those related to files, sockets, and process and thread management.
- [ ] **Interprocess communication:** lux will implement kernel-level support for local Unix sockets to facilitate communication with the servers.

## Software Architecture
lux is a microkernel that provides minimal kernel-level functionality and will behave as a wrapper for a variety of servers running in user space, which will provide the expected OS functionality. This design will depend on a user space router ([lux-operating-system/lumen](https://github.com/lux-operating-system/lumen)) to forward or "route" messages between the kernel and the servers. These servers will then implement driver functionality, such as device drivers, file system drivers, networking stacks, and so on. lux, lumen, and the servers follow the client-server paradigm and will communicate via Unix sockets.

This diagram illustrates the architecture of the various components in an operating system built on lux and lumen. It is subject to change as more components are developed.

![Diagram showing the software architecture of lux](https://jewelcodes.io/res/posts/postdata/7d0ff176d0a68f16603c5030937b325f66d8bd777193d.png)

All of the described components, including the microkernel itself (with the exception of the scheduler), can be preempted and are fully multithreaded applications for a more responsive software foundation.

## Building
Visit [lux-operating-system/lux](https://github.com/lux-operating-system/lux) for the full build instructions, starting from the toolchain and ending at a disk image that can be booted on a virtual machine or real hardware. If you don't want to manually build lux, [nightly builds](https://github.com/lux-operating-system/lux/actions) also generate a bootable disk image that can be used on a virtual machine.

## License
The lux microkernel is free and open source software released under the terms of the MIT License. Unix is a registered trademark of The Open Group.

## Notes
1. This figure excludes blank lines, comments, header files, and the platform abstraction layer, retaining only the core microkernel code that forms the basic logic behind lux.

#

Made with 💗 from Boston and Cairo
