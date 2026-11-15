
#ifndef FILE_EXPLORER_H
#define FILE_EXPLORER_H

#include <iostream>
#include <string>
#include <memory>

#include "DirectoryManager.h"
#include "ContextMenu.h"
#include "FileManager.h"
#include "ClipboardManager.h"
#include "SearchEngine.h"
#include "FileViewer.h"
#include "StorageAnalyzer.h"

class FileExplorer {

private:

    // Other modules
    DirectoryManager directoryManager;

    ContextMenu contextMenu;

    ClipboardManager clipboardManager;

    FileViewer fileViewer;

    // Initialized after workspace creation
    std::unique_ptr<FileManager> fileManager;

    std::unique_ptr<SearchEngine> searchEngine;

    std::unique_ptr<StorageAnalyzer> storageAnalyzer;

    bool running;

    // Display functions
    void displayHeader();

    void displayDirectory();

    // Command handling
    void handleCommand(const std::string& command);

    void handleSelection(int index);

    void handleItemCommand(const std::string& action, int index);

    // Open selected file or folder
    void openItem(int index);

    // Display help
    void displayHelp();

public:

    // Constructor
    FileExplorer();

    // Initialize application
    void initialize();

    // Main application loop
    void run();

    // Display current directory
    void display();

    // Navigation
    void goBack();

    void goHome();

    // Exit application
    void exitApplication();

};

#endif
