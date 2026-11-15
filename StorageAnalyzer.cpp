
#include "StorageAnalyzer.h"

#include <iostream>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;


// Constructor
StorageAnalyzer::StorageAnalyzer(const fs::path& workspace) {

    workspacePath = workspace;
}


// Analyze workspace storage
StorageInfo StorageAnalyzer::analyze() const {

    StorageInfo info;

    try {

        fs::path root = fs::canonical(workspacePath);

        if (!fs::is_directory(root)) {

            cerr << "Workspace is not a directory.\n";

            return info;
        }

        // Recursively visit workspace contents
        fs::recursive_directory_iterator iterator(
            root,
            fs::directory_options::skip_permission_denied
        );

        fs::recursive_directory_iterator end;

        while (iterator != end) {

            const auto& entry = *iterator;

            error_code ec;

            // Skip symbolic links
            if (entry.is_symlink(ec)) {

                iterator.increment(ec);
                continue;
            }

            // Check whether entry is a folder
            if (entry.is_directory(ec)) {

                info.totalFolders++;
            }

            // Check whether entry is a regular file
            else if (entry.is_regular_file(ec)) {

                info.totalFiles++;

                // Calculate file size
                uintmax_t size = entry.file_size(ec);

                if (!ec) {

                    info.totalBytes += size;
                }
            }

            // Move to next entry
            iterator.increment(ec);
        }

    }
    catch (const fs::filesystem_error& e) {

        cerr << "Storage analysis error: "
             << e.what() << '\n';
    }

    return info;
}


// Display storage information
void StorageAnalyzer::displayStorageInfo() const {

    StorageInfo info = analyze();

    cout << "\n========== STORAGE ANALYSIS ==========\n";

    cout << "Total Files: "
         << info.totalFiles << '\n';

    cout << "Total Folders: "
         << info.totalFolders << '\n';

    cout << "Total Storage: "
         << info.totalBytes << " bytes\n";

    cout << "Total Storage (KB): "
         << static_cast<double>(info.totalBytes) / 1024
         << " KB\n";

    cout << "Total Storage (MB): "
         << static_cast<double>(info.totalBytes) / (1024 * 1024)
         << " MB\n";

    cout << "======================================\n";
}
