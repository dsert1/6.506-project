# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/dsert/Documents/Documents - Deniz's Macbook/MIT Semesters/Spring 2023/6.5060 - Algorithm Engineering/6.506-project/perf_tests/build/_deps/googletest-src"
  "/Users/dsert/Documents/Documents - Deniz's Macbook/MIT Semesters/Spring 2023/6.5060 - Algorithm Engineering/6.506-project/perf_tests/build/_deps/googletest-build"
  "/Users/dsert/Documents/Documents - Deniz's Macbook/MIT Semesters/Spring 2023/6.5060 - Algorithm Engineering/6.506-project/perf_tests/build/_deps/googletest-subbuild/googletest-populate-prefix"
  "/Users/dsert/Documents/Documents - Deniz's Macbook/MIT Semesters/Spring 2023/6.5060 - Algorithm Engineering/6.506-project/perf_tests/build/_deps/googletest-subbuild/googletest-populate-prefix/tmp"
  "/Users/dsert/Documents/Documents - Deniz's Macbook/MIT Semesters/Spring 2023/6.5060 - Algorithm Engineering/6.506-project/perf_tests/build/_deps/googletest-subbuild/googletest-populate-prefix/src/googletest-populate-stamp"
  "/Users/dsert/Documents/Documents - Deniz's Macbook/MIT Semesters/Spring 2023/6.5060 - Algorithm Engineering/6.506-project/perf_tests/build/_deps/googletest-subbuild/googletest-populate-prefix/src"
  "/Users/dsert/Documents/Documents - Deniz's Macbook/MIT Semesters/Spring 2023/6.5060 - Algorithm Engineering/6.506-project/perf_tests/build/_deps/googletest-subbuild/googletest-populate-prefix/src/googletest-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/dsert/Documents/Documents - Deniz's Macbook/MIT Semesters/Spring 2023/6.5060 - Algorithm Engineering/6.506-project/perf_tests/build/_deps/googletest-subbuild/googletest-populate-prefix/src/googletest-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/dsert/Documents/Documents - Deniz's Macbook/MIT Semesters/Spring 2023/6.5060 - Algorithm Engineering/6.506-project/perf_tests/build/_deps/googletest-subbuild/googletest-populate-prefix/src/googletest-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
