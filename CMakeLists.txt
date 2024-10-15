# Add this to your CMakeLists.txt
enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(runTests tests/calculateBondPrice_test.cpp)
target_link_libraries(runTests ${GTEST_LIBRARIES} pthread)

add_test(NAME runTests COMMAND runTests)