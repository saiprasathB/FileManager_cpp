
#ifndef STORAGE_ANALYZER_H
#define STORAGE_ANALYZER_H

#include <filesystem>
#include <cstddef>
#include <cstdint>

struct StorageInfo {

    size_t totalFiles = 0;

    size_t totalFolders = 0;

    uintmax_t totalBytes = 0;

};

class StorageAnalyzer {

private:

    std::filesystem::path workspacePath;

public:

    // Constructor
    StorageAnalyzer(const std::filesystem::path& workspace);

    // Analyze workspace storage
    StorageInfo analyze() const;

    // Display storage information
    void displayStorageInfo() const;

};

#endif
