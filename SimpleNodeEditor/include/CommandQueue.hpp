#ifndef COMMANDQUEUE_H
#define COMMANDQUEUE_H

#include "Command.hpp"
#include <array>
#define COMMANDQUEUE_SIZE 30
namespace SimpleNodeEditor
{
    
class CommandQueue
{
public:
    CommandQueue();
    ~CommandQueue() = default;
    CommandQueue(const CommandQueue&) = delete;
    CommandQueue(CommandQueue&&) = delete;
    CommandQueue& operator=(const CommandQueue&) = delete;
    CommandQueue& operator=(CommandQueue&&) = delete;

    void AddAndExecuteCommad(std::unique_ptr<ICommand> cmd);
    bool Undo();
    bool Redo();

    void Clear();
    std::string ToString();
private:
    std::array<std::unique_ptr<ICommand>, COMMANDQUEUE_SIZE> m_queue;
    size_t m_current; // point to the position where the next commad will be placed at

};
} // namespace SimpleNodeEditor

#endif // COMMANDQUEUE_H