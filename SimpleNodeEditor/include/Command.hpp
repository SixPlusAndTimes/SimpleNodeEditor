#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include "DataStructureYaml.hpp"
#include "DataStructureEditor.hpp"
namespace SimpleNodeEditor
{

class NodeEditor;
class ICommand
{
    public:
    ICommand() = default;
    virtual ~ICommand() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual void Redo() = 0;
    virtual std::string GetName() const = 0;
};

class AddNodeCommand : public ICommand
{
public:
    AddNodeCommand(NodeEditor& editor, const NodeDescription& nodeDesc, const ImVec2& nodePos);

    void Execute() override;

    void Undo() override;
    void Redo() override;

    std::string GetName() const override { return "AddNodeCommand"; }

private:
    NodeEditor&          m_editor;
    ImVec2               m_nodePos;
    NodeDescription      m_nodeDesc;
    Node                 m_nodeSnapShot;
};

class AddEdgeCommand : public ICommand
{
public:
    AddEdgeCommand(NodeEditor& editor, PortUniqueId startPortId, PortUniqueId endPortId);

    void Execute() override;
    void Undo() override;
    void Redo() override;
    std::string GetName() const override;

private:
    NodeEditor&  m_editor;
    PortUniqueId m_startPortId;
    PortId       m_startPortIndex;
    PortUniqueId m_endPortId;
    PortId       m_endPortIndex;
    EdgeUniqueId m_createdEdgeUid;
};

}

#endif // COMMAND_H