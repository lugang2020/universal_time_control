#! /bin/bash
set -e

export PATH=$PATH:/f/i686-8.1.0-win32-sjlj-rt_v6-rev0/mingw32/bin
export PATH=$PATH:/d/i686-8.1.0-win32-sjlj-rt_v6-rev0/mingw32/bin

# Future dev: Apologies for the hardcoded path :)
# I have a strange Node dev environment and need to juggle various versions.
/C/nodejs/node.exe ./build_cimgui.mjs
/C/nodejs/node.exe ./build_speedhack_win_node.mjs
/C/nodejs/node.exe ./build_main_win_node.mjs

# gdb -ex run test_win.exe
./test_win.exe