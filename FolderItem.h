
#ifndef FOLDER_ITEM_H
#define FOLDER_ITEM_H

#include "FileSystemItem.h"

class FolderItem : public FileSystemItem {

public:

    // Constructor
    FolderItem(const std::filesystem::path& path);

    // Get folder type
    std::string getType() const override;

    // Get number of items inside folder
    size_t getItemCount() const;

    // Check whether folder is empty
    bool isEmpty() const;

};

#endif
