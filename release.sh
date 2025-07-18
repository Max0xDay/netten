#!/bin/bash

set -e

echo "Preparing netten release..."

if [[ ! -f "netten.c" ]]; then
    echo "Error: netten.c not found"
    exit 1
fi

echo "Compiling optimized binary..."
gcc -Wall -Wextra -std=c99 -O2 -s -static -o netten netten.c

echo "Testing binary..."
if ./netten help > /dev/null 2>&1; then
    echo "Binary test passed"
else
    echo "Binary test failed"
    exit 1
fi

echo "Creating release package..."
mkdir -p netten-release
cp netten netten-release/
cp install.sh netten-release/
cp README.md netten-release/

echo "Creating tarball..."
tar -czf netten-linux-x64.tar.gz netten-release/

echo "Cleaning up..."
rm -rf netten-release/

echo "Release ready: netten-linux-x64.tar.gz"
echo "Binary size: $(du -h netten | cut -f1)"
echo "Package size: $(du -h netten-linux-x64.tar.gz | cut -f1)"
