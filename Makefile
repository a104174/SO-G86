# Document Indexing System Makefile
# Version: 1.0

# Compiler settings
CC = gcc
CFLAGS_DEBUG = -Wall -Wextra -g -I$(INCLUDE_DIR)
CFLAGS_RELEASE = -Wall -Wextra -O2 -I$(INCLUDE_DIR)
LDFLAGS = 
BUILD ?= debug

# Directory structure
OBJ_DIR = obj
INCLUDE_DIR = include
BIN_DIR = bin
LOGS_DIR = logs
SRC_DIR = src
TMP_DIR = tmp
DATA_DIR = data
TEST_DIR = $(DATA_DIR)/test

# Targets
TARGETS = $(BIN_DIR)/dserver $(BIN_DIR)/dclient

# Conditional build flags
ifeq ($(BUILD),release)
CFLAGS = $(CFLAGS_RELEASE)
else
CFLAGS = $(CFLAGS_DEBUG)
endif

# Default target
all: folders $(TARGETS)
	@echo "Build completed successfully!"

# Create directory structure
folders:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR) $(LOGS_DIR) $(INCLUDE_DIR) \
		$(TMP_DIR) $(DATA_DIR) $(TEST_DIR)
	@echo "Directory structure created:"
	@echo "  - $(OBJ_DIR) for object files"
	@echo "  - $(BIN_DIR) for executables"
	@echo "  - $(DATA_DIR) for document storage"
	@echo "  - $(TMP_DIR) for FIFOs and temporary files"

# Server compilation
$(BIN)/dserver: $(OBJ)/dserver.o $(OBJ)/index.o
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "Server built: $@"

# Client compilation
$(BIN_DIR)/dclient: $(OBJ_DIR)/dclient.o $(OBJ_DIR)/common.o
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "Client built: $@"

# Object files
$(OBJ_DIR)/dserver.o: $(SRC_DIR)/dserver.c $(INCLUDE_DIR)/server.h $(INCLUDE_DIR)/common.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/dclient.o: $(SRC_DIR)/dclient.c $(INCLUDE_DIR)/client.h $(INCLUDE_DIR)/common.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/index.o: $(SRC_DIR)/index.c $(INCLUDE_DIR)/index.h $(INCLUDE_DIR)/common.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/common.o: $(SRC_DIR)/common.c $(INCLUDE_DIR)/common.h
	$(CC) $(CFLAGS) -c $< -o $@

# Test data setup
testdata:
	@echo "Creating test documents..."
	@echo "Sample document content for testing" > $(TEST_DIR)/doc1.txt
	@echo "Another document with keywords: linux makefile fifo" > $(TEST_DIR)/doc2.txt
	@echo "Test document with year 2023" > $(TEST_DIR)/doc3.txt
	@echo "Test data created in $(TEST_DIR)"

# Run server with test data
runserver: $(BIN_DIR)/dserver testdata
	@echo "Starting server with test documents..."
	@$(BIN_DIR)/dserver $(TEST_DIR) 10  # 10 = cache size

# Cleanup targets
clean:
	@rm -rf $(OBJ_DIR)/*.o
	@echo "Object files removed"

distclean: clean
	@rm -rf $(BIN_DIR)/* $(TMP_DIR)/* $(LOGS_DIR)/*
	@echo "Binaries and temporary files removed"

mrproper: distclean
	@rm -rf $(DATA_DIR)/*
	@echo "All generated files and data removed"

# Help target
help:
	@echo "Document Indexing System Build System"
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build all targets (default)"
	@echo "  debug      - Build with debug symbols (default)"
	@echo "  release    - Build with optimizations"
	@echo "  testdata   - Create test dataset"
	@echo "  runserver  - Run server with test data"
	@echo "  clean      - Remove object files"
	@echo "  distclean  - Remove all built files"
	@echo "  mrproper   - Remove everything including data"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Build options:"
	@echo "  BUILD=debug|release  - Set build type"

# Phony targets
.PHONY: all folders clean distclean mrproper testdata runserver help
