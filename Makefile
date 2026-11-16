# ==========================================
# Smart File Explorer
# C++20 Makefile
# All source and header files are in one folder
# ==========================================

CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -g

TARGET = explorer
BUILD_DIR = build

SOURCES = main.cpp \
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

OBJECTS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
DEPS = $(OBJECTS:.o=.d)

.PHONY: all run debug clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@
	@echo "Build successful: $(TARGET)"

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

run: $(TARGET)
	./$(TARGET)

debug: CXXFLAGS += -O0 -g3
debug: clean all

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

-include $(DEPS)
