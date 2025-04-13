project(Tests)

include(CTest)

# Add your executable
add_executable(tests
    main.cpp
)

# Link gtest to your executable
target_link_libraries(tests
    gtest::gtest
)

add_test(tests tests)
