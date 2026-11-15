

#include "FileItem.h"

#include <filesystem>

using namespace std;
namespace fs = std::filesystem;


// Constructor
FileItem::FileItem(const fs::path& path)
    : FileSystemItem(path) {

}


// Get file type
string FileItem::getType() const {

    return "File";
}


// Get file size
uintmax_t FileItem::getSize() const {

    return fs::file_size(itemPath);
}


// Get file extension
string FileItem::getExtension() const {

    return itemPath.extension().string();
}
