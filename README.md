# IDLC

## Prereqs
* Python 2
* CMake
* `make`, `gcc`, etc.

## Building
`idlc` uses CMake on this branch, so with `cmake`, `make`, and GCC installed, build as follows:

    mkdir foo-build
    cd foo-build
    cmake ..
    make

If `ninja` is preferred, `cmake .. -G Ninja` will produce the needed build files.
The build outputs the `idlc` binary in the build directory root.

## Usage
Since AST construction is the only part implemented so far on this branch,
`idlc <idl-file>` will run silently and do nothing, unless there is a parse error.
`idlc <idl-file> <dump-file>` will produce a debug dump of the IDL file's AST into the given dump file.
