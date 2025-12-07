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
    Type m_type = Type::OPEN;
    std::string m_fileFormat{ ".yaml" };
    std::filesystem::path m_fileName;
    std::filesystem::path m_directoryPath;
    std::filesystem::path m_resultPath;
    bool m_refresh;
    size_t m_currentIndex;
    std::vector<std::filesystem::directory_entry> m_currentFiles;
    std::vector<std::filesystem::directory_entry> m_currentDirectories;
    std::string m_title{ "" };
    static const size_t s_bufferSize = 200;
    char m_buffer[s_bufferSize];

public:
    FileDialog() = default;
    virtual ~FileDialog() = default;

    void SetType(Type t) { m_type = t; }
    void SetFileName(std::string_view name) { m_fileName = name; }
    void SetDirectory(const std::filesystem::path& dir) { m_directoryPath = dir; }
    std::filesystem::path GetResultPath() const { return m_resultPath; }
    auto GetFileName() const { return m_fileName; }
    auto GetFileFormat() const { return m_fileFormat; }
    Type GetType() const { return m_type; }
    bool Draw(bool* open);
};
}

#endif // FILEDIALOG_H