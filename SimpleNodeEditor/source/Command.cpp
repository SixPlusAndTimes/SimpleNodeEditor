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
    SNELOG_INFO("AddNode Execute Done, nodeUid = {}, m_nodes.size() = {}", m_nodeSnapShot.GetNodeUniqueId(), m_editor.m_nodes.size());
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
        SNELOG_INFO("AddEdgeCommand Execute Success: startPortUid {} endPortUid {} edgeUid {}", m_startPortUId, m_endPortUId, m_createdEdgeUid);
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

DeleteEdgeCommand::DeleteEdgeCommand(NodeEditor& editor, EdgeUniqueId edgeUid)
    : m_editor(editor), m_deletedEdgeUid(edgeUid), m_isActuallyDeleted(false)
{}


DeleteEdgeCommand::~DeleteEdgeCommand()
{
    if (m_isActuallyDeleted)
    {
        m_editor.m_edgeUidGenerator.UnregisterUniqueID(m_edgeSnapshot.GetEdgeUniqueId());
    }
}

void DeleteEdgeCommand::Execute()
{
    if (m_deletedEdgeUid == -1) return;
    // Capture snapshot before deleting the edge
    m_edgeSnapshot = m_editor.m_edges.find(m_deletedEdgeUid)->second;
    m_editor.DeleteEdge(m_deletedEdgeUid, false);
    m_isActuallyDeleted = true;
}

void DeleteEdgeCommand::Undo()
{
    if (m_deletedEdgeUid == -1) return;
    // Restore the edge with its original UID using the snapshot
    m_editor.RestoreEdge(m_edgeSnapshot);
    m_isActuallyDeleted = false;
}


void DeleteEdgeCommand::Redo()
{
    if (m_deletedEdgeUid == -1) return;
    m_editor.DeleteEdge(m_deletedEdgeUid, false);
    m_isActuallyDeleted = true;
}

std::string DeleteEdgeCommand::GetName() const
{
    return "DeleteEdgeCommand";
}

} // namespace SimpleNodeEditor