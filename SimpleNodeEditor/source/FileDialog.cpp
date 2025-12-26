#include "FileDialog.hpp"
#include "SNEConfig.hpp"
#include "Log.hpp"
#include "Common.hpp"
#include <vector>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
namespace SimpleNodeEditor
{

FileDialog::FileDialog()
:m_fs{ std::make_unique<FS::LocalFileSystem>() } // using local filesystem bydefault
{
    if (m_fs->GetFileSystemType() == FS::FileSystemType::Local)
    {
        // default open directory is currentruningpath/resource
        SetDefaultDirectoryPath(std::filesystem::current_path().append("resource"));
    }
}

void FileDialog::SwitchFileSystemType()
{
    if (m_fs->GetFileSystemType() == FS::FileSystemType::Local)
    {
        // Try to create SSH filesystem and only switch to it if connected
        auto sshfs = std::make_unique<FS::SshFileSystem>(SNEConfig::GetInstance().GetConfigValue<FS::SshConnectionInfo>("SshConnectionInfo"));
        if (sshfs && sshfs->IsConnected())
        {
            m_fs = std::move(sshfs);
            SetDefaultDirectoryPath(SNEConfig::GetInstance().GetConfigValue<std::string>("SshFileSystemDefaultOpenPath"));
        }
        else
        {
            SNELOG_ERROR("Failed to connect SSH filesystem, staying on LocalFileSystem");
            Notifier::Add(Message{Message::Type::ERR, "", "Unable to connect to SSH server. Using local filesystem."});
        }
    }
    else if (m_fs->GetFileSystemType() == FS::FileSystemType::Ssh)
    {
        m_fs = std::make_unique<FS::LocalFileSystem>();
        SetDefaultDirectoryPath(std::filesystem::current_path().append("resource"));
    }
    else
    {
        SNELOG_ERROR("Unreacheable! Invalid filesystem type [{}]", static_cast<int>(m_fs->GetFileSystemType()));
        SNE_ASSERT(false);
    }
}

bool FileDialog::Draw()
{
    if (!m_isRendered)
    {
        return false;
        SNELOG_DEBUG("(m_isRendered false)");
    }
    else
    {
        ImGui::OpenPopup(m_title.c_str());
        SNELOG_DEBUG("FileDiaLogOpenBegin()");
    }

    bool done = false;
    m_title = (m_type == Type::OPEN) ? "Open File" : "Save File";
    ImGui::SetNextWindowSize(ImVec2(660.0f, 410.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(ImVec2(410, 410), ImVec2(1080, 410));
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(m_title.c_str(), nullptr, ImGuiWindowFlags_NoCollapse))
    {
        // switch between local and ssh filesystem
        if (ImGui::Button("Switch"))
        {
            SwitchFileSystemType();
            ResetState();
        }
        // allow for virtual disk changing in windows
        #ifdef _WIN32
            if (m_fs->GetFileSystemType() != FS::FileSystemType::Ssh)
            {
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                std::vector<std::string> virtualDrivers;
                DWORD len = GetLogicalDriveStringsA(0, nullptr);
                if (len > 0)
                {
                    std::vector<char> buf(len + 1);
                    GetLogicalDriveStringsA(len, buf.data());
                    char* cur = buf.data();
                    while (*cur)
                    {
                        virtualDrivers.emplace_back(cur);
                        cur += strlen(cur) + 1;
                    }
                }
                if (ImGui::BeginCombo("##ChangeRoot", m_directoryPath.String().c_str()))
                {
                    static int driveIndex = -1;
                    for (size_t index = 0; index < virtualDrivers.size(); ++index)
                    {
                        bool sel = (index == driveIndex);
                        if (ImGui::Selectable(virtualDrivers[index].c_str(), sel))
                        {
                            driveIndex = static_cast<int>(index);
                            m_directoryPath = virtualDrivers[index];
                            m_refresh = true;
                        }
                    }
                    ImGui::EndCombo();
                }
            }
        # else
            ImGui::Text("%s", m_directoryPath.string().c_str());
        #endif

        if (m_currentFiles.empty() && m_currentDirectories.empty() || m_refresh)
        {
            ResetState();
            for (auto &entry : m_fs->List(m_directoryPath.String()))
            {
                entry.m_isDirectory ? m_currentDirectories.push_back(entry) : m_currentFiles.push_back(entry);
            }
        }
        /** Begin of listing directories and files**/
        ImGui::BeginChild("##browser", ImVec2(ImGui::GetContentRegionAvail().x, 300.0f), true, ImGuiWindowFlags_None);
        size_t index = 0;
        // Parent
        if (m_directoryPath.HasParentPath())
        {
            if (ImGui::Selectable("..", m_currentIndex == index, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(ImGui::GetContentRegionAvail().x, 0)))
            {
                m_currentIndex = index;
                if (ImGui::IsMouseDoubleClicked(0))
                {
                    m_directoryPath = m_directoryPath.ParentPath();
                    m_refresh = true;
                }
            }
            index++;
        }

        // Directories
        for (const auto& element : m_currentDirectories)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 210, 0, 255));
            if (ImGui::Selectable(element.m_path.GetFileName().c_str(), m_currentIndex == index, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(ImGui::GetContentRegionAvail().x, 0)))
            {
                m_currentIndex = index;
                if (ImGui::IsMouseDoubleClicked(0))
                {
                    m_directoryPath = element.m_path;
                    m_refresh = true;
                }
            }
            ImGui::PopStyleColor();
            index++;
        }

        // Files
        for (const auto& element : m_currentFiles)
        {
            if (ImGui::Selectable(element.m_path.GetFileName().c_str(), m_currentIndex == index, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(ImGui::GetContentRegionAvail().x, 0)))
            {
                m_currentIndex = index;
                m_fileName = FS::Path(element.m_path.GetFileName());
                if (ImGui::IsMouseDoubleClicked(0))
                {
                    m_isDoubleClickedOnFileName = true;
                }
            }
            index++;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
        {
            m_currentIndex = m_currentIndex == std::numeric_limits<size_t>::max() ? index : (m_currentIndex == 0 ? index - 1 : m_currentIndex - 1);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
        {
            m_currentIndex = (m_currentIndex == std::numeric_limits<size_t>::max() || (m_currentIndex == index -1)) ? 0 : std::min(m_currentIndex + 1, index - 1);
        }
        ImGui::EndChild();
        /** End of listing directories and files**/

        // Draw filename in input texteditor
        size_t fileNameSize = m_fileName.String().size();
        if (fileNameSize >= s_bufferSize)
        {
            fileNameSize = s_bufferSize - 1;
        }
        std::memcpy(m_buffer, m_fileName.String().c_str(), fileNameSize);
        m_buffer[fileNameSize] = 0;

        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 200, 0, 255));
        if (ImGui::InputText("File Name", m_buffer, s_bufferSize))
        {
            m_fileName = std::string(m_buffer);
            m_currentIndex = std::numeric_limits<size_t>::max();
        }
        ImGui::PopStyleColor();

        ImGui::SetNextItemShortcut(ImGuiKey_Escape);
        if (ImGui::Button("Cancel"))
        {
            ResetState();
            m_isRendered = false;
        }
        ImGui::SameLine();

        m_resultPath = m_directoryPath / m_fileName;
        if (m_type == Type::OPEN)
        {
            // ImGui::SetNextItemShortcut(ImGuiKey_Enter);
            if (ImGui::Button("Open") || m_isDoubleClickedOnFileName)
            {
                if ( m_fileName.String().rfind(".") != std::string::npos
                    && m_fileName.String().substr(m_fileName.String().rfind(".")) == m_fileFormat)
                {
                    if (m_fs->Exists(m_resultPath.String()))
                    {
                        ResetState();
                        m_isRendered = false;
                        done = true;
                    }
                    else
                    {
                        SNELOG_ERROR("not exist FileName[{}]", m_fileName.String());
                    }
                }
                else 
                {
                    SNELOG_ERROR("Invalid File Name[{}]", m_fileName.String());
                    Notifier::Add(Message(Message::Type::ERR, "", "File format is not valid. Only " + m_fileFormat + " is allowed"));
                    m_isDoubleClickedOnFileName = false;
                }
            }
        }
        else if (m_type == Type::SAVE)
        {
            const auto beforeFormatCheck = m_resultPath.String();
            bool isFormatCorrect = false;
            if (auto dot = m_fileName.String().rfind("."); dot == std::string::npos)
            {
                m_resultPath = m_resultPath.String() + m_fileFormat;
            }
            else if(m_fileName.String().substr(dot) != m_fileFormat)
            {
                m_resultPath = m_resultPath.String() + m_fileFormat;
            }
            else
            {
                isFormatCorrect = true;
            }

            ImGui::SetNextItemShortcut(ImGuiKey_Enter);
            if (ImGui::Button("Save") )
            {
                if (m_fs->Exists(beforeFormatCheck) == true && isFormatCorrect == false)
                {
                    Notifier::Add(Message(Message::Type::ERR, "", "Another file Exists with the same name."));
                }
                else if (m_fs->Exists(m_resultPath.String()) == false)
                {
                    ResetState();
                    m_isRendered = false;
                    done = true;
                }
                else
                {
                    ImGui::OpenPopup("Override?");
                }
            }
            ImGui::SetNextWindowPos(ImGui::GetCurrentWindow()->Rect().GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            if (ImGui::BeginPopupModal("Override?", nullptr, ImGuiWindowFlags_NoResize))
            {
                ImGui::Text("File already exists. Do you want to override?");
                ImGui::Separator();
                ImGui::SetNextItemShortcut(ImGuiKey_Enter);
                if (ImGui::Button("Yes", ImVec2(50.0f, 0.0f)))
                {
                    ResetState();
                    m_isRendered = false;
                    done = true;
                }
                ImGui::SameLine();
                ImGui::SetNextItemShortcut(ImGuiKey_Escape);
                if (ImGui::Button("No", ImVec2(50.0f, 0.0f)))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        ImGui::EndPopup();

    }
    return done;
}

} // namespace SimpleNodeEditor

