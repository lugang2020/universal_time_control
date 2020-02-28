#! /bin/bash
set -e

# ./build_cimgui_win.sh
# ./new_build_main.sh

rm -rf _release
mkdir _release

cp test_win.exe ui_impl.dll _release
cp libgcc_s_sjlj-1.dll libstdc++-6.dll sqlite3.dll utf8proc.dll _release
cp -R data _release