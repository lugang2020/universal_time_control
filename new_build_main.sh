#! /bin/bash
set -e

# C:/Users/JonathanWork/AppData/Roaming/nvm/v13.7.0/node.exe ./build_cimgui.mjs
# C:/Users/JonathanWork/AppData/Roaming/nvm/v13.7.0/node.exe ./build_speedhack_win_node.mjs
C:/Users/JonathanWork/AppData/Roaming/nvm/v13.7.0/node.exe ./build_main_win_node.mjs

# gdb -ex run test_win.exe
./test_win.exe