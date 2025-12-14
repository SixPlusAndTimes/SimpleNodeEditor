#include "FileDialog.hpp"
#include "Log.hpp"
namespace SimpleNodeEditor
{
    
bool FileDialog::Draw(bool* open)
{
    if (*open == false)
    {
        return false;
    }
    else
    {
        ImGui::OpenPopup(m_title.c_str());
    }

    bool done = false;
    m_title = (m_type == Type::OPEN) ? "Open File" : "Save File";
    ImGui::SetNextWindowSize(ImVec2(660.0f, 410.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(ImVec2(410, 410), ImVec2(1080, 410));
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(m_title.c_str(), nullptr, ImGuiWindowFlags_NoCollapse))
    {
        if (m_currentFiles.empty() && m_currentDirectories.empty() || m_refresh)
        {
            m_refresh = false;
            m_currentIndex = 0;
            m_currentFiles.clear();
            m_currentDirectories.clear();
            for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(m_directoryPath))
            {
                entry.is_directory() ? m_currentDirectories.push_back(entry) : m_currentFiles.push_back(entry);
            }
        }

        // Path
        ImGui::Text("%s", m_directoryPath.string().c_str());
        ImGui::BeginChild("##browser", ImVec2(ImGui::GetContentRegionAvail().x, 300.0f), true, ImGuiWindowFlags_None);
        size_t index = 0;

        // Parent
        if (m_directoryPath.has_parent_path())
        {
            if (ImGui::Selectable("..", m_currentIndex == index, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(ImGui::GetContentRegionAvail().x, 0)))
            {
                m_currentIndex = index;
                if (ImGui::IsMouseDoubleClicked(0))
                {
                    m_directoryPath = m_directoryPath.parent_path();
                    m_refresh = true;
                }
            }
            index++;
        }

        // Directories
        for (const auto& element : m_currentDirectories)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 210, 0, 255));
            if (ImGui::Selectable(element.path().filename().string().c_str(), m_currentIndex == index, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(ImGui::GetContentRegionAvail().x, 0)))
            {
                m_currentIndex = index;
                if (ImGui::IsMouseDoubleClicked(0))
                {
                    m_directoryPath = element.path();
                    m_refresh = true;
                }
            }
            ImGui::PopStyleColor();
            index++;
        }

        // Files
        for (const auto& element : m_currentFiles)
        {
            if (ImGui::Selectable(element.path().filename().string().c_str(), m_currentIndex == index, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(ImGui::GetContentRegionAvail().x, 0)))
            {
                m_currentIndex = index;
                m_fileName = element.path().filename();
                if (ImGui::IsMouseDoubleClicked(0))
                {
                    m_isDoubleClickedOnItem = true;
                }
            }
            index++;
        }
        ImGui::EndChild();

        // Draw filename
        size_t fileNameSize = m_fileName.string().size();
        if (fileNameSize >= s_bufferSize)
        {
            fileNameSize = s_bufferSize - 1;
        }
        std::memcpy(m_buffer, m_fileName.string().c_str(), fileNameSize);
        m_buffer[fileNameSize] = 0;

        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 200, 0, 255));
        if (ImGui::InputText("File Name", m_buffer, s_bufferSize))
        {
            m_fileName = std::string(m_buffer);
            m_currentIndex = 0;
        }
        ImGui::PopStyleColor();
        if (ImGui::Button("Cancel"))
        {
            m_refresh = false;
            m_currentIndex = 0;
            m_currentFiles.clear();
            m_currentDirectories.clear();
            *open = false;
        }
        ImGui::SameLine();
        m_resultPath = m_directoryPath / m_fileName;
        if (m_type == Type::OPEN)
        {
            if (ImGui::Button("Open") || m_isDoubleClickedOnItem)
            {
                if (std::filesystem::exists(m_resultPath) && m_fileName.string().rfind(".") != std::string::npos
                    && m_fileName.string().substr(m_fileName.string().rfind(".")) == m_fileFormat)
                {
                    m_refresh = false;
                    m_currentIndex = 0;
                    m_currentFiles.clear();
                    m_currentDirectories.clear();
                    done = true;
                    *open = false;
                    m_isDoubleClickedOnItem = false;
                }
                else
                {
                    Notifier::Add(Message(Message::Type::ERR, "", "File format is not valid."));
                }
            }
        }
        else if (m_type == Type::SAVE)
        {
            const auto beforeFormatCheck = m_resultPath.string();
            bool isFormatCorrect = false;
            if (auto dot = m_fileName.string().rfind("."); dot == std::string::npos)
            {
                m_resultPath = m_resultPath.string() + m_fileFormat;
            }
            else if(m_fileName.string().substr(dot) != m_fileFormat)
            {
                m_resultPath = m_resultPath.string() + m_fileFormat;
            }
            else
            {
                isFormatCorrect = true;
            }
            if (ImGui::Button("Save"))
            {
                if (std::filesystem::exists(beforeFormatCheck) == true && isFormatCorrect == false)
                {
                    Notifier::Add(Message(Message::Type::ERR, "", "Another file exists with the same name."));
                }
                else if (std::filesystem::exists(m_resultPath) == false)
                {
                    m_refresh = false;
                    m_currentIndex = 0;
                    m_currentFiles.clear();
                    m_currentDirectories.clear();
                    done = true;
                    *open = false;
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
                if (ImGui::Button("Yes", ImVec2(50.0f, 0.0f)))
                {
                    m_refresh = false;
                    m_currentIndex = 0;
                    m_currentFiles.clear();
                    m_currentDirectories.clear();
                    done = true;
                    *open = false;
                }
                ImGui::SameLine();
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

