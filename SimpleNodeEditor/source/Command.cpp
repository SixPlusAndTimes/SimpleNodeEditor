#include "Command.hpp"
#include "imgui.h"
#include "NodeEditor.hpp"
#include "Log.hpp"

namespace SimpleNodeEditor
{

AddNodeCommand::AddNodeCommand(NodeEditor& editor, const NodeDescription& nodeDesc, const ImVec2& nodePos)
: m_editor(editor), m_nodePos(nodePos), m_nodeDesc(nodeDesc), m_isActuallyAdded(false) {}

AddNodeCommand::~AddNodeCommand()
{
    // TODO: see if nodeeditor actually added the node, if not we unregister the nodeuid and the related edgeuid
    if (!m_isActuallyAdded)
    {
        m_editor.m_nodeUidGenerator.UnregisterUniqueID(m_nodeSnapShot.GetNodeUniqueId());
    }
}

void AddNodeCommand::Execute() 
{
    NodeUniqueId nodeUid = m_editor.AddNewNodes(m_nodeDesc);
    m_editor.SetNodePos(nodeUid, m_nodePos);
    m_nodeSnapShot = m_editor.m_nodes.find(nodeUid)->second;
    m_isActuallyAdded = true;
    SNELOG_INFO("AddNode Execute Done, nodeUid = {}, m_nodes.size() = {}", m_nodeSnapShot.GetNodeUniqueId(), m_editor.m_nodes.size());
}

void AddNodeCommand::Undo()
{
    m_nodeSnapShot = m_editor.m_nodes.find(m_nodeSnapShot.GetNodeUniqueId())->second;
    m_nodePos = m_editor.GetNodePos(m_nodeSnapShot.GetNodeUniqueId());

    // should not unregister nodeuid here, otherwise we may lose all information to rebuild some edges
    m_editor.DeleteNode(m_nodeSnapShot.GetNodeUniqueId(), false); 
    m_isActuallyAdded = false;
    SNELOG_INFO("AddNode Undo Done, nodeUid = {}, m_nodes.size() = {}", m_nodeSnapShot.GetNodeUniqueId(), m_editor.m_nodes.size());
}

void AddNodeCommand::Redo()
{
    NodeUniqueId nodeUid = m_editor.RestoreNode(m_nodeSnapShot);
    m_editor.SetNodePos(nodeUid, m_nodePos);
    m_isActuallyAdded = true;
    SNELOG_INFO("AddNode Redo Done, nodeUid {}, m_nodes.size() = {}", nodeUid, m_editor.m_nodes.size());
}

std::string AddNodeCommand::ToString() const 
{
     return "AddNodeCommand, NodeUid is " + std::to_string(m_nodeSnapShot.GetNodeUniqueId()); 
}

AddEdgeCommand::~AddEdgeCommand()
{
    if (!m_isAcltuallyAdded)
    {
        m_editor.m_edgeUidGenerator.UnregisterUniqueID(m_createdEdgeUid);
    }
}

AddEdgeCommand::AddEdgeCommand(NodeEditor& editor, PortUniqueId startPortId, PortUniqueId endPortId)
    : m_editor(editor), m_startPortUId(startPortId), m_endPortUId(endPortId), m_createdEdgeUid(-1), m_isAcltuallyAdded(false)
{ }

void AddEdgeCommand::Execute()
{
    // allow multiple inputports here
    m_createdEdgeUid = m_editor.AddNewEdge(m_startPortUId, m_endPortUId, {}, false); 
    if (m_createdEdgeUid != -1)
    {
        // Capture snapshot copy after edge creation for redo
        m_edgeSnapshot = m_editor.m_edges.find(m_createdEdgeUid)->second;
        SNELOG_INFO("AddEdgeCommand Execute Success: startPortUid {} endPortUid {} edgeUid {}", m_startPortUId, m_endPortUId, m_createdEdgeUid);
        m_isAcltuallyAdded = true;
    }
    else 
    {
        m_isAcltuallyAdded = false;
        SNELOG_ERROR("AddEdgeCommand Execute failed: startPortUid {} endPortUid {}", m_startPortUId, m_endPortUId);
    }
}

void AddEdgeCommand::Undo()
{
    if (m_createdEdgeUid == -1) return;
    m_edgeSnapshot = m_editor.m_edges.find(m_createdEdgeUid)->second;
    m_editor.DeleteEdge(m_createdEdgeUid, false);
    m_isAcltuallyAdded = false;
}

void AddEdgeCommand::Redo()
{
    if (m_createdEdgeUid == -1) return;
    // Restore edge with original UID using the snapshot copy
    m_editor.RestoreEdge(m_edgeSnapshot);
    m_isAcltuallyAdded = true;
}

std::string AddEdgeCommand::ToString() const 
{
    return std::format("AddEdgeCommand, EdgeUid is {} sourceNodeUid is {}, sourcePortUid is {} dstNodeUid is {}, dstPortUid is {}",
                                        m_createdEdgeUid, m_edgeSnapshot.GetSourceNodeUid(), m_edgeSnapshot.GetSourcePortUid(), 
                                        m_edgeSnapshot.GetDestinationNodeUid(), m_edgeSnapshot.GetDestinationPortUid());
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
    Execute();
}

std::string DeleteEdgeCommand::ToString() const 
{
    return std::format("DeleteEdgeCommand, EdgeUid is{} sourceNodeUid is {}, sourcePortUid is {} dstNodeUid is {}, dstPortUid is {}",
                                        m_deletedEdgeUid, m_edgeSnapshot.GetSourceNodeUid(), m_edgeSnapshot.GetSourcePortUid(), 
                                        m_edgeSnapshot.GetDestinationNodeUid(), m_edgeSnapshot.GetDestinationPortUid());
}

DeleteNodeCommand::DeleteNodeCommand(NodeEditor& editor, NodeUniqueId nodeUid)
    : m_editor(editor), m_deletedNodeUid(nodeUid), m_nodeSnapshot(), m_nodePos(), m_deletedEdgeSnapshots(), m_isActuallyDeleted(false)
{ }

DeleteNodeCommand::~DeleteNodeCommand()
{
    if (m_isActuallyDeleted)
    {
        m_editor.m_nodeUidGenerator.UnregisterUniqueID(m_deletedNodeUid);
        for (const auto& edge : m_deletedEdgeSnapshots)
        {
           m_editor.m_edgeUidGenerator.UnregisterUniqueID(edge.GetEdgeUniqueId());
        }
    }
}

void DeleteNodeCommand::Execute()
{
    if (m_deletedNodeUid == -1) return;
    // Capture snapshot before deleting the node
    auto it = m_editor.m_nodes.find(m_deletedNodeUid);
    if (it != m_editor.m_nodes.end())
    {
        m_nodeSnapshot = it->second;
        
        // Also capture all edges connected to this node before deletion
        for (InputPort& inPort : m_nodeSnapshot.GetInputPorts())
        {
            EdgeUniqueId edgeUid = inPort.GetEdgeUid();
            if (edgeUid != -1)
            {
                auto edgeIt = m_editor.m_edges.find(edgeUid);
                if (edgeIt != m_editor.m_edges.end())
                {
                    m_deletedEdgeSnapshots.push_back(edgeIt->second);
                }
            }
            inPort.SetEdgeUid(-1);
        }
        
        for (OutputPort& outPort : m_nodeSnapshot.GetOutputPorts())
        {
            const auto& edgeUids = outPort.GetEdgeUids();
            for (EdgeUniqueId edgeUid : edgeUids)
            {
                if (edgeUid != -1)
                {
                    auto edgeIt = m_editor.m_edges.find(edgeUid);
                    if (edgeIt != m_editor.m_edges.end())
                    {
                        m_deletedEdgeSnapshots.push_back(edgeIt->second);
                    }
                }
            }
            outPort.ClearEdges();
        }
        
        m_nodePos = m_editor.GetNodePos(m_deletedNodeUid);
        m_editor.DeleteNode(m_deletedNodeUid, false);
        m_isActuallyDeleted = true;
    }
}

void DeleteNodeCommand::Undo()
{
    if (m_deletedNodeUid == -1) return;
    // Restore the node with its original UID using the snapshot
    m_editor.RestoreNode(m_nodeSnapshot);
    m_editor.SetNodePos(m_deletedNodeUid, m_nodePos);
    // Restore all connected edges
    for (const Edge& edgeSnapshot : m_deletedEdgeSnapshots)
    {
        m_editor.RestoreEdge(edgeSnapshot);
    }
    m_deletedEdgeSnapshots.clear();
    m_isActuallyDeleted = false;
}

void DeleteNodeCommand::Redo()
{
    if (m_deletedNodeUid == -1) return;
    Execute();
}

std::string DeleteNodeCommand::ToString() const
{
    return std::format("DeleteNodeCommand, NodeUid is {}", m_deletedNodeUid);
}

} // namespace SimpleNodeEditor