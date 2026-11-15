
#include "FileManager.h"

#include <iostream>
#include <fstream>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;


// Constructor
FileManager::FileManager(const fs::path& workspace) {

    workspacePath = fs::canonical(workspace);
}


// Check whether path belongs to workspace
bool FileManager::isWithinWorkspace(const fs::path& path) const {

    try {

        fs::path root = fs::canonical(workspacePath);
        fs::path target = fs::canonical(path);

        auto rootIterator = root.begin();
        auto targetIterator = target.begin();

        for (; rootIterator != root.end();
             ++rootIterator, ++targetIterator) {

            if (targetIterator == target.end() ||
                *rootIterator != *targetIterator) {

                return false;
            }
        }

        return true;

    }
    catch (const fs::filesystem_error&) {

        return false;
    }
}


// Validate file or folder name
bool FileManager::isValidName(const string& name) const {

    if (name.empty() || name == "." || name == "..") {
        return false;
    }

    if (name.find('/') != string::npos ||
        name.find('\\') != string::npos) {

        return false;
    }

    return true;
}


// Create file
bool FileManager::createFile(const fs::path& directory,
                             const string& fileName) {

    try {

        if (!isValidName(fileName)) {

            cout << "Invalid file name.\n";
            return false;
        }

        if (!fs::is_directory(directory) ||
            !isWithinWorkspace(directory)) {

            cout << "Invalid destination directory.\n";
            return false;
        }

        fs::path filePath = fs::canonical(directory) / fileName;

        if (fs::exists(filePath)) {

            cout << "An item with this name already exists.\n";
            return false;
        }

        ofstream file(filePath);

        if (!file) {

            cout << "Unable to create file.\n";
            return false;
        }

        file.close();

        cout << "File created successfully.\n";

        return true;

    }
    catch (const fs::filesystem_error& e) {

        cerr << "File creation error: " << e.what() << '\n';
        return false;
    }
}


// Create folder
bool FileManager::createFolder(const fs::path& directory,
                               const string& folderName) {

    try {

        if (!isValidName(folderName)) {

            cout << "Invalid folder name.\n";
            return false;
        }

        if (!fs::is_directory(directory) ||
            !isWithinWorkspace(directory)) {

            cout << "Invalid destination directory.\n";
            return false;
        }

        fs::path folderPath = fs::canonical(directory) / folderName;

        if (fs::exists(folderPath)) {

            cout << "An item with this name already exists.\n";
            return false;
        }

        if (fs::create_directory(folderPath)) {

            cout << "Folder created successfully.\n";
            return true;
        }

        return false;

    }
    catch (const fs::filesystem_error& e) {

        cerr << "Folder creation error: " << e.what() << '\n';
        return false;
    }
}


// Rename file or folder
bool FileManager::renameItem(const fs::path& itemPath,
                             const string& newName) {

    try {

        if (!isValidName(newName)) {

            cout << "Invalid name.\n";
            return false;
        }

        if (!fs::exists(itemPath) ||
            fs::is_symlink(itemPath) ||
            !isWithinWorkspace(itemPath)) {

            cout << "Invalid item.\n";
            return false;
        }

        fs::path source = fs::canonical(itemPath);

        if (source == workspacePath) {

            cout << "Cannot rename workspace root.\n";
            return false;
        }

        fs::path newPath = source.parent_path() / newName;

        if (fs::exists(newPath)) {

            cout << "An item with this name already exists.\n";
            return false;
        }

        fs::rename(source, newPath);

        cout << "Item renamed successfully.\n";

        return true;

    }
    catch (const fs::filesystem_error& e) {

        cerr << "Rename error: " << e.what() << '\n';
        return false;
    }
}


// Delete file or folder
bool FileManager::deleteItem(const fs::path& itemPath) {

    try {

        if (!fs::exists(itemPath) ||
            fs::is_symlink(itemPath) ||
            !isWithinWorkspace(itemPath)) {

            cout << "Invalid item.\n";
            return false;
        }

        fs::path target = fs::canonical(itemPath);

        if (target == workspacePath) {

            cout << "Cannot delete workspace root.\n";
            return false;
        }

        fs::remove_all(target);

        cout << "Item deleted successfully.\n";

        return true;

    }
    catch (const fs::filesystem_error& e) {

        cerr << "Delete error: " << e.what() << '\n';
        return false;
    }
}


// Copy file or folder
bool FileManager::copyItem(const fs::path& source,
                           const fs::path& destination) {

    try {

        if (!fs::exists(source) ||
            fs::is_symlink(source) ||
            !isWithinWorkspace(source)) {

            cout << "Invalid source item.\n";
            return false;
        }

        if (!fs::is_directory(destination) ||
            !isWithinWorkspace(destination)) {

            cout << "Invalid destination directory.\n";
            return false;
        }

        fs::path sourcePath = fs::canonical(source);
        fs::path destinationPath = fs::canonical(destination);

        if (sourcePath == workspacePath) {

            cout << "Cannot copy workspace root.\n";
            return false;
        }

        if (fs::is_directory(sourcePath)) {

            // Prevent copying a folder into itself
            if (isWithinWorkspace(destinationPath) &&
                fs::relative(destinationPath, sourcePath).begin()
                    != fs::relative(destinationPath, sourcePath).end()) {

                cout << "Cannot copy a folder into itself or its subfolder.\n";
                return false;
            }
        }

        fs::path target = destinationPath / sourcePath.filename();

        if (fs::exists(target)) {

            cout << "Destination already contains an item with this name.\n";
            return false;
        }

        fs::copy(
            sourcePath,
            target,
            fs::copy_options::recursive |
            fs::copy_options::copy_symlinks
        );

        cout << "Item copied successfully.\n";

        return true;

    }
    catch (const fs::filesystem_error& e) {

        cerr << "Copy error: " << e.what() << '\n';
        return false;
    }
}


// Move file or folder
bool FileManager::moveItem(const fs::path& source,
                           const fs::path& destination) {

    try {

        if (!fs::exists(source) ||
            fs::is_symlink(source) ||
            !isWithinWorkspace(source)) {

            cout << "Invalid source item.\n";
            return false;
        }

        if (!fs::is_directory(destination) ||
            !isWithinWorkspace(destination)) {

            cout << "Invalid destination directory.\n";
            return false;
        }

        fs::path sourcePath = fs::canonical(source);
        fs::path destinationPath = fs::canonical(destination);

        if (sourcePath == workspacePath) {

            cout << "Cannot move workspace root.\n";
            return false;
        }

        if (fs::is_directory(sourcePath) &&
            fs::relative(destinationPath, sourcePath).begin()
                != fs::relative(destinationPath, sourcePath).end()) {

            cout << "Cannot move a folder into itself or its subfolder.\n";
            return false;
        }

        fs::path target = destinationPath / sourcePath.filename();

        if (fs::exists(target)) {

            cout << "Destination already contains an item with this name.\n";
            return false;
        }

        fs::rename(sourcePath, target);

        cout << "Item moved successfully.\n";

        return true;

    }
    catch (const fs::filesystem_error& e) {

        cerr << "Move error: " << e.what() << '\n';
        return false;
    }
}



// Display item properties
void FileManager::showProperties(const fs::path& itemPath) {

    try {

        if (!fs::exists(itemPath) ||
            fs::is_symlink(itemPath) ||
            !isWithinWorkspace(itemPath)) {

            cout << "Invalid item.\n";
            return;
        }

        cout << "\n========== PROPERTIES ==========\n";

        cout << "Name: "
             << itemPath.filename().string() << '\n';

        cout << "Path: "
             << fs::canonical(itemPath).string() << '\n';

        if (fs::is_directory(itemPath)) {

            cout << "Type: File Folder\n";

            size_t count = 0;

            for (const auto& entry : fs::directory_iterator(itemPath)) {
                count++;
            }

            cout << "Items inside: " << count << '\n';
        }
        else {

            cout << "Type: File\n";

            cout << "Extension: "
                 << itemPath.extension().string() << '\n';

            cout << "Size: "
                 << fs::file_size(itemPath) << " bytes\n";
        }

        cout << "================================\n";

    }
    catch (const fs::filesystem_error& e) {

        cerr << "Properties error: " << e.what() << '\n';
    }
}




bool FileManager::writeFile(const fs::path& filePath,
                            const string& content) {

    if (!fs::exists(filePath) ||
        fs::is_symlink(fs::symlink_status(filePath)) ||
        !fs::is_regular_file(filePath) ||
        !isWithinWorkspace(filePath)) {

        cout << "Invalid file path!" << endl;
        return false;
    }

    fs::path safePath = fs::canonical(filePath);

    ofstream file(safePath, ios::out | ios::trunc);

    if (!file.is_open()) {
        cout << "Unable to open file!" << endl;
        return false;
    }

    file << content;
    file.close();

    cout << "File content saved successfully!" << endl;
    return true;
}


bool FileManager::appendFile(const fs::path& filePath,
                             const string& content) {

    if (!fs::exists(filePath) ||
        fs::is_symlink(fs::symlink_status(filePath)) ||
        !fs::is_regular_file(filePath) ||
        !isWithinWorkspace(filePath)) {

        cout << "Invalid file path!" << endl;
        return false;
    }

    fs::path safePath = fs::canonical(filePath);

    ofstream file(safePath, ios::out | ios::app);

    if (!file.is_open()) {
        cout << "Unable to open file!" << endl;
        return false;
    }

    file << content;
    file.close();

    cout << "Content appended successfully!" << endl;
    return true;
}



