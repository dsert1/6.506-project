if(EXISTS "/Users/dsert/Documents/Documents - Deniz's Macbook/MIT Semesters/Spring 2023/6.5060 - Algorithm Engineering/6.506-project/perf_tests/build/normal_perf[1]_tests.cmake")
  include("/Users/dsert/Documents/Documents - Deniz's Macbook/MIT Semesters/Spring 2023/6.5060 - Algorithm Engineering/6.506-project/perf_tests/build/normal_perf[1]_tests.cmake")
else()
  add_test(normal_perf_NOT_BUILT normal_perf_NOT_BUILT)
endif()
