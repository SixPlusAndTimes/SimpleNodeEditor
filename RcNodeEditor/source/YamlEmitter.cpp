#include "YamlEmitter.hpp"

    YAML::Emitter& operator <<(YAML::Emitter& out, const SimpleNodeEditor::YamlPruningRule& pruningRule)
    {
        out << YAML::Key << "group" << YAML::Value << pruningRule.m_Group;
        out << YAML::Key << "type" << YAML::Value << pruningRule.m_Type;
        return out;
    }

    YAML::Emitter& operator <<(YAML::Emitter& out, const std::vector<SimpleNodeEditor::YamlPruningRule>& pruningRules)
    {
        out << YAML::BeginSeq;        
        out << YAML::BeginMap;
        out << YAML::Newline;
        for (const auto& pruneRule : pruningRules)
        {
            out << pruneRule;
        }
        out << YAML::EndMap;
        out << YAML::EndSeq;        
        return out;
    }

    YAML::Emitter& operator <<(YAML::Emitter& out, const SimpleNodeEditor::YamlPort& port)
    {
        out << YAML::Key << "NodeName" << YAML::Value << port.m_nodeName;
        out << YAML::Key << "NodeId" << YAML::Value << port.m_nodeYamlId;
        out << YAML::Key << "PortName" << YAML::Value << port.m_portName;
        out << YAML::Key << "PortId" << YAML::Value << port.m_portYamlId;
        if (!port.m_PruningRules.empty())
        {
            out << YAML::Key << "PruneRule" << YAML::Value << port.m_PruningRules;
        }
        return out;
    }

    YAML::Emitter& operator << (YAML::Emitter& out, const SimpleNodeEditor::YamlNode& yamlnode) {
        out << YAML::Key << "NodeName" << YAML::Value << yamlnode.m_nodeName;
        out << YAML::Key << "NodeId" << YAML::Value << yamlnode.m_nodeYamlId;
        out << YAML::Key << "IsSrcNode" << YAML::Value << yamlnode.m_isSrcNode;
        out << YAML::Key << "NodeType" << YAML::Value << yamlnode.m_nodeYamlType;
        if (!yamlnode.m_Properties.empty())
        {
            // TODO 
        }

        if (!yamlnode.m_PruningRules.empty())
        {
            out << YAML::Key << "PruneRule" << YAML::Value << yamlnode.m_PruningRules;
        }
        return out;
    }

    // YAML::Emitter& operator << (YAML::Emitter& out, const SimpleNodeEditor::YamlEdge& yamlEdge) {
    //     assert(yamlEdge.m_isValid);
    //     out << yamlEdge.m_yamlDstPort;
    //     out << yamlEdge.m_yamlDstPort;
    // }

namespace SimpleNodeEditor
{
YamlEmitter::YamlEmitter() 
: m_Emitter()
{
    // m_Emitter.SetIndent (4);
}

YamlEmitter::YamlEmitter(std::ostream& stream)
: m_Emitter(stream)
{
    // m_Emitter.SetIndent (4);
}

void YamlEmitter::BeginMap()
{
    m_Emitter << YAML::BeginMap;
}

void YamlEmitter::EndMap()
{
    m_Emitter << YAML::EndMap;
}

void YamlEmitter::BeginSequence()
{
    m_Emitter << YAML::BeginSeq;
}

void YamlEmitter::EndSequence()
{
    m_Emitter << YAML::EndSeq;
}

void YamlEmitter::BeginValue()
{
    m_Emitter << YAML::Value;
}

YAML::Emitter& YamlEmitter::GetEmitter()
{
    return m_Emitter;
}

PipelineEmitter::PipelineEmitter() : YamlEmitter() 
{

}

PipelineEmitter::PipelineEmitter(std::ostream& stream) : YamlEmitter(stream)
{

}

void PipelineEmitter::EmitPipeline(const std::string& pipelineName, const std::unordered_map<NodeUniqueId, Node>& nodesMap, 
                                    const std::unordered_map<NodeUniqueId, Node>& prunedNodesMap,
                                    const std::unordered_map<EdgeUniqueId, Edge>& egesMap,
                                    const std::unordered_map<NodeUniqueId, Edge>& prunedEdgesMap)
{
    BeginMap();
    EmitKey("Pipeline"); BeginValue();
    BeginSequence();
    BeginMap();
    GetEmitter() << YAML::Newline;
    EmitKeyValue("pipelinename", pipelineName);

    EmitNodeList(nodesMap, prunedNodesMap);
    EmitLinkList(egesMap, prunedEdgesMap);

    EndMap();
    EndSequence();
    EndMap();
}

void PipelineEmitter::EmitYamlNode(const YamlNode& yamlNode)
{
    GetEmitter() << yamlNode;
}

void PipelineEmitter::EmitNodeList(const std::unordered_map<NodeUniqueId, Node>& nodesMap, const std::unordered_map<NodeUniqueId, Node>& prunedNodesMap)
{
    EmitKey("NodeList"); BeginValue();
    BeginSequence();  // Single sequence for all nodes
    
    // Emit regular nodes
    for (const auto& [_, node] : nodesMap)
    {
        BeginMap();
        GetEmitter() << YAML::Newline;
        EmitYamlNode(node.GetYamlNode());
        EndMap();
    }
    
    // Emit pruned nodes
    for (const auto& [_, nodePruned] : prunedNodesMap)
    {
        BeginMap();
        GetEmitter() << YAML::Newline;
        EmitYamlNode(nodePruned.GetYamlNode());
        EndMap();
    }
    
    EndSequence();
}

// Define equality operator for YamlPort
bool operator==(const YamlPort& lhs, const YamlPort& rhs) {
    return lhs.m_nodeName == rhs.m_nodeName &&
           lhs.m_nodeYamlId == rhs.m_nodeYamlId &&
           lhs.m_portName == rhs.m_portName &&
           lhs.m_portYamlId == rhs.m_portYamlId;
}

struct YamlPortHash {
    size_t operator()(const YamlPort& port) const {
        // Combine hashes of relevant fields
        size_t h1 = std::hash<std::string>()(port.m_nodeName);
        size_t h2 = std::hash<int>()(port.m_nodeYamlId);
        size_t h3 = std::hash<std::string>()(port.m_portName);
        size_t h4 = std::hash<int>()(port.m_portYamlId);
        return h1 ^ (h2 << 8) ^ (h3 << 16) ^ (h4 << 32);
    }
};

std::unordered_map<YamlPort, std::vector<YamlPort>, YamlPortHash> GroupEdges(const std::unordered_map<EdgeUniqueId, Edge>& edgesMap, 
                                                                const std::unordered_map<EdgeUniqueId, Edge>& prunedEdgesMap)
{
    std::unordered_map<YamlPort, std::vector<YamlPort>, YamlPortHash> result;
    auto collect = [](const std::unordered_map<EdgeUniqueId, Edge>& edges, std::unordered_map<YamlPort, std::vector<YamlPort>, YamlPortHash>& resContainer)
    {
        for (const auto&[_, edge] : edges)
        {
            const auto& srcPort = edge.GetYamlEdge().m_yamlSrcPort; 
            const auto& dstPort = edge.GetYamlEdge().m_yamlDstPort; 
            if (resContainer.contains(edge.GetYamlEdge().m_yamlSrcPort))
            {
                resContainer.at(srcPort).push_back(dstPort);
            }
            else
            {
                resContainer.emplace(srcPort, std::vector<YamlPort>{dstPort});
            }
        }
    };
    collect(edgesMap, result);
    collect(prunedEdgesMap, result);
    return result;

}

void PipelineEmitter::EmitYamlEdge(const YamlPort& srcPort, const std::vector<YamlPort>& dstPortVec)
{
    BeginMap();
    GetEmitter() << YAML::Newline;
    // Emit source port
    EmitKey("SrcPort"); BeginValue();
    BeginMap();
    GetEmitter() << srcPort;
    EndMap();
    
    // Emit destination ports
    EmitKey("DstPort"); BeginValue();
    BeginSequence();
    for (const auto& dstPort : dstPortVec) {
        BeginMap();
        GetEmitter() << YAML::Newline;
        GetEmitter() << dstPort;
        EndMap();
    }
    EndSequence();
    
    EndMap();
}

void PipelineEmitter::EmitLinkList(const std::unordered_map<EdgeUniqueId, Edge>& edgesMap, const std::unordered_map<EdgeUniqueId, Edge>& prunedEdgesMap)
{

    auto collecedEdges = GroupEdges(edgesMap, prunedEdgesMap);
    EmitKey("LinkList"); BeginValue();
    BeginSequence();  // Single sequence for all nodes
        for (const auto& [srcPort, dstPortVec] : collecedEdges)
        {
            EmitYamlEdge(srcPort, dstPortVec);
            
        }
    EndSequence();
}


} // namespace SimpleNodeEditor