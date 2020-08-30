You will need a recent version of Node and a 32-bit MinGW-W64(i686-8.1.0-win32-sjlj-rt_v6-rev0) build in your PATH.
You will also need Bash and other basic Unix-y tools- the ones provided by Git Bash
are enough.

Run ./new_build_main.sh to build the speedhack, injector, and UI.
Doing so with LIVE_RELOAD=1 defined will enable a limited hot reload
system (powered by tcc), intended to allow rapid UI iteration.
