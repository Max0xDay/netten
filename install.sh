#!/bin/bash

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

install_precompiled() {
    print_status "Installing pre-compiled netten binary..."
    
    if [[ ! -f "netten" ]]; then
        print_error "netten binary not found in current directory"
        print_error "This script expects a pre-compiled 'netten' binary"
        exit 1
    fi
    
    if [[ ! -x "netten" ]]; then
        print_status "Making binary executable..."
        chmod +x netten
    fi
    
    print_status "Testing binary..."
    if ./netten help &> /dev/null; then
        print_success "Binary test passed"
    else
        print_error "Binary test failed"
        exit 1
    fi
    
    print_status "Installing to /usr/local/bin..."
    if sudo cp netten /usr/local/bin/ && sudo chmod +x /usr/local/bin/netten; then
        print_success "Installation completed!"
        echo "You can now run: netten help"
    else
        print_error "Installation failed"
        exit 1
    fi
}

compile_and_install() {
    print_status "Compiling and installing netten..."
    
    if [[ ! -f "netten.c" ]]; then
        print_error "netten.c source file not found"
        exit 1
    fi
    
    if ! command -v gcc &> /dev/null; then
        print_error "GCC compiler not found. Please install build-essential"
        exit 1
    fi
    
    print_status "Compiling..."
    if gcc -Wall -Wextra -std=c99 -O2 -s -static -o netten netten.c; then
        print_success "Compilation completed"
    else
        print_error "Compilation failed"
        exit 1
    fi
    
    install_precompiled
}

uninstall_netten() {
    print_status "Uninstalling netten..."
    sudo rm -f /usr/local/bin/netten
    print_success "netten uninstalled"
}

show_help() {
    echo "netten Installation Script"
    echo "Usage: $0 [option]"
    echo "Options:"
    echo "  install     - Install pre-compiled binary (default)"
    echo "  compile     - Compile from source and install"
    echo "  uninstall   - Remove netten"
    echo "  help        - Show this help"
}

main() {
    case "${1:-install}" in
        "install")
            install_precompiled
            ;;
        "compile")
            compile_and_install
            ;;
        "uninstall")
            uninstall_netten
            ;;
        "help"|"-h"|"--help")
            show_help
            ;;
        *)
            print_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
}

main "$@"
