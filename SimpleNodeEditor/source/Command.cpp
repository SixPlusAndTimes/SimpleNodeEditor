#include "Command.hpp"
#include "imgui.h"
#include "NodeEditor.hpp"
#include "Log.hpp"

namespace SimpleNodeEditor
{

AddNodeCommand::AddNodeCommand(NodeEditor& editor, const NodeDescription& nodeDesc, const ImVec2& nodePos)
: m_editor(editor), m_nodePos(nodePos), m_nodeDesc(nodeDesc) {}

void AddNodeCommand::Execute() 
{
    NodeUniqueId nodeUid = m_editor.AddNewNodes(m_nodeDesc);
    m_editor.SetNodePos(nodeUid, m_nodePos);
    m_nodeSnapShot = m_editor.m_nodes.find(nodeUid)->second;
}

void AddNodeCommand::Undo()
{
    m_nodeSnapShot = m_editor.m_nodes.find(m_nodeSnapShot.GetNodeUniqueId())->second;
    m_nodePos = m_editor.GetNodePos(m_nodeSnapShot.GetNodeUniqueId());

    // should not unregister nodeuid here, otherwise we may lose all information to rebuild some edges
    m_editor.DeleteNode(m_nodeSnapShot.GetNodeUniqueId(), false); 
    SNELOG_INFO("AddNode Redo Done, nodeUid = {}, m_nodes.size() = {}", m_nodeSnapShot.GetNodeUniqueId(), m_editor.m_nodes.size());
}

void AddNodeCommand::Redo()
{
    NodeUniqueId nodeUid = m_editor.RestoreNode(m_nodeSnapShot);
    m_editor.SetNodePos(nodeUid, m_nodePos);
    SNELOG_INFO("AddNode Redo Done, nodeUid {}, m_nodes.size() = {}", nodeUid, m_editor.m_nodes.size());
}


AddEdgeCommand::AddEdgeCommand(NodeEditor& editor, PortUniqueId startPortId, PortUniqueId endPortId)
    : m_editor(editor), m_startPortUId(startPortId), m_endPortUId(endPortId), m_createdEdgeUid(-1)
{}

void AddEdgeCommand::Execute()
{
    m_createdEdgeUid = m_editor.AddNewEdge(m_startPortUId, m_endPortUId);
    if (m_createdEdgeUid != -1)
    {
        // Capture snapshot copy after edge creation for redo
        m_edgeSnapshot = m_editor.m_edges.find(m_createdEdgeUid)->second;
    }
    else 
    {
        SNELOG_ERROR("AddEdgeCommand Execute failed: startPortUid {} endPortUid {}", m_startPortUId, m_endPortUId);
    }
}

void AddEdgeCommand::Undo()
{
    if (m_createdEdgeUid == -1) return;
    m_edgeSnapshot = m_editor.m_edges.find(m_createdEdgeUid)->second;
    m_editor.DeleteEdge(m_createdEdgeUid, false);
}

void AddEdgeCommand::Redo()
{
    if (m_createdEdgeUid == -1) return;
    // Restore edge with original UID using the snapshot copy
    m_editor.RestoreEdge(m_edgeSnapshot);
}

std::string AddEdgeCommand::GetName() const
{
    return "AddEdgeCommand";
}

} // namespace SimpleNodeEditor