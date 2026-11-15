
#ifndef FILE_VIEWER_H
#define FILE_VIEWER_H

#include <filesystem>

class FileViewer {

public:

    // Display file contents
    void openFile(const std::filesystem::path& filePath);

};

#endif
