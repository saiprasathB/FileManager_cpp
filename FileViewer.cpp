
#include "FileViewer.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>

using namespace std;
namespace fs = std::filesystem;


// Open and display file
void FileViewer::openFile(const fs::path& filePath) {

    try {

        // Check whether file exists
        if (!fs::exists(filePath)) {

            cout << "File does not exist.\n";
            return;
        }

        // Check whether selected item is a regular file
        if (!fs::is_regular_file(filePath)) {

            cout << "Selected item is not a regular file.\n";
            return;
        }

        // Open file
        ifstream file(filePath);

        if (!file.is_open()) {

            cout << "Unable to open file.\n";
            return;
        }

        cout << "\n========== FILE CONTENTS ==========\n";

        string line;

        // Read file line by line
        while (getline(file, line)) {

            cout << line << '\n';
        }

        cout << "===================================\n";

        file.close();

    }
    catch (const fs::filesystem_error& e) {

        cerr << "File viewing error: " << e.what() << '\n';
    }
}
