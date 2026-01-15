#include "Command.hpp"
#include "imgui.h"
#include "NodeEditor.hpp"

namespace SimpleNodeEditor
{
    AddNodeCommand::AddNodeCommand(NodeEditor& editor, const NodeDescription& nodeDesc, const ImVec2& nodePos)
    : m_editor(editor), m_nodeDesc(nodeDesc), m_nodePos(nodePos), m_createdNodeUid(-1) {}

    void AddNodeCommand::Execute() 
    {
        m_createdNodeUid = m_editor.AddNewNodes(m_nodeDesc);
        ImNodes::SetNodeScreenSpacePos(m_createdNodeUid, m_nodePos);
    }

    void AddNodeCommand::Undo()
    {
        if (m_createdNodeUid == -1) return;
        m_editor.DeleteNode(m_createdNodeUid, false);
    }

    void AddNodeCommand::Redo()
    {
        m_createdNodeUid = m_editor.AddNewNodes(m_nodeDesc);
        ImNodes::SetNodeScreenSpacePos(m_createdNodeUid, m_nodePos);
    }
} // namespace SimpleNodeEditor

