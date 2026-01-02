#ifndef FILEDIALOG_H
#define FILEDIALOG_H

#include <filesystem>
#include <vector>
#include <cstring>
#include <limits>
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include "Notify.hpp"
#include "FileSystem.hpp"
namespace SimpleNodeEditor
{

class FileDialog
{
public:
    enum class Type
    {
        SAVE,
        OPEN,
    };
private:
    Type m_type = Type::OPEN;
    
    std::string m_fileFormat{ ".yaml" };
    FS::Path m_fileName;
    FS::Path m_directoryPath;
    FS::Path m_resultPath;
    bool m_refresh;
    bool m_isRendered = false;
    size_t m_currentIndex = std::numeric_limits<size_t>::max();
    std::vector<FS::FileEntry> m_currentFiles;
    std::vector<FS::FileEntry> m_currentDirectories;
    std::string m_title{ "" };
    static const size_t s_bufferSize = 200;
    char m_buffer[s_bufferSize];
    bool m_isDoubleClickedOnFileName = false;
    // RemoteSshContext m_sshContext;
    std::shared_ptr<FS::IFileSystem> m_fs;
    void ResetState()
    {
        m_refresh = false;
        m_currentIndex = std::numeric_limits<size_t>::max();
        m_currentFiles.clear();
        m_currentDirectories.clear();
        m_isDoubleClickedOnFileName = false;
    }; // related to file views
public:
    FileDialog();
    virtual ~FileDialog() = default;

    void                                SwitchFileSystemType();
    void                                MarkFileDialogOpen(){m_type = Type::OPEN; m_isRendered = true;};
    void                                MarkFileDialogSave(const std::string& name){m_type = Type::SAVE; m_isRendered = true; SetFileName(name);};
    void                                SetType(Type t) { m_type = t; }
    void                                SetFileName(const std::string name) { m_fileName = name; }
    void                                SetDefaultDirectoryPath(const std::filesystem::path& dir) { m_directoryPath = dir; }
    FS::Path                            GetResultPath() const { return m_resultPath; }
    std::unique_ptr<std::ostream>       GetResultOutStream(); 
    std::unique_ptr<std::istream>       GetResultInStream(); 
    auto                                GetFileName() const { return m_fileName; }
    auto                                GetFileFormat() const { return m_fileFormat; }
    Type                                GetType() const { return m_type; }
    bool                                Draw();
};
}

#endif // FILEDIALOG_H