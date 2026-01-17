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
    ImNodes::SetNodeScreenSpacePos(nodeUid, m_nodePos);
    m_nodeSnapShot = m_editor.m_nodes.find(nodeUid)->second;
}

void AddNodeCommand::Undo()
{
    m_nodeSnapShot = m_editor.m_nodes.find(m_nodeSnapShot.GetNodeUniqueId())->second;
    m_nodePos = m_editor.GetNodePos(m_nodeSnapShot.GetNodeUniqueId());

    // should not unregister nodeuid here, otherwise we may lose all information to rebuild some edges
    m_editor.DeleteNode(m_nodeSnapShot.GetNodeUniqueId(), false); 
}

void AddNodeCommand::Redo()
{
    NodeUniqueId nodeUid = m_editor.RestoreNode(m_nodeSnapShot);
    ImNodes::SetNodeScreenSpacePos(m_nodeSnapShot.GetNodeUniqueId(), m_nodePos);
    SNELOG_INFO("AddNode Redo Done, nodeUid {}, m_nodes.size() = {}", nodeUid, m_editor.m_nodes.size());
}


AddEdgeCommand::AddEdgeCommand(NodeEditor& editor, PortUniqueId startPortId, PortUniqueId endPortId)
    : m_editor(editor), m_startPortId(startPortId), m_endPortId(endPortId), m_createdEdgeUid(-1)
    , m_startPortIndex(-1), m_endPortIndex(-1)
{}

void AddEdgeCommand::Execute()
{
    m_createdEdgeUid = m_editor.AddNewEdge(m_startPortId, m_endPortId);
    m_startPortIndex =  m_editor.m_outportPorts.find(m_startPortId)->second->GetPortId();
    m_endPortIndex =  m_editor.m_inportPorts.find(m_endPortId)->second->GetPortId();
}

void AddEdgeCommand::Undo()
{
    if (m_createdEdgeUid == -1) return;
    m_editor.DeleteEdge(m_createdEdgeUid, false);

}

void AddEdgeCommand::Redo()
{
    Execute();
}

std::string AddEdgeCommand::GetName() const
{
    return "AddEdgeCommand";
}

} // namespace SimpleNodeEditor