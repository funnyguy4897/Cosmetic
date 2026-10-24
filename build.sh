#!/bin/sh
set -e
x86_64-w64-mingw32-g++ -shared -O2 -o xinput1_4.dll dllmain.cpp xinput1_4.def \
    -static -static-libgcc -static-libstdc++
echo "Built xinput1_4.dll"
