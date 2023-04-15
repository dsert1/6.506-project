#!/bin/bash

echo "==== Building Tests ===="
cmake -S . -B build
cmake --build build
cd build
echo
echo "==== Running Tests ===="
ctest
cd ..