#ifndef NOTIFY_H
#define NOTIFY_H

#include "imgui.h"
#include <vector>
#include <string> 

namespace SimpleNodeEditor
{

class Message
{
public:
    enum class Status
    {
        FADEIN,
        ON,
        FADEOUT,
        OFF
    };
    enum class Type
    {
        NONE,
        SUCCESS,
        WARNING,
        ERR,
        INFO
    };
    Message(Type type, const std::string& title, const std::string& content = "", float onTime = 3.0f);
    virtual ~Message() = default;

    Status GetStatus() const;
    std::u8string GetIcon() const;
    std::string GetTitle() const { return m_title; }
    std::string GetContent() const { return m_content; }
    std::string GetTypeName() const;
    float GetFadeValue() const;
    ImColor GetColor() const;

private:
    Type m_type = Type::NONE;
    std::string m_title{ "" };
    std::string m_content{ "" };
    float m_fadeInTime{ 0.150f };
    float m_onTime{ 3.0f };
    float m_fadeOutTime{ 0.150f };
    float m_opacity{ 1.0f };
    float m_notifCreateTime{ 0.0f };
};

class Notifier
{
public:
    Notifier(const Notifier&) = delete;
    Notifier& operator=(const Notifier&) = delete;
    virtual ~Notifier() = default;
    static Notifier& GetInstance()
    {
        static Notifier instance;
        return instance;
    }
    static void Add(const Message& msg) { GetInstance().AddMessage(msg); }
    static void Draw() { GetInstance().DrawNotifications(); }
    void DrawNotifications();

private:
    Notifier() = default;
    void AddMessage(const Message& msg);
    float m_xPadding{ 30.0f };
    float m_yPadding{ 30.0f };
    float m_yMessagePadding{ 10.0f };
    float m_wrapRatio{ 0.25f };
    float m_rounding = 8.0f;
    ImVec4 m_backgroundColor = ImVec4(0.886f, 0.929f, 0.969f, 0.4f);
    std::vector<Message> m_msgs;
    float m_heightMsgs{ 0.0f };
    void RemoveMsg(int i) { m_msgs.erase(m_msgs.begin() + i); }
};

}

#endif // NOTIFY_H