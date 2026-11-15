
#include "ClipboardManager.h"
#include "FileManager.h"

#include <iostream>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;


// Constructor
ClipboardManager::ClipboardManager() {

    hasItem = false;
    isCut = false;
}


// Copy selected item
void ClipboardManager::copy(const fs::path& path) {

    selectedPath = path;

    hasItem = true;
    isCut = false;

    cout << "Item copied to clipboard.\n";
}


// Cut selected item
void ClipboardManager::cut(const fs::path& path) {

    selectedPath = path;

    hasItem = true;
    isCut = true;

    cout << "Item selected for moving.\n";
}


// Paste item
bool ClipboardManager::paste(const fs::path& destination,
                             FileManager& fileManager) {

    if (!hasItem) {

        cout << "Clipboard is empty.\n";

        return false;
    }

    bool success;

    if (isCut) {

        success = fileManager.moveItem(selectedPath, destination);
    }
    else {

        success = fileManager.copyItem(selectedPath, destination);
    }

    if (success) {

        clear();

    }

    return success;
}


// Check clipboard status
bool ClipboardManager::isEmpty() const {

    return !hasItem;
}


// Clear clipboard
void ClipboardManager::clear() {

    selectedPath.clear();

    hasItem = false;
    isCut = false;
}
