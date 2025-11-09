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
        out << YAML:: BeginMap;
        out << YAML::Newline;
        for (const auto& pruneRule : pruningRules)
        {
            out << pruneRule;
        }
        out << YAML:: EndMap;
        out << YAML::EndSeq;        
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
        EmitYamlNode(node.GetYamlNode());
        EndMap();
        GetEmitter() << YAML::Newline;
    }
    
    // Emit pruned nodes
    for (const auto& [_, nodePruned] : prunedNodesMap)
    {
        BeginMap();
        EmitYamlNode(nodePruned.GetYamlNode());
        EndMap();
        GetEmitter() << YAML::Newline;
    }
    
    EndSequence();
}



void PipelineEmitter::EmitLinkList(const std::unordered_map<EdgeUniqueId, Edge>& edgesMap, const std::unordered_map<EdgeUniqueId, Edge>& prunedEdgesMap)
{
    EmitKey("LinkList"); BeginValue();
    BeginSequence();
        for (const auto& edge : edgesMap)
        {
            EmitYamlEdge(edge.second.GetYamlEdge());
        }
    EndSequence();
}


} // namespace SimpleNodeEditor