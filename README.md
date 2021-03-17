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

You will also need to implement the KLCD dispatch loop and the LVDs `handle_rpc_calls` dispatcher, an implementation
of the former of wich you'll find in `nullnet_gen/net_klcd/main.c`.

Currently one manual modification is always required: the LVD requires certain init / exit calls (i.e.
`__dummy_lcd_init()` in nullnet), and you will have to add RPC_IDs for these in `common.h` and handle them explicitly in
your dispatch loop code. Note also that `glue_user_init()` in the existing `glue_user.c` **must** be called by both
sides before the glue layer can track shadows, as this call initializes the employed hashmaps.

Therefore, what `nullnet_gen` modifies manually is as follows:
- Adding `MODULE_INIT` and `MODULE_EXIT` `RPC_ID`s
- Adding switch cases to the driver-side (i.e. client-side) dispatch code (in `nullnet_gen/client.c:try_dispatch`) to handle these and call `__dummy_lcd_init()` and `__dummy_lcd_exit()`, respectively
- Calling `glue_user_init()` in the module init functions of both the KLCD and LCD before any RPC calls
	- Refer to `nullnet_gen/net_klcd/main.c:99` and `nullnet_gen/dummy_lcd/main.c:74`
