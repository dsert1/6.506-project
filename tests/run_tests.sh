#!/bin/bash

echo "==== Building Tests ===="
cmake -S . -B build
cmake --build build
if [ $? -eq 0 ]; then
  cd build
  echo
  echo "==== Running Tests ===="
  ctest
  cd ..
fi