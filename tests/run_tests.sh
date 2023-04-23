#!/bin/bash

verbose=0
while getopts "v" OPTION
do
  case $OPTION in
    v) verbose=1
    ;;
  esac
done

echo "==== Building Tests ===="
cmake -S . -B build
cmake --build build
if [ $? -eq 0 ]; then
  cd build
  echo
  echo "==== Running Tests ===="
  if [ $verbose -eq 1 ]; then
    ctest --output-on-failure -V
  else
    ctest --output-on-failure
  fi
  cd ..
fi