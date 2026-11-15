
#ifndef FILE_SYSTEM_ITEM_H
#define FILE_SYSTEM_ITEM_H

#include <filesystem>
#include <string>

class FileSystemItem {

protected:

    std::filesystem::path itemPath;

public:

    // Constructor
    FileSystemItem(const std::filesystem::path& path);

    // Virtual destructor
    virtual ~FileSystemItem();

    // Get item name
    std::string getName() const;

    // Get item path
    const std::filesystem::path& getPath() const;

    // Get item type
    virtual std::string getType() const = 0;

};

#endif
