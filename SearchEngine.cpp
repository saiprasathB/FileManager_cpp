
#include "SearchEngine.h"

#include <algorithm>
#include <cctype>
#include <iostream>

using namespace std;
namespace fs = std::filesystem;


// Constructor
SearchEngine::SearchEngine(const fs::path& workspace) {

    workspacePath = fs::canonical(workspace);
}


// Search files and folders
vector<fs::path> SearchEngine::search(const string& keyword) const {

    vector<fs::path> results;

    if (keyword.empty()) {

        cout << "Search keyword cannot be empty.\n";

        return results;
    }

    try {

        // Convert keyword to lowercase
        string searchKeyword = keyword;

        transform(searchKeyword.begin(), searchKeyword.end(),
                  searchKeyword.begin(),
                  [](unsigned char ch) {
                      return static_cast<char>(tolower(ch));
                  });

        // Recursively search workspace
        fs::recursive_directory_iterator iterator(
            workspacePath,
            fs::directory_options::skip_permission_denied
        );

        fs::recursive_directory_iterator end;

        while (iterator != end) {

            const auto& entry = *iterator;

            error_code ec;

            // Skip symbolic links
            if (entry.is_symlink(ec)) {

                iterator.increment(ec);

                if (ec) {
                    ec.clear();
                }

                continue;
            }

            // Get item name
            string name = entry.path().filename().string();

            // Convert item name to lowercase
            transform(name.begin(), name.end(),
                      name.begin(),
                      [](unsigned char ch) {
                          return static_cast<char>(tolower(ch));
                      });

            // Check whether name contains keyword
            if (name.find(searchKeyword) != string::npos) {

                results.push_back(entry.path());
            }

            // Move to next entry
            iterator.increment(ec);

            if (ec) {
                ec.clear();
            }
        }

        // Sort search results alphabetically
        sort(results.begin(), results.end());

    }
    catch (const fs::filesystem_error& e) {

        cerr << "Search error: " << e.what() << '\n';
    }

    return results;
}
