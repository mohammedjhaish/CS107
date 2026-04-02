# Heap Allocator (CS107)

Custom implementation of `malloc`, `free`, and `realloc` in C using implicit and explicit free lists, optimized for memory utilization and throughput.

## Overview

This project implements a dynamic memory allocator from scratch in C, similar to the behavior of the standard `malloc/free` interface.

The allocator manages a contiguous memory segment, supports allocation, deallocation, and resizing, and includes optimizations such as block splitting, coalescing, and in-place `realloc`.

Two designs were implemented:
- Implicit free list
- Explicit free list (optimized)

## Features

- Custom `malloc`, `free`, and `realloc` implementations
- Implicit and explicit free list designs
- Block splitting to reduce fragmentation
- Coalescing of adjacent free blocks
- In-place `realloc` optimization
- 8-byte alignment for all allocations
- Heap validation and debugging utilities

## Design

### Memory Layout
Each block consists of:
- Header (stores size and allocation status)
- Payload

All blocks are aligned to 8 bytes to ensure proper memory access.

### Implicit Free List
- Traverses all blocks linearly to find a free block
- Simple design but less efficient for large heaps

### Explicit Free List
- Maintains a doubly linked list of free blocks
- Stores next/prev pointers inside free block payload
- Enables faster allocation and deallocation (O(1) list updates)

### Coalescing
Adjacent free blocks are merged during `free` to reduce external fragmentation and improve future allocations.

### Reallocation
Attempts in-place expansion by merging with neighboring free blocks when possible, avoiding unnecessary memory copying.

## Performance

Measured using the provided test harness and Valgrind callgrind:

- ~39 instructions per allocation (explicit allocator)
- ~33% average memory utilization on large traces
- Significant throughput improvement compared to implicit free list

## Project Structure

- `implicit.c` – implicit free list allocator
- `explicit.c` – optimized explicit free list allocator
- `segment.c/h` – memory segment interface
- `test_harness.c` – testing framework
- `samples/` – test scripts

## Build & Run

```bash
make
./test_explicit samples/example1.script
