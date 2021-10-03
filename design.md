# Design of `idlc`

## Static RPCS

- How do normal RPCs work?
    1. source of pointer passes their function pointer across the boundary
    2. during unmarshaling, the trampoline stub is copied, marked executable, the pointer in unpacked, stash just ahead of it in memory, and the completed trampoline is then returned, to be placed into teh correct field
    3. during a function call, the trampoline grabs the pointer from behind itself, packs it ahead of the RPC message, and the receiver uses the passed pointer to perform the indirect call
- How should static RPCs work?
    - Can't use trampolines, have only one call stub
    - "Real" pointer is stashed in a global variable (named according to scope, of course)
    - "Real" pointer never has to reach the "other side" (no need for more leaky isolations)
    - During marshaling, pointer is stashed, assert is fired if global wasn't already null
    - only a boolean is passed, indicating the "presence" of the function pointer (i.e., non-nullness
        - If we enforce an assert on `NULL`, we don't have to pass anything
    - during unmarshaling, a "trampoline" is inserted to implement call logic
        - if a presence value is used, it will decide if the trampoline or `NULL` is inserted
    - call behaves as a normal, direct rpc
    - on receiving side, the implementer indirects the call through the stored global (which can never be `NULL` by now)
