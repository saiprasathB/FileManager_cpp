
#ifndef FILE_ITEM_H
#define FILE_ITEM_H

#include "FileSystemItem.h"

class FileItem : public FileSystemItem {

public:

    // Constructor
    FileItem(const std::filesystem::path& path);

    // Get file type
    std::string getType() const override;

    // Get file size
    uintmax_t getSize() const;

    // Check file extension
    std::string getExtension() const;

};

#endif
