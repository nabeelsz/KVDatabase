# Command overview (run from the repository root):
#   make                      Build server: debug symbols and logging on
#   make DEBUG=0              Build server: debug symbols on, logging off
#   make MODE=release         Build optimized server with logging off
#   make MODE=release DEBUG=1 Build optimized server with logging on
#   make tests                Compile all three test executables
#   make test                 Compile and run all three test executables
#   make hashmap_tests        Compile the hash map tests only
#   make database_tests       Compile the database concurrency/performance tests only
#   make client_tests         Compile the client request-handler tests only
#   make all                  Compile the server and all tests
#   make run PORT=8080        Build and start the server on port 8080
#   make clean                Remove generated build directories
#   make help                 Display the command overview
#
# Outputs: build/<mode>-logging<0|1>/
# "debug" means debug symbols on and optimization off.
# "release" means optimization on.
# "logging1" means DEBUG=1 (logging on); "logging0" means DEBUG=0 (logging off).
#
# Build and run one test (two separate commands, from the repository root):
#   make hashmap_tests
#   ./build/debug-logging1/hashmap_tests
#
# After "make tests", run any test individually:
#   ./build/debug-logging1/hashmap_tests
#   ./build/debug-logging1/database_tests
#   ./build/debug-logging1/client_tests
#
# Build and run a test with debug logging off:
#   make hashmap_tests DEBUG=0
#   ./build/debug-logging0/hashmap_tests
#
# Build and run an optimized test:
#   make hashmap_tests MODE=release
#   ./build/release-logging0/hashmap_tests
#
# Build and run the server, then connect from a second terminal:
#   make
#   ./build/debug-logging1/kv_server 8080
# In the second terminal:
#   printf 'PUT key value\n' | nc localhost 8080
#   printf 'GET key\n' | nc localhost 8080
#
# "make run PORT=8080" builds and starts the server in one command.
# "make test" builds and runs all tests in one command.
# Executables are inside build/, so "./hashmap_tests" at the root will not work.
# Assertions remain enabled in debug and release builds.
#
# GNU Make; run from Linux or WSL with g++ installed.
.DEFAULT_GOAL := server
SHELL := /bin/sh
.DELETE_ON_ERROR:

MODE ?= debug
ifeq ($(MODE),debug)
MODE_FLAGS := -O0 -g3
DEBUG ?= 1
else ifeq ($(MODE),release)
MODE_FLAGS := -O2
DEBUG ?= 0
else
$(error MODE must be debug or release)
endif

ifneq ($(DEBUG),0)
ifneq ($(DEBUG),1)
$(error DEBUG must be 0 or 1)
endif
endif

# Keep assertions enabled in both modes for the assert-based tests.
BUILD_DIR := build/$(MODE)-logging$(DEBUG)
PROJECT_CPPFLAGS := -Isrc -Itests -DDEBUG=$(DEBUG)
# Existing sources use GNU extensions, including arithmetic on void pointers.
PROJECT_CXXFLAGS := -std=gnu++17 -Wall -Wextra -Werror=return-type -Wno-write-strings -pthread $(MODE_FLAGS)
PROJECT_LDLIBS := -pthread
CORE_OBJECTS := $(BUILD_DIR)/src/database.o $(BUILD_DIR)/src/hashmap.o
SERVER := $(BUILD_DIR)/kv_server
TEST_BINS := $(addprefix $(BUILD_DIR)/,hashmap_tests database_tests client_tests)
PORT ?= 8080

.PHONY: all server tests test run clean help hashmap_tests database_tests client_tests
all: server tests
server: $(SERVER)
tests: $(TEST_BINS)
hashmap_tests: $(BUILD_DIR)/hashmap_tests
database_tests: $(BUILD_DIR)/database_tests
client_tests: $(BUILD_DIR)/client_tests

$(SERVER): $(BUILD_DIR)/src/client.o $(CORE_OBJECTS)
	$(CXX) $(LDFLAGS) $^ $(PROJECT_LDLIBS) $(LDLIBS) -o $@

$(BUILD_DIR)/hashmap_tests: $(BUILD_DIR)/tests/hashmap_tests.o $(BUILD_DIR)/src/hashmap.o
	$(CXX) $(LDFLAGS) $^ $(PROJECT_LDLIBS) $(LDLIBS) -o $@

$(BUILD_DIR)/database_tests: $(BUILD_DIR)/tests/database_tests.o $(CORE_OBJECTS)
	$(CXX) $(LDFLAGS) $^ $(PROJECT_LDLIBS) $(LDLIBS) -o $@

$(BUILD_DIR)/client_tests: $(BUILD_DIR)/client_test_main.o $(CORE_OBJECTS)
	$(CXX) $(LDFLAGS) $^ $(PROJECT_LDLIBS) $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: %.cpp Makefile
	mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(PROJECT_CPPFLAGS) $(CXXFLAGS) $(PROJECT_CXXFLAGS) -MMD -MP -c $< -o $@

# client_tests.cpp includes client.cpp and has its own main commented out.
# Rename the included server main and call the existing test function.
$(BUILD_DIR)/client_test_main.cpp: Makefile
	mkdir -p $(@D)
	printf '%s\n' '#define main kv_server_main' '#include "client_tests.cpp"' '#undef main' 'int main() { req_handler_tests(); return 0; }' > $@

$(BUILD_DIR)/client_test_main.o: $(BUILD_DIR)/client_test_main.cpp Makefile
	$(CXX) $(CPPFLAGS) $(PROJECT_CPPFLAGS) $(CXXFLAGS) $(PROJECT_CXXFLAGS) -MMD -MP -c $< -o $@

test: tests
	@set -e; for test_bin in $(TEST_BINS); do \
		printf '\nRunning %s\n' "$$test_bin"; \
		"$$test_bin"; \
	done

run: server
	./$(SERVER) $(PORT)

# Only remove artifacts owned by this Makefile.
clean:
	rm -rf build/debug-logging0 build/debug-logging1 build/release-logging0 build/release-logging1

help:
	@printf '%s\n' \
		'make                         Build the server (debug, logging on)' \
		'make tests                   Build all test executables' \
		'make test                    Build and run all tests' \
		'make all                     Build the server and all tests' \
		'make run PORT=8080           Build and run the server' \
		'make MODE=release            Build optimized, logging off' \
		'make DEBUG=0                 Build debug, logging off' \
		'make MODE=release DEBUG=1    Build optimized, logging on' \
		'make hashmap_tests           Build an individual test executable' \
		'make clean                   Remove generated build directories' \
		'Outputs: build/<mode>-logging<0|1>/' \
		'Assertions remain enabled in both modes.'

-include $(wildcard $(BUILD_DIR)/src/*.d $(BUILD_DIR)/tests/*.d $(BUILD_DIR)/*.d)
