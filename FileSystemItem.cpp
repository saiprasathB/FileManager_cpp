
#include "FileSystemItem.h"

using namespace std;
namespace fs = std::filesystem;


// Constructor
FileSystemItem::FileSystemItem(const fs::path& path) {

    itemPath = path;
}


// Destructor
FileSystemItem::~FileSystemItem() = default;


// Get item name
string FileSystemItem::getName() const {

    return itemPath.filename().string();
}


// Get item path
const fs::path& FileSystemItem::getPath() const {

    return itemPath;
}
