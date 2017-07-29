#!/usr/bin/env zsh
cloc . --include-lang='C','C/C++ Header' --exclude-dir=cmake-build-debug,build
