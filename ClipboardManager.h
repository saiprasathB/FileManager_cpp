
#ifndef CLIPBOARD_MANAGER_H
#define CLIPBOARD_MANAGER_H

#include <filesystem>

class FileManager;

class ClipboardManager {

private:

    std::filesystem::path selectedPath;

    bool hasItem;
    bool isCut;

public:

    // Constructor
    ClipboardManager();

    // Copy selected item
    void copy(const std::filesystem::path& path);

    // Cut selected item
    void cut(const std::filesystem::path& path);

    // Paste item into destination
    bool paste(const std::filesystem::path& destination,
               FileManager& fileManager);

    // Check clipboard status
    bool isEmpty() const;

    // Clear clipboard
    void clear();

};

#endif
