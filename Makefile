# libcontain Makefile
# ============================================================================
# Configuration
# ============================================================================

CC = gcc
AR = ar
ARFLAGS = rcs
VERSION = 1.0.0

# POSIX feature test macro for strdup() and other extensions
POSIX_FLAGS = -D_POSIX_C_SOURCE=200809L

# Build directory
BUILD_DIR = build

# Compiler flags - base flags without include path or POSIX flags
BASE_CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -fPIC

# Build variants
RELEASE_CFLAGS = $(BASE_CFLAGS) -O2 -Iinclude $(POSIX_FLAGS)
DEBUG_CFLAGS = $(BASE_CFLAGS) -O0 -g -DCONTAINER_DEBUG -Iinclude $(POSIX_FLAGS)
SANITIZE_CFLAGS = $(BASE_CFLAGS) -O0 -g -fsanitize=address,undefined -DCONTAINER_DEBUG -Iinclude $(POSIX_FLAGS)

CFLAGS = $(RELEASE_CFLAGS)
LDFLAGS = -lm

# Parallel builds
MAKEFLAGS += -j$(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Installation directories
PREFIX = /usr/local
INCDIR = $(PREFIX)/include
LIBDIR = $(PREFIX)/lib

# ============================================================================
# Source Files
# ============================================================================

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,$(BUILD_DIR)/src/%.o,$(SRCS))
DEP_FILES = $(OBJS:.o=.d)
LIBRARY = $(BUILD_DIR)/libcontain.a
SHARED_LIBRARY = $(BUILD_DIR)/libcontain.so.$(VERSION)
SHARED_LIBRARY_SONAME = libcontain.so.1

# Check if sources exist
ifeq ($(SRCS),)
$(error No source files found in src/ directory)
endif

# ============================================================================
# Test Files
# ============================================================================

TEST_DIR = tests
TEST_NAMES = vector deque linkedlist hashset hashmap iterator chainer chainer2 typed allocator
TEST_BINS = $(addprefix $(BUILD_DIR)/$(TEST_DIR)/, $(addsuffix _test, $(TEST_NAMES)))
TEST_TARGETS = $(addprefix test-, $(TEST_NAMES))

# ============================================================================
# Benchmark Files
# ============================================================================

BENCH_DIR = benchmarks
BENCH_NAMES = vector deque linkedlist hashset hashmap pipeline pipeline2 typed_vs_generic \
              vs_uthash vs_klib vs_stb_ds
BENCH_BINS = $(addprefix $(BUILD_DIR)/$(BENCH_DIR)/, $(addsuffix , $(BENCH_NAMES)))
BENCH_TARGETS = $(addprefix bench-, $(BENCH_NAMES))

# ============================================================================
# Tour
# ============================================================================

TOUR = $(BUILD_DIR)/tests/tour
TOUR_SRC = tests/tour.c

# ============================================================================
# Dependency Flags
# ============================================================================

DEPFLAGS = -MMD -MP
CFLAGS += $(DEPFLAGS)

# ============================================================================
# Phony Targets
# ============================================================================

.PHONY: all clean install uninstall test tour benchmark bench-all help \
        debug sanitize release format check distclean run-tour install-pc \
        $(TEST_TARGETS) $(BENCH_TARGETS)

# ============================================================================
# Main Build
# ============================================================================

all: $(LIBRARY) $(SHARED_LIBRARY)

$(LIBRARY): $(OBJS)
	@echo "  AR       $@"
	$(AR) $(ARFLAGS) $@ $^

$(SHARED_LIBRARY): $(OBJS)
	@echo "  LD       $@"
	$(CC) -shared -Wl,-soname,$(SHARED_LIBRARY_SONAME) -o $@ $^ $(LDFLAGS)
	ln -sf $@ $(BUILD_DIR)/$(SHARED_LIBRARY_SONAME)
	ln -sf $(SHARED_LIBRARY_SONAME) $(BUILD_DIR)/libcontain.so

$(BUILD_DIR)/src/%.o: src/%.c | $(BUILD_DIR)/src
	@echo "  CC       $<"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/src:
	mkdir -p $@

# Include dependency files
-include $(DEP_FILES)

# ============================================================================
# Build Variants
# ============================================================================

release: CFLAGS = $(RELEASE_CFLAGS)
release: clean all

debug: CFLAGS = $(DEBUG_CFLAGS)
debug: clean all

sanitize: CFLAGS = $(SANITIZE_CFLAGS)
sanitize: clean all

# ============================================================================
# Tour
# ============================================================================

tour: $(TOUR)

$(TOUR): $(TOUR_SRC) $(LIBRARY) | $(BUILD_DIR)/tests
	@echo "  CC       $@"
	$(CC) $(CFLAGS) $(TOUR_SRC) -o $(TOUR) $(LIBRARY) $(LDFLAGS)
	@echo ""
	@echo "========================================="
	@echo "  Tour built successfully!"
	@echo "  Run ./$(TOUR) to see the complete demo"
	@echo "========================================="

$(BUILD_DIR)/tests:
	mkdir -p $@

run-tour: tour
	./$(TOUR)

# ============================================================================
# Tests
# ============================================================================

test: $(TEST_TARGETS) tour
	@echo ""
	@echo "========================================="
	@echo "  Running Tests"
	@echo "========================================="
	@for test in $(TEST_BINS); do \
		echo ""; \
		echo "=== $$test ==="; \
		./$$test; \
	done
	@echo ""
	@echo "========================================="
	@echo "  Running Tour"
	@echo "========================================="
	@./$(TOUR)
	@echo ""
	@echo "========================================="
	@echo "  All tests passed!"
	@echo "========================================="

# Pattern rule for test binaries
$(BUILD_DIR)/$(TEST_DIR)/%_test: $(TEST_DIR)/%_test.c $(LIBRARY) | $(BUILD_DIR)/$(TEST_DIR)
	@echo "  CC       $@"
	$(CC) $(CFLAGS) $< -o $@ $(LIBRARY) $(LDFLAGS)

$(BUILD_DIR)/$(TEST_DIR):
	mkdir -p $@

# Individual test targets - use pattern to avoid repetition
$(TEST_TARGETS): test-%: $(BUILD_DIR)/$(TEST_DIR)/%_test $(LIBRARY)
	./$(BUILD_DIR)/$(TEST_DIR)/$*_test

# ============================================================================
# Benchmarks
# ============================================================================

bench: $(BENCH_TARGETS)
	@echo ""
	@echo "========================================="
	@echo "  All Benchmarks Completed"
	@echo "========================================="

# Pattern rule for benchmark binaries
$(BUILD_DIR)/$(BENCH_DIR)/%: $(BENCH_DIR)/%.c $(LIBRARY) | $(BUILD_DIR)/$(BENCH_DIR)
	@echo "  CC       $@"
	$(CC) $(CFLAGS) $< -o $@ $(LIBRARY) $(LDFLAGS)

$(BUILD_DIR)/$(BENCH_DIR):
	mkdir -p $@

# Individual benchmark targets - use pattern to avoid repetition
$(BENCH_TARGETS): bench-%: $(BUILD_DIR)/$(BENCH_DIR)/bench_% $(LIBRARY)
	./$(BUILD_DIR)/$(BENCH_DIR)/bench_$*

bench-all: bench

# ============================================================================
# Installation
# ============================================================================

install: $(LIBRARY) $(SHARED_LIBRARY)
	@echo "  INSTALL  headers"
	mkdir -p $(DESTDIR)$(INCDIR)/contain
	mkdir -p $(DESTDIR)$(INCDIR)/contain/typed
	cp -r include/contain/*.h $(DESTDIR)$(INCDIR)/contain/
	cp -r include/contain/typed/*.h $(DESTDIR)$(INCDIR)/contain/typed/
	@echo "  INSTALL  $(LIBRARY)"
	install -m 644 $(LIBRARY) $(DESTDIR)$(LIBDIR)/
	@echo "  INSTALL  $(SHARED_LIBRARY)"
	install -m 755 $(SHARED_LIBRARY) $(DESTDIR)$(LIBDIR)/
	ln -sf $(SHARED_LIBRARY) $(DESTDIR)$(LIBDIR)/$(SHARED_LIBRARY_SONAME)
	ln -sf $(SHARED_LIBRARY_SONAME) $(DESTDIR)$(LIBDIR)/libcontain.so
	@echo "  INSTALL  complete"

uninstall:
	@echo "  REMOVE   headers"
	rm -rf $(DESTDIR)$(INCDIR)/contain
	@echo "  REMOVE   libraries"
	rm -f $(DESTDIR)$(LIBDIR)/$(LIBRARY)
	rm -f $(DESTDIR)$(LIBDIR)/$(SHARED_LIBRARY)
	rm -f $(DESTDIR)$(LIBDIR)/$(SHARED_LIBRARY_SONAME)
	rm -f $(DESTDIR)$(LIBDIR)/libcontain.so
	@echo "  UNINSTALL complete"

# ============================================================================
# pkg-config Support
# ============================================================================

PKG_CONFIG = libcontain.pc

$(PKG_CONFIG): libcontain.pc.in
	sed 's|@VERSION@|$(VERSION)|g; s|@PREFIX@|$(PREFIX)|g' $< > $@

install-pc: $(PKG_CONFIG)
	mkdir -p $(DESTDIR)$(LIBDIR)/pkgconfig
	install -m 644 $(PKG_CONFIG) $(DESTDIR)$(LIBDIR)/pkgconfig/

# ============================================================================
# Code Quality
# ============================================================================

format:
	@echo "  FORMAT   source files"
	clang-format -i include/contain/*.h src/*.c tests/*.c benchmarks/*.c

check:
	@echo "  CHECK    static analysis"
	clang-tidy include/contain/*.h src/*.c -- $(CFLAGS)

# ============================================================================
# Coverage
# ============================================================================

coverage: CFLAGS = -std=c99 -Wall -Wextra -O0 -g -fprofile-arcs -ftest-coverage -Iinclude $(POSIX_FLAGS)
coverage: clean test
	@echo "  COVERAGE generating report"
	gcovr --html --html-details -o $(BUILD_DIR)/coverage.html
	@echo "  Coverage report: $(BUILD_DIR)/coverage.html"

# ============================================================================
# Cleanup
# ============================================================================

clean:
	@echo "  CLEAN    build directory"
	rm -rf $(BUILD_DIR)
	@echo "  CLEAN    pkg-config"
	rm -f $(PKG_CONFIG)

distclean: clean
	@echo "  DISTCLEAN complete"

# ============================================================================
# Help
# ============================================================================

help:
	@echo "========================================="
	@echo "  libcontain Build System v$(VERSION)"
	@echo "========================================="
	@echo ""
	@echo "Building:"
	@echo "  make              - Build static and shared libraries"
	@echo "  make release      - Build with optimizations (default)"
	@echo "  make debug        - Build with debug symbols"
	@echo "  make sanitize     - Build with AddressSanitizer"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make distclean    - Remove all generated files"
	@echo ""
	@echo "Tour:"
	@echo "  make tour         - Build tour program"
	@echo "  make run-tour     - Build and run tour"
	@echo ""
	@echo "Testing:"
	@echo "  make test         - Run all tests and tour"
	@echo "  make test-vector  - Run vector tests"
	@echo "  make test-deque   - Run deque tests"
	@echo "  make test-linkedlist - Run linkedlist tests"
	@echo "  make test-hashset - Run hashset tests"
	@echo "  make test-hashmap - Run hashmap tests"
	@echo "  make test-iterator - Run iterator tests"
	@echo "  make test-chainer - Run chainer tests"
	@echo "  make test-chainer2 - Run chainer2 tests"
	@echo "  make test-typed   - Run typed wrapper tests"
	@echo "  make test-allocator - Run allocator tests"
	@echo "  make coverage     - Generate coverage report"
	@echo ""
	@echo "Benchmarks:"
	@echo "  make bench        - Run all benchmarks"
	@echo "  make bench-vector - Run vector benchmarks"
	@echo "  make bench-deque  - Run deque benchmarks"
	@echo "  make bench-linkedlist - Run linkedlist benchmarks"
	@echo "  make bench-hashset - Run hashset benchmarks"
	@echo "  make bench-hashmap - Run hashmap benchmarks"
	@echo "  make bench-pipeline - Run pipeline benchmarks"
	@echo "  make bench-pipeline2 - Run pipeline2 benchmarks"
	@echo "  make bench-typed-vs-generic - Compare typed vs generic"
	@echo "  make bench-vs-uthash - Compare with uthash"
	@echo "  make bench-vs-klib - Compare with klib"
	@echo "  make bench-vs-stb-ds - Compare with stb_ds"
	@echo "  make bench-vs-cpp - Compare with C++"
	@echo ""
	@echo "Installation:"
	@echo "  make install      - Install library and headers"
	@echo "  make uninstall    - Remove installed files"
	@echo "  make install-pc   - Install pkg-config file"
	@echo ""
	@echo "Code Quality:"
	@echo "  make format       - Format code with clang-format"
	@echo "  make check        - Run static analysis with clang-tidy"