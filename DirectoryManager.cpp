
#include "DirectoryManager.h"

#include <algorithm>
#include <iostream>

using namespace std;
namespace fs = std::filesystem;


// Constructor
DirectoryManager::DirectoryManager() {

    workspacePath = fs::current_path() / "workspace";

    currentPath = workspacePath;
}


// Initialize workspace
bool DirectoryManager::initialize() {

    try {

        if (!fs::exists(workspacePath)) {
            fs::create_directories(workspacePath);
        }

        workspacePath = fs::canonical(workspacePath);

        currentPath = workspacePath;

        refresh();

        return true;

    }
    catch (const fs::filesystem_error& e) {

        cerr << "Initialization error: " << e.what() << '\n';

        return false;
    }
}


// Get workspace path
const fs::path& DirectoryManager::getWorkspacePath() const {

    return workspacePath;
}


// Get current path
const fs::path& DirectoryManager::getCurrentPath() const {

    return currentPath;
}


// Get directory entries
const vector<fs::directory_entry>& DirectoryManager::getEntries() const {

    return entries;
}


// Check whether a path is inside workspace
bool DirectoryManager::isWithinWorkspace(const fs::path& path) const {

    auto root = workspacePath.begin();
    auto target = path.begin();

    for (; root != workspacePath.end(); ++root, ++target) {

        if (target == path.end() || *root != *target) {
            return false;
        }
    }

    return true;
}


// Refresh directory listing
bool DirectoryManager::refresh() {

    try {

        entries.clear();

        fs::directory_iterator iterator(
            currentPath,
            fs::directory_options::skip_permission_denied
        );

        for (const auto& entry : iterator) {

            entries.push_back(entry);
        }

        // Sort entries alphabetically
        sort(entries.begin(), entries.end(),
             [](const fs::directory_entry& a,
                const fs::directory_entry& b) {

                 return a.path().filename().string() <
                        b.path().filename().string();
             });

        return true;

    }
    catch (const fs::filesystem_error& e) {

        cerr << "Refresh error: " << e.what() << '\n';

        return false;
    }
}


// Open selected directory
bool DirectoryManager::openDirectory(int index) {

    if (index < 1 || index > static_cast<int>(entries.size())) {

        cout << "Invalid selection.\n";

        return false;
    }

    try {

        fs::path selectedPath = entries[index - 1].path();

        if (!fs::is_directory(selectedPath)) {

            cout << "Selected item is not a folder.\n";

            return false;
        }

        fs::path newPath = fs::canonical(selectedPath);

        if (!isWithinWorkspace(newPath)) {

            cout << "Access outside workspace is not allowed.\n";

            return false;
        }

        currentPath = newPath;

        refresh();

        return true;

    }
    catch (const fs::filesystem_error& e) {

        cerr << "Navigation error: " << e.what() << '\n';

        return false;
    }
}


// Go to parent directory
bool DirectoryManager::goBack() {

    if (currentPath == workspacePath) {

        cout << "Already in workspace root.\n";

        return false;
    }

    currentPath = currentPath.parent_path();

    refresh();

    return true;
}


// Return to workspace root
void DirectoryManager::goHome() {

    currentPath = workspacePath;

    refresh();
}
