#include "CommandQueue.hpp"
#include <sstream>
#include "Log.hpp"
namespace SimpleNodeEditor
{

CommandQueue::CommandQueue()
: m_queue()
, m_current(0)
{ }

void CommandQueue::AddAndExecuteCommad(std::unique_ptr<ICommand> cmd)
{
    m_queue[m_current] = std::move(cmd);
    m_queue[m_current]->Execute(); 
    SNELOG_INFO("AddAndExecuteCommad, commandInfo: {}", m_queue[m_current]->ToString());
    m_current = (m_current + 1) % COMMANDQUEUE_SIZE;
    m_queue[m_current] = nullptr; // we must delete original command here
}

bool CommandQueue::Undo()
{
    size_t prev = (m_current - 1 + COMMANDQUEUE_SIZE) % COMMANDQUEUE_SIZE;

    if (!m_queue[prev]) 
    {
        return false;
    }

    m_queue[prev]->Undo();
    SNELOG_INFO("Perform Undo, commandInfo: {}", m_queue[prev]->ToString());
    m_current = prev;

    return true;
}

bool CommandQueue::Redo()
{
    if (!m_queue[m_current])
    {
        return false;
    }
    m_queue[m_current]->Redo();
    SNELOG_INFO("Perform Redo command: {}", m_queue[m_current]->ToString());
    m_current = (m_current + 1) % COMMANDQUEUE_SIZE;
    return true;
}

void CommandQueue::Clear()
{
    m_current = 0;
    std::array<std::unique_ptr<ICommand>, COMMANDQUEUE_SIZE>().swap(m_queue);
}

std::string CommandQueue::ToString()
{
    std::stringstream strstream;
    strstream << "CommandQueueInfo : total elem size : " << COMMANDQUEUE_SIZE << " m_current is " << m_current << std::endl;
    for (size_t i = 0; i < m_queue.size(); ++i)
    {
        if (m_current == i)
        {
            strstream << "m_current =>\t";
        } else 
        {
            strstream << "\t\t";
        }
        if (m_queue[i] != nullptr)
        {
            strstream << i << "th elm info : " << m_queue[i]->ToString() << std::endl;
        }
        else
        {
            strstream << i << "th elm is nullptr " << std::endl;
        }
    }
    return strstream.str();
}
}