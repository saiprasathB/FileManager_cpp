# Smart File Explorer: build, GoogleTest, gcov and Cppcheck
# All application .cpp/.h files are in the project root.

CXX := g++
TARGET := explorer
BUILD_DIR := build

CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -g
CPPFLAGS := -I.

APP_SOURCES := main.cpp \
               FileExplorer.cpp \
               DirectoryManager.cpp \
               FileSystemItem.cpp \
               FileItem.cpp \
               FolderItem.cpp \
               ContextMenu.cpp \
               FileManager.cpp \
               ClipboardManager.cpp \
               SearchEngine.cpp \
               FileViewer.cpp \
               StorageAnalyzer.cpp

# Test files belong in tests/*.cpp
TEST_SOURCES := $(wildcard tests/*.cpp)

APP_OBJECTS := $(patsubst %.cpp,$(BUILD_DIR)/app/%.o,$(APP_SOURCES))
TEST_APP_SOURCES := $(filter-out main.cpp,$(APP_SOURCES))
TEST_APP_OBJECTS := $(patsubst %.cpp,$(BUILD_DIR)/test-app/%.o,$(TEST_APP_SOURCES))
TEST_OBJECTS := $(patsubst tests/%.cpp,$(BUILD_DIR)/tests/%.o,$(TEST_SOURCES))
TEST_BIN := $(BUILD_DIR)/explorer_tests

COV_DIR := $(BUILD_DIR)/coverage
COV_APP_OBJECTS := $(patsubst %.cpp,$(COV_DIR)/app/%.o,$(TEST_APP_SOURCES))
COV_TEST_OBJECTS := $(patsubst tests/%.cpp,$(COV_DIR)/tests/%.o,$(TEST_SOURCES))
COV_TEST_BIN := $(COV_DIR)/explorer_tests

.PHONY: all run test coverage cppcheck quality clean help check-tests

all: $(TARGET)

$(TARGET): $(APP_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@
	@echo "Build successful: $@"

$(BUILD_DIR)/app/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/test-app/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/tests/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

run: $(TARGET)
	./$(TARGET)

check-tests:
	@if [ -z "$(TEST_SOURCES)" ]; then \
		echo "No tests found. Add GoogleTest .cpp files under tests/."; \
		exit 1; \
	fi

test: check-tests $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_APP_OBJECTS) $(TEST_OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@ -lgtest_main -lgtest -pthread

# Separate instrumented build: --coverage enables gcov data generation.
$(COV_DIR)/app/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) -std=c++20 -Wall -Wextra -Wpedantic -O0 -g --coverage -MMD -MP -c $< -o $@

$(COV_DIR)/tests/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) -std=c++20 -Wall -Wextra -Wpedantic -O0 -g --coverage -MMD -MP -c $< -o $@

$(COV_TEST_BIN): $(COV_APP_OBJECTS) $(COV_TEST_OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) --coverage $^ -o $@ -lgtest_main -lgtest -pthread

coverage: check-tests $(COV_TEST_BIN)
	./$(COV_TEST_BIN)
	@echo "Generating gcov reports..."
	@find $(COV_DIR) -name '*.gcno' -print0 | while IFS= read -r -d '' file; do \
		gcov -b -c "$$file"; \
	done
	@echo "Coverage data and reports are under $(COV_DIR) (gcov may place .gcov files in the current directory)."

cppcheck:
	cppcheck --enable=warning,style,performance,portability \
	         --inline-suppr --std=c++20 --language=c++ \
	         --suppress=missingIncludeSystem \
	         $(APP_SOURCES) $(wildcard *.h) $(TEST_SOURCES)

quality: test coverage cppcheck

clean:
	rm -rf $(BUILD_DIR) $(TARGET) *.gcda *.gcno *.gcov

-include $(APP_OBJECTS:.o=.d)
-include $(TEST_APP_OBJECTS:.o=.d)
-include $(TEST_OBJECTS:.o=.d)
-include $(COV_APP_OBJECTS:.o=.d)
-include $(COV_TEST_OBJECTS:.o=.d)

help:
	@echo "make          Build application"
	@echo "make run      Build and run application"
	@echo "make test     Build and run GoogleTest tests"
	@echo "make coverage Run tests with gcov instrumentation"
	@echo "make cppcheck Run Cppcheck static analysis"
	@echo "make quality  Run tests, coverage and Cppcheck"
	@echo "make clean    Remove generated files"
