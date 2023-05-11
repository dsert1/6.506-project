cmake_minimum_required(VERSION 3.14)
project(project)

# variables to determine which tests to run
set(VALIDATE_TABLE false CACHE BOOL "whether to run assertions related to table state")

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

add_subdirectory(../quotient_filter qf_build)
add_subdirectory(../quotient_filter_graveyard_hashing qfg_build)

enable_testing()

add_executable(
  normal_filter_tests $<TARGET_OBJECTS:QuotientFilter> normal_filter_tests.cc
)
add_executable(
  graveyard_filter_tests $<TARGET_OBJECTS:QuotientFilterGraveyard> graveyard_filter_tests.cc
)
add_executable(
  redistribution_tests $<TARGET_OBJECTS:QuotientFilterGraveyard> redistribution_tests.cc
)
target_link_libraries(normal_filter_tests
  QuotientFilter
  GTest::gtest_main
)
target_link_libraries(graveyard_filter_tests
  QuotientFilterGraveyard
  GTest::gtest_main
)
target_link_libraries(redistribution_tests
  QuotientFilterGraveyard
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(normal_filter_tests EXTRA_ARGS ${VALIDATE_TABLE})
gtest_discover_tests(graveyard_filter_tests EXTRA_ARGS ${VALIDATE_TABLE})
gtest_discover_tests(redistribution_tests EXTRA_ARGS ${VALIDATE_TABLE})