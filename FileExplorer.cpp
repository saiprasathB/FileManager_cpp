
#include "FileExplorer.h"

#include <iomanip>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <limits>

using namespace std;
namespace fs = std::filesystem;


// Read multiline content until END is entered
string readMultilineContent() {

    string content;
    string line;

    cout << "\nEnter content (type END on a new line to save):\n";

    while (getline(cin, line)) {

        if (line == "END") {
            break;
        }

        content += line + '\n';
    }

    return content;
}


// Constructor
FileExplorer::FileExplorer() {
    running = true;
}


// Initialize application
void FileExplorer::initialize() {

    if (!directoryManager.initialize()) {
        running = false;
        return;
    }

    const fs::path& workspace =
        directoryManager.getWorkspacePath();

    fileManager = make_unique<FileManager>(workspace);
    searchEngine = make_unique<SearchEngine>(workspace);
    storageAnalyzer = make_unique<StorageAnalyzer>(workspace);

    cout << "\nSmart File Explorer initialized!\n";
    cout << "Workspace: " << workspace.string() << '\n';
}



void FileExplorer::displayHeader() {

    cout << "\n";
    cout << "============================================\n";
    cout << "             SMART FILE EXPLORER             \n";
    cout << "============================================\n";

    cout << "Current Location: "
         << directoryManager.getCurrentPath().string() << '\n';

    cout << "--------------------------------------------\n";
}

void FileExplorer::displayDirectory() {

    const auto& entries = directoryManager.getEntries();

    int totalFiles = 0;
    int totalFolders = 0;
    int index = 1;

    cout << "\n+--------------------------------------------------+\n";
    cout << "|                  FILES AND FOLDERS               |\n";
    cout << "+------+----------------------+----------+--------+\n";
    cout << "| No.  | Name                 | Type     | Size   |\n";
    cout << "+------+----------------------+----------+--------+\n";

    for (const auto& entry : entries) {

        string name = entry.path().filename().string();
        string type = "File";
        string size = "-";

        try {

            if (entry.is_symlink()) {
                type = "Link";
            }
            else if (entry.is_directory()) {
                type = "Folder";
                totalFolders++;
            }
            else if (entry.is_regular_file()) {

                totalFiles++;

                string extension = entry.path().extension().string();

                if (!extension.empty()) {
                    extension.erase(0, 1);

                    transform(extension.begin(), extension.end(),
                              extension.begin(),
                              [](unsigned char c) {
                                  return static_cast<char>(toupper(c));
                              });

                    type = extension + " File";
                }

                error_code ec;
                uintmax_t fileSize = entry.file_size(ec);

                if (!ec) {
                    size = to_string(fileSize) + " B";
                }
                else {
                    size = "N/A";
                }
            }

        }
        catch (const fs::filesystem_error&) {
            type = "Unknown";
            size = "N/A";
        }

        cout << "| "
             << left << setw(4) << index++
             << " | "
             << setw(20) << name
             << " | "
             << setw(8) << type
             << " | "
             << right << setw(6) << size
             << " |\n";
    }

    cout << "+------+----------------------+----------+--------+\n";

    if (entries.empty()) {
        cout << "|               This folder is empty.              |\n";
        cout << "+--------------------------------------------------+\n";
    }
    else {

        string summary = "Total: " + to_string(totalFiles) +
                         " Files, " + to_string(totalFolders) +
                         " Folders";

        cout << "| " << left << setw(48) << summary << " |\n";
        cout << "+--------------------------------------------------+\n";
    }
}




// Display explorer
void FileExplorer::display() {

    displayHeader();
    displayDirectory();
}


// Open selected file or folder
void FileExplorer::openItem(int index) {

    const auto& entries = directoryManager.getEntries();

    if (index < 1 || index > static_cast<int>(entries.size())) {
        cout << "Invalid selection.\n";
        return;
    }

    fs::path selectedPath = entries[index - 1].path();

    try {

        if (fs::is_symlink(selectedPath)) {
            cout << "Opening symbolic links is not allowed.\n";
            return;
        }

        if (fs::is_directory(selectedPath)) {
            directoryManager.openDirectory(index);
        }
        else if (fs::is_regular_file(selectedPath)) {
            fileViewer.openFile(selectedPath);
        }
        else {
            cout << "Unsupported item type.\n";
        }

    }
    catch (const fs::filesystem_error& e) {
        cerr << "Open error: " << e.what() << '\n';
    }
}


// Handle context menu selection
void FileExplorer::handleSelection(int index) {

    const auto& entries = directoryManager.getEntries();

    if (index < 1 || index > static_cast<int>(entries.size())) {
        cout << "Invalid selection.\n";
        return;
    }

    fs::path selectedPath = entries[index - 1].path();

    try {

        if (fs::is_symlink(selectedPath)) {
            cout << "Operations on symbolic links are not supported.\n";
            return;
        }

        bool isDirectory = fs::is_directory(selectedPath);

        ContextMenu::Action action = contextMenu.show(isDirectory);

        switch (action) {

            case ContextMenu::Action::Open:
                openItem(index);
                break;

            case ContextMenu::Action::Copy:
                clipboardManager.copy(selectedPath);
                cout << "Item copied to clipboard.\n";
                break;

            case ContextMenu::Action::Cut:
                clipboardManager.cut(selectedPath);
                cout << "Item cut to clipboard.\n";
                break;

            case ContextMenu::Action::Write:
                handleItemCommand("write", index);
                break;

            case ContextMenu::Action::Rename:
                handleItemCommand("rename", index);
                break;

            case ContextMenu::Action::Delete:
                handleItemCommand("delete", index);
                break;

            case ContextMenu::Action::Properties:
                fileManager->showProperties(selectedPath);
                break;

            case ContextMenu::Action::Cancel:
                cout << "Operation cancelled.\n";
                break;
        }

    }
    catch (const fs::filesystem_error& e) {
        cerr << "Selection error: " << e.what() << '\n';
    }
}


// Handle operations on a numbered item
void FileExplorer::handleItemCommand(const string& action, int index) {

    const auto& entries = directoryManager.getEntries();

    if (index < 1 || index > static_cast<int>(entries.size())) {
        cout << "Invalid selection.\n";
        return;
    }

    fs::path selectedPath = entries[index - 1].path();

    try {

        if (fs::is_symlink(selectedPath)) {
            cout << "Operations on symbolic links are not supported.\n";
            return;
        }

        if (action == "open") {
            openItem(index);
        }
        else if (action == "copy") {
            clipboardManager.copy(selectedPath);
        }
        else if (action == "cut") {
            clipboardManager.cut(selectedPath);
        }
        else if (action == "write" || action == "append") {

            if (!fs::is_regular_file(selectedPath)) {
                cout << "Writing is supported only for files.\n";
                return;
            }

            string content = readMultilineContent();

            if (action == "write") {
                fileManager->writeFile(selectedPath, content);
            }
            else {
                fileManager->appendFile(selectedPath, content);
            }
        }
        else if (action == "rename") {

            string newName;

            cout << "Enter new name: ";
            getline(cin, newName);

            if (fileManager->renameItem(selectedPath, newName)) {
                directoryManager.refresh();
            }
        }
        else if (action == "delete") {

            char confirmation;

            cout << "Are you sure you want to delete "
                 << selectedPath.filename().string()
                 << "? (y/n): ";

            cin >> confirmation;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            if (confirmation == 'y' || confirmation == 'Y') {

                if (fileManager->deleteItem(selectedPath)) {
                    directoryManager.refresh();
                }
            }
            else {
                cout << "Deletion cancelled.\n";
            }
        }
        else if (action == "properties") {
            fileManager->showProperties(selectedPath);
        }

    }
    catch (const fs::filesystem_error& e) {
        cerr << "Operation error: " << e.what() << '\n';
    }
}


// Display available commands

void FileExplorer::displayHelp() {

    cout << "\n";
    cout << "+--------------------------------------------------+\n";
    cout << "|                 AVAILABLE COMMANDS               |\n";
    cout << "+----------------------+---------------------------+\n";
    cout << "| Command              | Description               |\n";
    cout << "+----------------------+---------------------------+\n";

    cout << "| " << left << setw(20) << "open 2"
         << " | " << setw(25) << "Open selected item" << " |\n";

    cout << "| " << setw(20) << "copy 1"
         << " | " << setw(25) << "Copy item" << " |\n";

    cout << "| " << setw(20) << "cut 1"
         << " | " << setw(25) << "Cut item" << " |\n";

    cout << "| " << setw(20) << "paste"
         << " | " << setw(25) << "Paste copied/cut item" << " |\n";

    cout << "| " << setw(20) << "write 2"
         << " | " << setw(25) << "Write file content" << " |\n";

    cout << "| " << setw(20) << "append 2"
         << " | " << setw(25) << "Append file content" << " |\n";

    cout << "| " << setw(20) << "rename 2"
         << " | " << setw(25) << "Rename item" << " |\n";

    cout << "| " << setw(20) << "delete 2"
         << " | " << setw(25) << "Delete item" << " |\n";

    cout << "| " << setw(20) << "properties 1"
         << " | " << setw(25) << "Show item properties" << " |\n";

    cout << "| " << setw(20) << "create-file"
         << " | " << setw(25) << "Create new file" << " |\n";

    cout << "| " << setw(20) << "create-folder"
         << " | " << setw(25) << "Create new folder" << " |\n";

    cout << "| " << setw(20) << "search Notes"
         << " | " << setw(25) << "Search workspace" << " |\n";

    cout << "| " << setw(20) << "storage"
         << " | " << setw(25) << "Storage analysis" << " |\n";

    cout << "| " << setw(20) << "back"
         << " | " << setw(25) << "Parent directory" << " |\n";

    cout << "| " << setw(20) << "home"
         << " | " << setw(25) << "Workspace root" << " |\n";

    cout << "| " << setw(20) << "refresh"
         << " | " << setw(25) << "Refresh listing" << " |\n";

    cout << "| " << setw(20) << "exit"
         << " | " << setw(25) << "Exit application" << " |\n";

    cout << "+----------------------+---------------------------+\n";
}


// Handle user commands
void FileExplorer::handleCommand(const string& command) {

    istringstream input(command);

    string action;
    input >> action;

    if (action.empty()) {
        return;
    }

    // Check whether the user entered only a number
    bool isNumber = all_of(
        action.begin(),
        action.end(),
        [](unsigned char ch) {
            return isdigit(ch);
        }
    );

    if (isNumber) {
        handleSelection(stoi(action));
        return;
    }

    if (action == "back") {
        goBack();
    }
    else if (action == "home") {
        goHome();
    }
    else if (action == "refresh") {
        directoryManager.refresh();
    }
    else if (action == "exit") {
        exitApplication();
    }
    else if (action == "help") {
        displayHelp();
    }
    else if (action == "open" ||
             action == "copy" ||
             action == "cut" ||
             action == "write" ||
             action == "append" ||
             action == "rename" ||
             action == "delete" ||
             action == "properties") {

        int index;

        if (!(input >> index)) {
            cout << "Please provide an item number.\n";
            return;
        }

        handleItemCommand(action, index);
    }
    else if (action == "paste") {

        if (clipboardManager.isEmpty()) {
            cout << "Clipboard is empty.\n";
            return;
        }

        if (clipboardManager.paste(
                directoryManager.getCurrentPath(),
                *fileManager)) {

            directoryManager.refresh();
        }
    }
    else if (action == "create-file") {

        string fileName;

        cout << "Enter file name: ";
        getline(cin, fileName);

        if (fileManager->createFile(
                directoryManager.getCurrentPath(),
                fileName)) {

            directoryManager.refresh();
        }
    }
    else if (action == "create-folder") {

        string folderName;

        cout << "Enter folder name: ";
        getline(cin, folderName);

        if (fileManager->createFolder(
                directoryManager.getCurrentPath(),
                folderName)) {

            directoryManager.refresh();
        }
    }
    else if (action == "search") {

        string keyword;
        getline(input, keyword);

        size_t first = keyword.find_first_not_of(" \t");

        if (first == string::npos) {
            cout << "Enter a search keyword.\n";
            return;
        }

        keyword = keyword.substr(first);

        vector<fs::path> results = searchEngine->search(keyword);

        cout << "\n========== SEARCH RESULTS ==========\n";

        if (results.empty()) {
            cout << "No matching items found.\n";
        }
        else {

            int index = 1;

            for (const auto& path : results) {
                cout << index++ << ". "
                     << path.string() << '\n';
            }
        }

        cout << "====================================\n";
    }
    else if (action == "storage") {
        storageAnalyzer->displayStorageInfo();
    }
    else {
        cout << "Unknown command. Type help.\n";
    }
}


// Go to parent directory
void FileExplorer::goBack() {
    directoryManager.goBack();
}


// Return to workspace root
void FileExplorer::goHome() {
    directoryManager.goHome();
}


// Exit application
void FileExplorer::exitApplication() {

    running = false;

    cout << "\nExiting Smart File Explorer...\n";
}


// Main application loop
void FileExplorer::run() {

    initialize();

    while (running) {

        display();

        string command;

        cout << "\nExplorer> ";

        if (!getline(cin, command)) {
            exitApplication();
            break;
        }

        handleCommand(command);
    }
}
