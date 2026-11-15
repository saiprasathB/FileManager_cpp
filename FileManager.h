
#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <filesystem>
#include <string>

class FileManager {
private:
    std::filesystem::path workspacePath;

    bool isWithinWorkspace(const std::filesystem::path& path) const;
    bool isValidName(const std::string& name) const;

public:
    FileManager(const std::filesystem::path& workspace);

    bool createFile(const std::filesystem::path& directory,
                    const std::string& fileName);

    bool createFolder(const std::filesystem::path& directory,
                      const std::string& folderName);

    bool renameItem(const std::filesystem::path& itemPath,
                    const std::string& newName);

    bool deleteItem(const std::filesystem::path& itemPath);

    bool copyItem(const std::filesystem::path& source,
                  const std::filesystem::path& destination);

    bool moveItem(const std::filesystem::path& source,
                  const std::filesystem::path& destination);

    void showProperties(const std::filesystem::path& itemPath);

    // New methods
    bool writeFile(const std::filesystem::path& filePath,
                   const std::string& content);

    bool appendFile(const std::filesystem::path& filePath,
                    const std::string& content);
};

#endif
