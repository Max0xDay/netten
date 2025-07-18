#!/bin/bash

echo "Compiling netten with GCC..."
echo "gcc -Wall -Wextra -std=c99 -O2 -s -static -o netten netten.c"

gcc -Wall -Wextra -std=c99 -O2 -s -static -o netten netten.c

if [ $? -eq 0 ]; then
    echo "Compilation successful!"
    echo "Binary created: ./netten"
    echo "Usage: ./netten help"
    
    if ./netten help > /dev/null 2>&1; then
        echo "Binary test passed"
    else
        echo "Binary test failed"
    fi
else
    echo "Compilation failed"
    exit 1
fi
