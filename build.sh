#!/bin/bash
set -e

echo "Building CMake project..."

if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

cd build

echo "Configuring CMake..."
cmake -DCMAKE_BUILD_TYPE=Debug ..

echo "Building project..."
cmake --build .

echo "Build completed successfully!"