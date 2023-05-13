cmake_minimum_required(VERSION 3.14)
project(my_project)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 14)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_subdirectory(../quotient_filter qf_build)
add_subdirectory(../quotient_filter_graveyard_hashing qfg_build)

add_executable(
  normal_perf
  $<TARGET_OBJECTS:QuotientFilter>
  $<TARGET_OBJECTS:QuotientFilterGraveyard>
  normal_perf.cc
  graveyard_perf
  graveyard_perf.cc
)
target_link_libraries(
  normal_perf
  QuotientFilter
  QuotientFilterGraveyard
  graveyard_perf
  GTest::gtest_main
)



include(GoogleTest)
gtest_discover_tests(normal_perf)
# gtest_discover_tests(graveyard_perf)