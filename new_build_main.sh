#! /bin/bash
set -e

# Future dev: Apologies for the hardcoded path :)
# I have a strange Node dev environment and need to juggle various versions.
/C/nodejs/node.exe ./build_cimgui.mjs
/C/nodejs/node.exe ./build_speedhack_win_node.mjs
/C/nodejs/node.exe ./build_main_win_node.mjs

# gdb -ex run test_win.exe
./test_win.exe