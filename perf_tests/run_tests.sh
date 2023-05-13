#!/bin/bash

usage() {
  echo "Usage: $0 [-T <bool>] [-v]" 1>&2;
  exit 1;
}

verbose=0
while getopts "T:v" OPTION
do
  case $OPTION in
    v)
      verbose=1
      ;;
    T)
      t=${OPTARG}
      if [ $t == true ] || [ $t == false ]; then
        cmake -DVALIDATE_TABLE:BOOL=$t build
      else
        usage
      fi
      ;;
    *)
      usage
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
    ctest --timeout 10 --output-on-failure -V
  else
    ctest --timeout 10 --output-on-failure
  fi
  cd ..
fi