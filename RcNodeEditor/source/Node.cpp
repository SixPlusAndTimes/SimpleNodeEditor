#include "Node.hpp"
#include <algorithm>
#include "spdlog/spdlog.h"
#include <cmath>
#include "imnodes.h"

namespace SimpleNodeEditor
{

Port::Port(PortUniqueId portUid, PortId portId, const std::string& name, NodeUniqueId ownedBy,
           YamlPort::PortYamlId portYamlId)
    : m_portUid(portUid),
      m_portId(portId),
      m_portName(name),
      m_ownedByNodeUid(ownedBy),
      m_portYamlId(portYamlId)
{
}

YamlPort::PortYamlId Port::GetPortYamlId() const
{
    return m_portYamlId;
}

void Port::SetPortId(PortId portId)
{
    m_portId = portId;
}
void Port::SetPortname(const std::string& name)
{
    m_portName = name;
}

std::string_view Port::GetPortname() const
{
    return m_portName;
}

Port::PortId Port::GetPortId() const
{
    return m_portId;
}

PortUniqueId Port::GetPortUniqueId() const
{
    return m_portUid;
}

NodeUniqueId Port::OwnedByNodeUid() const
{
    return m_ownedByNodeUid;
}

InputPort::InputPort(PortUniqueId portUid, PortId portId, const std::string& name,
                     NodeUniqueId ownedBy, YamlPort::PortYamlId portYamlId)
    : Port(portUid, portId, name, ownedBy, portYamlId), m_linkFrom(-1)
{
    SPDLOG_INFO("InputPort construced with portUid = {}, portId = {}, portName = {}, linkfrom = {}",
                portUid, portId, name, m_linkFrom);
}

void InputPort::SetEdgeUid(EdgeUniqueId edgeUid)
{
    m_linkFrom = edgeUid;
    SPDLOG_INFO("InputPort id = {}, setedgeuid = {}", GetPortUniqueId(), edgeUid);
}

EdgeUniqueId InputPort::GetEdgeUid()
{
    return m_linkFrom;
}

EdgeUniqueId InputPort::GetEdgeUid() const
{
    return m_linkFrom;
}

bool InputPort::HasNoEdgeLinked()
{
    return m_linkFrom == -1;
}

OutputPort::OutputPort(PortUniqueId portUid, PortId portId, const std::string& name,
                       NodeUniqueId ownedBy, YamlPort::PortYamlId portYamlId)
    : Port(portUid, portId, name, ownedBy, portYamlId), m_linkTos()
{
    SPDLOG_INFO("OutputPort construced with portUid = {}, portId = {}, portName = {}", portUid,
                portId, name);
}

bool OutputPort::HasNoEdgeLinked() 
{
    return m_linkTos.size() == 0;
}

Edge::Edge(PortUniqueId sourcePortUid, PortUniqueId destinationPortUid, EdgeUniqueId edgeUid,
           YamlEdge yamlEdge)
    : m_srcPortUid(sourcePortUid),
      m_dstPortUid(destinationPortUid),
      m_edgeUid(edgeUid),
      m_yamlEdge(yamlEdge)
{
}

Edge::Edge(PortUniqueId sourcePortUid, NodeUniqueId sourceNodeUid, PortUniqueId destinationPortUid,
           NodeUniqueId destinationNodeUid, EdgeUniqueId edgeUid, YamlEdge yamlEdge)
    : m_srcPortUid(sourcePortUid),
      m_srcNodeUid(sourceNodeUid),
      m_dstPortUid(destinationPortUid),
      m_dstNodeUid(destinationNodeUid),
      m_edgeUid(edgeUid),
      m_yamlEdge(yamlEdge)
{
}

YamlEdge& Edge::GetYamlEdge()
{
    return m_yamlEdge;
}

const YamlEdge& Edge::GetYamlEdge() const
{
    return m_yamlEdge;
}

EdgeUniqueId Edge::GetEdgeUniqueId() const
{
    return m_edgeUid;
}

void Edge::SetSourceNodeUid(NodeUniqueId nodeUid)
{
    m_srcNodeUid = nodeUid;
}

void Edge::SetDestinationNodeUid(NodeUniqueId nodeUid)
{
    m_dstNodeUid = nodeUid;
}

PortUniqueId Edge::GetSourcePortUid() const
{
    return m_srcPortUid;
}

PortUniqueId Edge::GetDestinationPortUid() const
{
    return m_dstPortUid;
}

NodeUniqueId Edge::GetSourceNodeUid() const
{
    return m_srcNodeUid;
}

NodeUniqueId Edge::GetDestinationNodeUid() const
{
    return m_dstNodeUid;
}

Node::Node(NodeUniqueId nodeUid, NodeType nodeType, const YamlNode& yamlNode,
           const std::string& nodeTitle, ImNodesStyle& nodeStyle)
    : m_nodeUid(nodeUid),
      m_nodeType(nodeType),
      m_nodeWidth(),
      m_nodeTitle(nodeTitle),
      m_nodePos(),
      m_inputPorts(),
      m_outputPorts(),
      m_yamlNodeId(yamlNode.m_nodeYamlId),
      m_yamlNode(yamlNode),
      m_nodeStyle(nodeStyle)
{
    SPDLOG_INFO("Node constructed with nodeUid = {}, ymalNodeId = {}, nodeTtile = {}", m_nodeUid,
                m_yamlNodeId, m_nodeTitle);
}

void Node::CalcNodeWidth()
{
    // if (m_nodeWidth.has_value())
    // {
    //     return ;
    // }
    // float maxInportNameLength = 0.f;
    // float maxOutportNameLength = 0.f;
    // for (InputPort inport : m_inputPorts)
    // {
    //     maxInportNameLength = std::max(maxInportNameLength, ImGui::CalcTextSize(inport.GetPortname().data()).x) ;
    // }
    // for (OutputPort outport : m_outputPorts)
    // {
    //     maxOutportNameLength = std::max(maxOutportNameLength, ImGui::CalcTextSize(outport.GetPortname().data()).x) ;
    // }

    // m_nodeWidth = maxInportNameLength + maxOutportNameLength + m_nodeStyle.NodePadding.x * 2 + m_nodeStyle.PinOffset * 2;
    // SPDLOG_INFO("calc nodeuid[{}] nodewidth[{}]", m_nodeUid, m_nodeWidth.value());
}

float Node::GetNodeWidth()
{
    CalcNodeWidth();
    assert(m_nodeWidth.has_value());
    return m_nodeWidth.value();
}

std::vector<EdgeUniqueId> Node::GetAllEdges() const
{
    std::vector<EdgeUniqueId> edges{};

    for (const InputPort& inPort : m_inputPorts)
    {
        if (inPort.GetEdgeUid() != -1)
        {
            edges.push_back(inPort.GetEdgeUid());
        }
    }

    for (const OutputPort& outPort : m_outputPorts)
    {
        if (outPort.GetEdgeUids().size() != 0)
        {
            edges.insert(edges.end(), outPort.GetEdgeUids().begin(), outPort.GetEdgeUids().end());
        }
    }

    return edges;
}

YamlNode& Node::GetYamlNode()
{
    return m_yamlNode;
}

const YamlNode& Node::GetYamlNode() const
{
    return m_yamlNode;
}

void Node::SetNodeYamlId(YamlNode::NodeYamlId nodeYamlId)
{
    m_yamlNodeId = nodeYamlId;
}

PortUniqueId Node::FindPortUidAmongOutports(YamlPort::PortYamlId portYamlId) const
{
    auto iter = std::find_if(m_outputPorts.begin(), m_outputPorts.end(),
                             [portYamlId](const OutputPort& outPutPort)
                             { return outPutPort.GetPortYamlId() == portYamlId; });
    if (iter != m_outputPorts.end())
    {
        return iter->GetPortUniqueId();
    }
    else
    {
        SPDLOG_ERROR("cannot find outportuid, by the portYamlId[{}] in the nodeUid[{}]", portYamlId,
                     m_nodeUid);
        return -1;
    }
}

PortUniqueId Node::FindPortUidAmongInports(YamlPort::PortYamlId portYamlId) const
{
    auto iter = std::find_if(m_inputPorts.begin(), m_inputPorts.end(),
                             [portYamlId](const InputPort& inPutPort)
                             { return inPutPort.GetPortYamlId() == portYamlId; });
    if (iter != m_inputPorts.end())
    {
        return iter->GetPortUniqueId();
    }
    else
    {
        SPDLOG_ERROR("cannot find inportuid, by the portYamlId[{}] in the nodeUid[{}]", portYamlId,
                     m_nodeUid);
        return -1;
    }
}

void Node::SetNodePosition(const ImVec2& pos)
{
    m_nodePos = pos;
}

void Node::AddInputPort(const InputPort& inPort)
{
    m_inputPorts.push_back(inPort);
}

void Node::AddOutputPort(const OutputPort& outPort)
{
    m_outputPorts.push_back(outPort);
}

NodeUniqueId Node::GetNodeUniqueId() const
{
    return m_nodeUid;
}

void Node::SetNodeTitle(const std::string& nodeTitle)
{
    m_nodeTitle = nodeTitle;
}

const std::string_view Node::GetNodeTitle() const
{
    return m_nodeTitle;
}

YamlNode::NodeYamlId Node::GetNodeYamlId() const
{
    return m_yamlNodeId;
}

const std::vector<InputPort>& Node::GetInputPorts() const
{
    return m_inputPorts;
}

std::vector<InputPort>& Node::GetInputPorts()
{
    return m_inputPorts;
}

InputPort* Node::GetInputPort(PortUniqueId portUid)
{
    for (auto& inport : m_inputPorts)
    {
        if (inport.GetPortUniqueId() == portUid)
        {
            return &inport;
        }
    }

    SPDLOG_ERROR("Can not find portUid = {} in Nodeuid = {}, check it!", portUid, m_nodeUid);
    return nullptr;
}

const std::vector<OutputPort>& Node::GetOutputPorts() const
{
    return m_outputPorts;
}

std::vector<OutputPort>& Node::GetOutputPorts()
{
    return m_outputPorts;
}

void OutputPort::PushEdge(EdgeUniqueId edgeUid)
{
    SPDLOG_INFO("outport id[{}], push edge id[{}]", GetPortUniqueId(), edgeUid);
    m_linkTos.push_back(edgeUid);
}

void OutputPort::DeletEdge(EdgeUniqueId edgeUid)
{
    auto iter = std::find_if(m_linkTos.begin(), m_linkTos.end(), [edgeUid](EdgeUniqueId traverseEle)
                             { return edgeUid == traverseEle; });

    if (iter != m_linkTos.end())
    {
        m_linkTos.erase(iter);
    }
}

const std::vector<EdgeUniqueId>& OutputPort::GetEdgeUids() const
{
    return m_linkTos;
}

void OutputPort::ClearEdges()
{
    std::vector<EdgeUniqueId>().swap(m_linkTos);
}

OutputPort* Node::GetOutputPort(PortUniqueId portUid)
{
    auto iter = std::find_if(m_outputPorts.begin(), m_outputPorts.end(),
                             [portUid](const OutputPort& outPort)
                             { return portUid == outPort.GetPortUniqueId(); });

    if (iter != m_outputPorts.end())
    {
        return &(*iter);
    }
    else
    {
        SPDLOG_ERROR("cannot find outport, check it! portUid = {}", portUid);
        return nullptr;
    }
}
} // namespace SimpleNodeEditor
