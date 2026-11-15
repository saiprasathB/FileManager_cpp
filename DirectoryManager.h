
#ifndef DIRECTORY_MANAGER_H
#define DIRECTORY_MANAGER_H

#include <filesystem>
#include <vector>

class DirectoryManager {

private:

    std::filesystem::path workspacePath;
    std::filesystem::path currentPath;

    std::vector<std::filesystem::directory_entry> entries;

    bool isWithinWorkspace(const std::filesystem::path& path) const;

public:

    // Constructor
    DirectoryManager();

    // Initialize workspace
    bool initialize();

    // Get directory paths
    const std::filesystem::path& getWorkspacePath() const;
    const std::filesystem::path& getCurrentPath() const;

    // Get directory entries
    const std::vector<std::filesystem::directory_entry>& getEntries() const;

    // Directory operations
    bool refresh();
    bool openDirectory(int index);
    bool goBack();
    void goHome();

};

#endif
