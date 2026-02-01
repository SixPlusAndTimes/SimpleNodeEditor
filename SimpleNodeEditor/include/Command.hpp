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

    ICommand(const ICommand&) = default;
    ICommand(ICommand&&) = default;
    ICommand& operator=(const ICommand&) = default;
    ICommand& operator=(ICommand&&) = default;

    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual void Redo() = 0;
    virtual std::string ToString() const = 0;
};

class AddNodeCommand : public ICommand
{
public:
    AddNodeCommand(NodeEditor& editor, const NodeDescription& nodeDesc, const ImVec2& nodePos);

    virtual ~AddNodeCommand();
    AddNodeCommand(const AddNodeCommand&) = default;
    AddNodeCommand(AddNodeCommand&&) = default;
    AddNodeCommand& operator=(const AddNodeCommand&) = default;
    AddNodeCommand& operator=(AddNodeCommand&&) = default;

    void Execute() override;

    void Undo() override;
    void Redo() override;

    std::string ToString() const override;

private:
    NodeEditor&          m_editor;
    ImVec2               m_nodePos;
    NodeDescription      m_nodeDesc;
    Node                 m_nodeSnapShot;
    bool                 m_isActuallyAdded;
};

class AddEdgeCommand : public ICommand
{
public:
    AddEdgeCommand(NodeEditor& editor, PortUniqueId startPortId, PortUniqueId endPortId);

    virtual ~AddEdgeCommand();
    AddEdgeCommand(const AddEdgeCommand&) = default;
    AddEdgeCommand(AddEdgeCommand&&) = default;
    AddEdgeCommand& operator=(const AddEdgeCommand&) = default;
    AddEdgeCommand& operator=(AddEdgeCommand&&) = default;

    void Execute() override;
    void Undo() override;
    void Redo() override;
    std::string ToString() const override;

private:
    NodeEditor&  m_editor;
    PortUniqueId m_startPortUId;
    PortUniqueId m_endPortUId;
    EdgeUniqueId m_createdEdgeUid;
    Edge         m_edgeSnapshot;  // Store actual snapshot copy, not pointer
    bool         m_isAcltuallyAdded;
};

class DeleteEdgeCommand : public ICommand
{
public:
    DeleteEdgeCommand(NodeEditor& editor, EdgeUniqueId edgeUid);

    virtual ~DeleteEdgeCommand();
    DeleteEdgeCommand(const DeleteEdgeCommand&) = default;
    DeleteEdgeCommand(DeleteEdgeCommand&&) = default;
    DeleteEdgeCommand& operator=(const DeleteEdgeCommand&) = default;
    DeleteEdgeCommand& operator=(DeleteEdgeCommand&&) = default;

    void Execute() override;
    void Undo() override;
    void Redo() override;
    std::string ToString() const override;

private:
    NodeEditor&  m_editor;
    EdgeUniqueId m_deletedEdgeUid;
    bool         m_isActuallyDeleted;
    Edge         m_edgeSnapshot;  // Store snapshot for undo
};

class DeleteNodeCommand : public ICommand
{
public:
    DeleteNodeCommand(NodeEditor& editor, NodeUniqueId nodeUid);

    virtual ~DeleteNodeCommand();
    DeleteNodeCommand(const DeleteNodeCommand&) = default;
    DeleteNodeCommand(DeleteNodeCommand&&) = default;
    DeleteNodeCommand& operator=(const DeleteNodeCommand&) = default;
    DeleteNodeCommand& operator=(DeleteNodeCommand&&) = default;

    void Execute() override;
    void Undo() override;
    void Redo() override;
    std::string ToString() const override;

private:
    NodeEditor&             m_editor;
    NodeUniqueId            m_deletedNodeUid;
    Node                    m_nodeSnapshot;              // Store snapshot for undo
    ImVec2                  m_nodePos;
    std::vector<Edge>       m_deletedEdgeSnapshots;      // Store snapshots of deleted edges
    bool                    m_isActuallyDeleted;
};

}

#endif // COMMAND_H