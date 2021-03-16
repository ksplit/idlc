# IDLC

## Prereqs

`$ sudo apt install cmake build-essential g++ curl unzip tar python`

## Building

Before the first build, run `./setup` in the project root.

`idlc` uses CMake:
```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Usage

```
$ idlc foo.idl
```
will generate from the IDL a set of files:
- `client.c`
- `server.c`
- `common.c`
- `trampolines.lds.S`
- `common.h`

Both driver and kernel side must be built with `common.c` and `trampolines.lds.S`; `server.c` must be linked
kernel-side, and `client.c`, driver-side. The generated code expects a `glue_user.h` header in the same directory as
the generated sources, into which you should put any includes or defines needed by the prototypes of the driver / kernel
interface. The generated code makes calls to `glue_user_*` functions (these implement environment-specific
implementations of tracing, shadow tracking, or RPC message delivery), which must be defined and linked by both sides.
The `nullnet_gen` driver in the `nullnet_gen` branch of `lvd-linux` has a mostly complete set of implementations in
`glue_user.c`, which should be usable as a drop-in implementation in any other LVD driver.
