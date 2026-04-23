cmake_minimum_required(VERSION 3.10)
project(libcontain VERSION 1.0.0 LANGUAGES C)

# C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# POSIX extensions for strdup() and other functions
add_compile_definitions(_POSIX_C_SOURCE=200809L)

# ============================================================================
# Build Options
# ============================================================================

option(BUILD_SHARED_LIBS "Build shared library" OFF)
option(CONTAINER_DEBUG "Enable debug mode" OFF)
option(BUILD_TESTS "Build tests" ON)
option(BUILD_BENCHMARKS "Build benchmarks" ON)

# ============================================================================
# Compiler Flags
# ============================================================================

# Base flags (applied to all builds)
set(BASE_C_FLAGS "-Wall -Wextra -Wpedantic -fPIC")

# Build type flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(CONTAINER_DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g")
elseif(CMAKE_BUILD_TYPE STREQUAL "Sanitize")
    add_compile_definitions(CONTAINER_DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g -fsanitize=address,undefined")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
endif()

# Math library
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${BASE_C_FLAGS} -lm")

# ============================================================================
# Library
# ============================================================================

# Collect all source files
file(GLOB LIB_SOURCES "src/*.c")

# Check if sources exist
list(LENGTH LIB_SOURCES SOURCE_COUNT)
if(${SOURCE_COUNT} EQUAL 0)
    message(FATAL_ERROR "No source files found in src/ directory")
endif()

# Create library target
add_library(libcontain ${LIB_SOURCES})
target_include_directories(libcontain
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Set library properties
set_target_properties(libcontain PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    OUTPUT_NAME contain
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# ============================================================================
# Tour
# ============================================================================

add_executable(tour tests/tour.c)
target_link_libraries(tour libcontain)
set_target_properties(tour PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

add_custom_target(run-tour
    COMMAND ${CMAKE_BINARY_DIR}/tests/tour
    DEPENDS tour
    COMMENT "Running libcontain tour"
)

# ============================================================================
# Tests
# ============================================================================

if(BUILD_TESTS)
    enable_testing()
    
    # Test names list
    set(TEST_NAMES 
        vector deque linkedlist hashset hashmap iterator chainer chainer2 typed allocator
    )
    
    # Create test executables using a function
    function(add_libcontain_test NAME)
        add_executable(${NAME}_test tests/${NAME}_test.c)
        target_link_libraries(${NAME}_test libcontain)
        set_target_properties(${NAME}_test PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
        )
        add_test(NAME ${NAME} COMMAND ${CMAKE_BINARY_DIR}/tests/${NAME}_test)
        
        # Add individual test target
        add_custom_target(test-${NAME}
            COMMAND ${CMAKE_BINARY_DIR}/tests/${NAME}_test
            DEPENDS ${NAME}_test
            COMMENT "Running ${NAME} tests"
        )
    endfunction()
    
    # Create all tests
    foreach(TEST_NAME ${TEST_NAMES})
        add_libcontain_test(${TEST_NAME})
    endforeach()
    
    # Add tour test
    add_test(NAME tour COMMAND ${CMAKE_BINARY_DIR}/tests/tour)
    
    # Custom target to run all tests with tour
    add_custom_target(test-all
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        COMMAND ${CMAKE_BINARY_DIR}/tests/tour
        DEPENDS ${TEST_NAMES}
        COMMENT "Running all tests and tour"
    )
endif()

# ============================================================================
# Benchmarks
# ============================================================================

if(BUILD_BENCHMARKS)
    # Benchmark names list
    set(BENCH_NAMES 
        vector deque linkedlist hashset hashmap pipeline pipeline2 typed-vs-generic
        vs-uthash vs-klib vs-stb-ds vs-cpp
    )
    
    # Create benchmark executables using a function
    function(add_libcontain_benchmark NAME)
        # Convert hyphens to underscores for target name
        string(REPLACE "-" "_" SAFE_NAME ${NAME})
        
        add_executable(bench_${SAFE_NAME} benchmarks/bench_${NAME}.c)
        target_link_libraries(bench_${SAFE_NAME} libcontain)
        set_target_properties(bench_${SAFE_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarks
        )
        
        # Add individual benchmark target
        add_custom_target(bench-${NAME}
            COMMAND ${CMAKE_BINARY_DIR}/benchmarks/bench_${SAFE_NAME}
            DEPENDS bench_${SAFE_NAME}
            COMMENT "Running ${NAME} benchmark"
        )
    endfunction()
    
    # Create all benchmarks
    foreach(BENCH_NAME ${BENCH_NAMES})
        # Check if benchmark source exists
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/bench_${BENCH_NAME}.c)
            add_libcontain_benchmark(${BENCH_NAME})
        endif()
    endforeach()
    
    # Custom target to run all benchmarks
    add_custom_target(bench
        COMMENT "All benchmarks completed"
    )
    
    add_custom_target(bench-all DEPENDS bench)
endif()

# ============================================================================
# Code Quality
# ============================================================================

# Format target
add_custom_target(format
    COMMAND clang-format -i include/contain/*.h src/*.c tests/*.c benchmarks/*.c
    COMMENT "Formatting source files"
)

# Check target (static analysis)
add_custom_target(check
    COMMAND clang-tidy include/contain/*.h src/*.c -- -Iinclude
    COMMENT "Running static analysis"
)

# ============================================================================
# Coverage
# ============================================================================

# Coverage target (requires gcovr)
add_custom_target(coverage
    COMMAND ${CMAKE_COMMAND} -E echo "Coverage target - build with -DCMAKE_C_FLAGS=\"-fprofile-arcs -ftest-coverage\" and run tests first"
    COMMAND gcovr --html --html-details -o ${CMAKE_BINARY_DIR}/coverage.html
    COMMENT "Generating coverage report"
)

# ============================================================================
# Installation
# ============================================================================

include(GNUInstallDirs)

# Install library
install(TARGETS libcontain
    EXPORT libcontainTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install headers
install(DIRECTORY include/contain/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/contain
    FILES_MATCHING PATTERN "*.h"
)

# Install typed headers
install(DIRECTORY include/contain/typed/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/contain/typed
    FILES_MATCHING PATTERN "*.h"
)

# Install CMake targets file
install(EXPORT libcontainTargets
    FILE libcontainTargets.cmake
    NAMESPACE libcontain::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libcontain
)

# ============================================================================
# pkg-config Support
# ============================================================================

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/libcontain.pc.cmake.in
    ${CMAKE_BINARY_DIR}/libcontain.pc
    @ONLY
)

install(FILES ${CMAKE_BINARY_DIR}/libcontain.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# ============================================================================
# Cleanup
# ============================================================================

# Note: CMake handles build directory cleanup automatically
# Use: cmake --build . --target clean

# ============================================================================
# Help Target
# ============================================================================

add_custom_target(print-help
    COMMAND ${CMAKE_COMMAND} -E echo "========================================="
    COMMAND ${CMAKE_COMMAND} -E echo "  libcontain Build System v${PROJECT_VERSION}"
    COMMAND ${CMAKE_COMMAND} -E echo "========================================="
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Building:"
    COMMAND ${CMAKE_COMMAND} -E echo "  make              - Build static and shared libraries"
    COMMAND ${CMAKE_COMMAND} -E echo "  make clean        - Remove build artifacts"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Build Types:"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake .. -DCMAKE_BUILD_TYPE=Release   - Optimized build (default)"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake .. -DCMAKE_BUILD_TYPE=Debug     - Debug build with symbols"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake .. -DCMAKE_BUILD_TYPE=Sanitize  - Build with AddressSanitizer"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Tour:"
    COMMAND ${CMAKE_COMMAND} -E echo "  make tour         - Build tour program"
    COMMAND ${CMAKE_COMMAND} -E echo "  make run-tour     - Build and run tour"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Testing:"
    COMMAND ${CMAKE_COMMAND} -E echo "  make test         - Run all tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  make test-all     - Run all tests and tour"
    COMMAND ${CMAKE_COMMAND} -E echo "  make test-vector  - Run vector tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  make test-deque   - Run deque tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  make test-linkedlist - Run linkedlist tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  make test-hashset - Run hashset tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  make test-hashmap - Run hashmap tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  make test-iterator - Run iterator tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  make test-chainer - Run chainer tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  make test-chainer2 - Run chainer2 tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  make test-typed   - Run typed wrapper tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  make test-allocator - Run allocator tests"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Benchmarks:"
    COMMAND ${CMAKE_COMMAND} -E echo "  make bench        - Run all benchmarks"
    COMMAND ${CMAKE_COMMAND} -E echo "  make bench-vector - Run vector benchmarks"
    COMMAND ${CMAKE_COMMAND} -E echo "  make bench-deque  - Run deque benchmarks"
    COMMAND ${CMAKE_COMMAND} -E echo "  make bench-linkedlist - Run linkedlist benchmarks"
    COMMAND ${CMAKE_COMMAND} -E echo "  make bench-hashset - Run hashset benchmarks"
    COMMAND ${CMAKE_COMMAND} -E echo "  make bench-hashmap - Run hashmap benchmarks"
    COMMAND ${CMAKE_COMMAND} -E echo "  make bench-pipeline - Run pipeline benchmarks"
    COMMAND ${CMAKE_COMMAND} -E echo "  make bench-pipeline2 - Run pipeline2 benchmarks"
    COMMAND ${CMAKE_COMMAND} -E echo "  make bench-typed-vs-generic - Compare typed vs generic"
    COMMAND ${CMAKE_COMMAND} -E echo "  make bench-vs-uthash - Compare with uthash"
    COMMAND ${CMAKE_COMMAND} -E echo "  make bench-vs-klib - Compare with klib"
    COMMAND ${CMAKE_COMMAND} -E echo "  make bench-vs-stb-ds - Compare with stb_ds"
    COMMAND ${CMAKE_COMMAND} -E echo "  make bench-vs-cpp - Compare with C++"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Installation:"
    COMMAND ${CMAKE_COMMAND} -E echo "  make install      - Install library and headers"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Code Quality:"
    COMMAND ${CMAKE_COMMAND} -E echo "  make format       - Format code with clang-format"
    COMMAND ${CMAKE_COMMAND} -E echo "  make check        - Run static analysis with clang-tidy"
    COMMAND ${CMAKE_COMMAND} -E echo "  make coverage     - Generate coverage report"
    COMMAND ${CMAKE_COMMAND} -E echo ""
)