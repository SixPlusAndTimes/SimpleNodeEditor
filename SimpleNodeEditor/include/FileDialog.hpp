#ifndef FILEDIALOG_H
#define FILEDIALOG_H

#include <filesystem>
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include "Notify.hpp"

namespace SimpleNodeEditor
{

class FileDialog
{
public:
    enum class Type
    {
        SAVE,
        OPEN
    };
private:
    Type type = Type::OPEN;
    std::string fileFormat{ ".yaml" };
    std::filesystem::path fileName;
    std::filesystem::path directoryPath;
    std::filesystem::path resultPath;
    bool refresh;
    size_t currentIndex;
    std::vector<std::filesystem::directory_entry> currentFiles;
    std::vector<std::filesystem::directory_entry> currentDirectories;
    std::string title{ "" };
    static const size_t bufferSize = 200;
    char buffer[bufferSize];

public:
    FileDialog() = default;
    virtual ~FileDialog() = default;

    void SetType(Type t) { type = t; }
    void SetFileName(std::string_view name) { fileName = name; }
    void SetDirectory(const std::filesystem::path& dir) { directoryPath = dir; }
    std::filesystem::path GetResultPath() const { return resultPath; }
    auto GetFileName() const { return fileName; }
    auto GetFileFormat() const { return fileFormat; }
    Type GetType() const { return type; }
    bool Draw(bool* open);
};
}

#endif // FILEDIALOG_H