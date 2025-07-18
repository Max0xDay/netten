#!/bin/bash

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

check_system() {
    if [[ "$OSTYPE" != "linux-gnu"* ]]; then
        print_error "This tool is designed for Linux systems only"
        exit 1
    fi
}

check_dependencies() {
    print_status "Checking dependencies..."
    if ! command -v gcc &> /dev/null; then
        print_error "GCC compiler not found. Please install build-essential"
        exit 1
    fi
    print_success "GCC found"
}

clean_build() {
    print_status "Cleaning previous builds..."
    rm -f netten
    print_success "Clean completed"
}

build_project() {
    print_status "Compiling with GCC..."
    print_status "gcc -Wall -Wextra -std=c99 -O2 -s -static -o netten netten.c"
    
    if gcc -Wall -Wextra -std=c99 -O2 -s -static -o netten netten.c; then
        print_success "Compilation completed"
    else
        print_error "Compilation failed"
        exit 1
    fi
}

test_build() {
    print_status "Testing build..."
    if [[ ! -f "./netten" ]]; then
        print_error "Binary not found"
        exit 1
    fi
    if ./netten help &> /dev/null; then
        print_success "Binary test passed"
    else
        print_error "Binary test failed"
        exit 1
    fi
}

install_system() {
    print_status "Installing to /usr/local/bin..."
    if sudo cp netten /usr/local/bin/ && sudo chmod +x /usr/local/bin/netten; then
        print_success "Installation completed"
    else
        print_error "Installation failed"
        exit 1
    fi
}

show_usage() {
    echo "netten Build Script"
    echo "Usage: $0 [option]"
    echo "Options:"
    echo "  build     - Compile the project (default)"
    echo "  clean     - Clean build artifacts"
    echo "  install   - Build and install system-wide"
    echo "  help      - Show this help"
}

main() {
    local action="${1:-build}"
    
    case "$action" in
        "build")
            check_system
            check_dependencies
            clean_build
            build_project
            test_build
            print_success "Build completed!"
            echo "Usage: ./netten help"
            ;;
        "clean")
            clean_build
            ;;
        "install")
            check_system
            check_dependencies
            clean_build
            build_project
            test_build
            install_system
            ;;
        "help"|"-h"|"--help")
            show_usage
            ;;
        *)
            print_error "Unknown option: $action"
            show_usage
            exit 1
            ;;
    esac
}

main "$@"
