
#include "FolderItem.h"
#include <iostream>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;


// Constructor
FolderItem::FolderItem(const fs::path& path)
    : FileSystemItem(path) {

}


// Get folder type
string FolderItem::getType() const {

    return "Folder";
}


// Get number of items inside folder
size_t FolderItem::getItemCount() const {

    size_t count = 0;

    try {

        for (const auto& entry : fs::directory_iterator(itemPath)) {

            count++;
        }

    }
    catch (const fs::filesystem_error& e) {

        cerr << "Folder access error: " << e.what() << '\n';
    }

    return count;
}


// Check whether folder is empty
bool FolderItem::isEmpty() const {

    return getItemCount() == 0;
}
