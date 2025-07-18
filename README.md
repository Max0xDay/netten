# netten

A command-line network monitoring tool for Linux systems.

## Quick Start

```bash
gcc -o netten netten.c
./netten help
```

## Commands

```bash
./netten interfaces     # List network interfaces
./netten traffic        # Show traffic statistics
./netten ports          # Show listening ports
./netten ip             # Show IP addresses
./netten mac            # Show MAC addresses
./netten drivers        # Show driver information
./netten speed          # Show link speeds
./netten stats          # Show detailed statistics
./netten connections    # Show active connections
./netten route          # Show routing table
```

## Build Options

```bash
# Simple compilation
gcc -o netten netten.c

# Optimized compilation
gcc -Wall -Wextra -std=c99 -O2 -s -static -o netten netten.c

# Using build script
./build.sh

# Using compile script
./compile.sh
```

## Installation

```bash
# Install pre-compiled binary
./install.sh

# Compile and install
./install.sh compile

# Manual installation
gcc -o netten netten.c
sudo cp netten /usr/local/bin/
```
