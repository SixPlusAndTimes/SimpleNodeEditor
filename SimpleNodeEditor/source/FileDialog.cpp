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
        ImGui::OpenPopup(title.c_str());
    }

    bool done = false;
    title = (type == Type::OPEN) ? "Open File" : "Save File";
    ImGui::SetNextWindowSize(ImVec2(660.0f, 410.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(ImVec2(410, 410), ImVec2(1080, 410));
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_NoCollapse))
    {
        if (currentFiles.empty() && currentDirectories.empty() || refresh)
        {
            refresh = false;
            currentIndex = 0;
            currentFiles.clear();
            currentDirectories.clear();
            for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(directoryPath))
            {
                entry.is_directory() ? currentDirectories.push_back(entry) : currentFiles.push_back(entry);
            }
        }

        // Path
        ImGui::Text("%s", directoryPath.string().c_str());
        ImGui::BeginChild("##browser", ImVec2(ImGui::GetContentRegionAvail().x, 300.0f), true, ImGuiWindowFlags_None);
        size_t index = 0;

        // Parent
        if (directoryPath.has_parent_path())
        {
            if (ImGui::Selectable("..", currentIndex == index, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(ImGui::GetContentRegionAvail().x, 0)))
            {
                currentIndex = index;
                if (ImGui::IsMouseDoubleClicked(0))
                {
                    directoryPath = directoryPath.parent_path();
                    refresh = true;
                }
            }
            index++;
        }

        // Directories
        for (const auto& element : currentDirectories)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 210, 0, 255));
            if (ImGui::Selectable(element.path().filename().string().c_str(), currentIndex == index, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(ImGui::GetContentRegionAvail().x, 0)))
            {
                currentIndex = index;
                if (ImGui::IsMouseDoubleClicked(0))
                {
                    directoryPath = element.path();
                    refresh = true;
                }
            }
            ImGui::PopStyleColor();
            index++;
        }

        // Files
        for (const auto& element : currentFiles)
        {
            if (ImGui::Selectable(element.path().filename().string().c_str(), currentIndex == index, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(ImGui::GetContentRegionAvail().x, 0)))
            {
                currentIndex = index;
                fileName = element.path().filename();
            }
            index++;
        }
        ImGui::EndChild();

        // Draw filename
        size_t fileNameSize = fileName.string().size();
        if (fileNameSize >= bufferSize)
        {
            fileNameSize = bufferSize - 1;
        }
        std::memcpy(buffer, fileName.string().c_str(), fileNameSize);
        buffer[fileNameSize] = 0;

        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 200, 0, 255));
        if (ImGui::InputText("File Name", buffer, bufferSize))
        {
            fileName = std::string(buffer);
            currentIndex = 0;
        }
        ImGui::PopStyleColor();
        if (ImGui::Button("Cancel"))
        {
            refresh = false;
            currentIndex = 0;
            currentFiles.clear();
            currentDirectories.clear();
            *open = false;
        }
        ImGui::SameLine();
        resultPath = directoryPath / fileName;
        if (type == Type::OPEN)
        {
            if (ImGui::Button("Open"))
            {
                if (std::filesystem::exists(resultPath) && fileName.string().rfind(".") != std::string::npos
                    && fileName.string().substr(fileName.string().rfind(".")) == fileFormat)
                {
                    refresh = false;
                    currentIndex = 0;
                    currentFiles.clear();
                    currentDirectories.clear();
                    done = true;
                    *open = false;
                }
                else
                {
                    Notifier::Add(Message(Message::Type::ERR, "", "File format is not valid."));
                }
            }
        }
        else if (type == Type::SAVE)
        {
            const auto beforeFormatCheck = resultPath.string();
            bool isFormatCorrect = false;
            if (auto dot = fileName.string().rfind("."); dot == std::string::npos)
            {
                resultPath = resultPath.string() + fileFormat;
            }
            else if(fileName.string().substr(dot) != fileFormat)
            {
                resultPath = resultPath.string() + fileFormat;
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
                else if (std::filesystem::exists(resultPath) == false)
                {
                    refresh = false;
                    currentIndex = 0;
                    currentFiles.clear();
                    currentDirectories.clear();
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
                    refresh = false;
                    currentIndex = 0;
                    currentFiles.clear();
                    currentDirectories.clear();
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

