
#ifndef SEARCH_ENGINE_H
#define SEARCH_ENGINE_H

#include <filesystem>
#include <string>
#include <vector>

class SearchEngine {

private:

    std::filesystem::path workspacePath;

public:

    // Constructor
    SearchEngine(const std::filesystem::path& workspace);

    // Search files and folders by name
    std::vector<std::filesystem::path> search(
        const std::string& keyword
    ) const;

};

#endif
