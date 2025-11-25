#include "Common.hpp"
#include "Notify.hpp"
#include "Log.hpp"

namespace SimpleNodeEditor
{

Message::Status Message::GetStatus() const
{
    const auto elapsedTime = ImGui::GetTime() - m_notifCreateTime; // [s]
    if (elapsedTime > m_fadeInTime + m_onTime + m_fadeOutTime) { return Status::OFF; }
    else if (elapsedTime > m_fadeInTime + m_onTime) { return Status::FADEOUT; }
    else if (elapsedTime > m_fadeInTime) { return Status::ON; }
    else { return Status::FADEIN; }
}

std::u8string Message::GetIcon() const
{
    if (m_type == Type::SUCCESS) { return std::u8string(u8"\ue86c"); }
    else if (m_type == Type::WARNING) { return std::u8string(u8"\ue002"); }
    else if (m_type == Type::ERR) { return std::u8string(u8"\ue000"); }
    else if (m_type == Type::INFO) { return std::u8string(u8"\ue88e"); }
    else { return u8""; }
}

std::string Message::GetTypeName() const
{
    if (m_type == Type::SUCCESS) { return "Success"; }
    else if (m_type == Type::WARNING) { return "Warning"; }
    else if (m_type == Type::ERR) { return "Error"; }
    else if (m_type == Type::INFO) { return "Info"; }
    else { return ""; }
}

float Message::GetFadeValue() const
{
    const auto status = GetStatus();
    const auto elapsedTime = static_cast<float>(ImGui::GetTime() - m_notifCreateTime); // [s]
    if (status == Status::FADEIN)
    {
        return (elapsedTime / m_fadeInTime) * m_opacity;
    }
    else if (status == Status::FADEOUT)
    {
        return (1.0f - ((elapsedTime - m_fadeInTime - m_onTime) / m_fadeOutTime)) * m_opacity;
    }
    return 1.0f * m_opacity;
}

ImColor Message::GetColor() const
{
    if (m_type == Type::SUCCESS) { return ImColor(0.0f, 0.5f, 0.0f, 1.0f); }
    else if (m_type == Type::WARNING) { return ImColor(0.5f, 0.5f, 0.0f, 1.0f); }
    else if (m_type == Type::ERR) { return ImColor(0.5f, 0.0f, 0.0f, 1.0f); }
    else if (m_type == Type::INFO) { return ImColor(0.0f, 0.3f, 0.5f, 1.0f); }
    else { return ImColor(1.0f, 1.0f, 1.0f, 1.0f); }
}

Message::Message(Type type, const std::string& title, const std::string& content, float onTime) :
    m_type(type), m_title(title), m_content(content), m_onTime(onTime)
{
    m_notifCreateTime = static_cast<float>(ImGui::GetTime());
}

void Notifier::DrawNotifications()
{
    float height = 0.0f;
    for (int i = 0; i < m_msgs.size(); i++)
    {
        auto notif = &m_msgs[i];
        if (notif->GetStatus() == Message::Status::OFF)
        {
            RemoveMsg(i);
            continue;
        }
        std::string notifName = "Notif" + std::to_string(i);
        const auto fadeValue = notif->GetFadeValue();
        ImGui::SetNextWindowBgAlpha(fadeValue);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, m_rounding);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, m_backgroundColor);
        const auto rightCorner = ImVec2(ImGui::GetMainViewport()->Pos.x + ImGui::GetMainViewport()->Size.x, ImGui::GetMainViewport()->Pos.y + ImGui::GetMainViewport()->Size.y);
        ImGui::SetNextWindowPos(ImVec2(rightCorner.x - m_xPadding, rightCorner.y - m_yPadding - height), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
        // ImGuiWindowFlags_NoBringToFrontOnFocus can not be set
        ImGui::Begin(notifName.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav );
        ImGui::PushTextWrapPos(ImGui::GetMainViewport()->Size.x * m_wrapRatio);
        const auto icon = notif->GetIcon();
        auto iconColor = notif->GetColor();
        iconColor.Value.w = fadeValue;
        // ImGui::TextColored(iconColor, reinterpret_cast<const char*>(icon.c_str()));
        ImGui::SameLine();
        if (const auto title = notif->GetTitle(); title.empty() == false)
        {
            ImGui::TextColored(notif->GetColor(), title.c_str());
        }
        else if (const auto typeName = notif->GetTypeName(); typeName.empty() == false)
        {
            ImGui::TextColored(notif->GetColor(), typeName.c_str());
        }
        if (const auto content = notif->GetContent(); content.empty() == false)
        {
            ImGui::Separator();
            ImGui::TextColored(notif->GetColor(), content.c_str());
        }
        ImGui::PopTextWrapPos();
        height += ImGui::GetWindowHeight() + m_yMessagePadding;
        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }
    m_heightMsgs = height;
}

void Notifier::AddMessage(const Message & msg)
{
    if (m_heightMsgs >= ImGui::GetMainViewport()->Size.y * 0.8f)
    {
        RemoveMsg(0);
    }
    m_msgs.push_back(msg);
}
}
