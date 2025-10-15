#include "spdlog/spdlog.h"
#include "YamlParser.hpp"

namespace SimpleNodeEditor
{
    
bool YamlParser::LoadFile(const std::string& filePath)
{
    m_rootNode = YAML::LoadFile(filePath);
    if (m_rootNode)
    {
        m_filePath = filePath;
        return true;
    }
    else
    {
        return false;
    }
}


ConfigParser::ConfigParser(const std::string& filePath)
{
    // is it useful to check LoadFile here?
    if (LoadFile(filePath))
    {
        SPDLOG_INFO("ConfigParser LoadFile success : {}", filePath);
    }
    else 
    {
        SPDLOG_ERROR("ConfigParser failed : {}", filePath);
    }
}

NodeDescriptionParser::NodeDescriptionParser(const std::string& filePath)
{
    if (LoadFile(filePath))
    {
        SPDLOG_INFO("NodeDescriptionParser LoadFile success : {}", filePath);
    }
    else 
    {
        SPDLOG_ERROR("ConfigParser LoadFile failed : {}", filePath);
    }
}

std::vector<NodeDescription> NodeDescriptionParser::ParseNodeDescriptions()
{
    std::vector<NodeDescription> ret;

    // 1. whether m_rootNode is valid 
    if (!m_rootNode)
    {
        SPDLOG_ERROR("rootNode is not invalid");
        return ret;
    }


    // 2. whether NodeDescriptions Node exists in the yaml file
    if (!m_rootNode["NodeDescriptions"])
    {
        SPDLOG_ERROR("Cannot find NodeDescriptions in yaml file, check it!");
        return ret;
    }
    // 3. whether NodeDescriptions Node is a sequence ndoe 
    YAML::Node nodeDescriptions = m_rootNode["NodeDescriptions"];
    if (!nodeDescriptions.IsSequence())
    {
        SPDLOG_ERROR("NodeDescription is not a sequence, check it!");
        return ret;
    }

    // 4 iterate through all node descriptions
    for (const auto& node : nodeDescriptions) {
        NodeDescription desc;
        // parse NodeName
        if (node["NodeName"] && node["NodeName"].IsScalar()) {
            desc.m_nodeName = node["NodeName"].as<std::string>();
        } else {
            SPDLOG_ERROR("Skipping node: missing or invalid NodeName");
            continue;
        }

        // parse InputPorts
        if (node["InputPorts"] && node["InputPorts"].IsSequence()) {
            for (const auto& port : node["InputPorts"]) {
                if (port.IsScalar()) {
                    desc.m_inputPortNames.push_back(port.as<std::string>());
                } else {
                    SPDLOG_ERROR("Skipping invalid InputPort in node: {}", desc.m_nodeName);
                }
            }
        } else {
            SPDLOG_ERROR("Node {} has no valid InputPorts sequence", desc.m_nodeName);
        }
        // parse OutputPorts
        if (node["OutputPorts"] && node["OutputPorts"].IsSequence()) {
            for (const auto& port : node["OutputPorts"]) {
                if (port.IsScalar()) {
                    desc.m_outputPortNames.push_back(port.as<std::string>());
                } else {
                    SPDLOG_ERROR("Skipping invalid OutputPort in node: {}", desc.m_nodeName);
                }
            }
        } else {
            SPDLOG_ERROR("Node {} has no valid OutputPorts sequence", desc.m_nodeName);
        }
        ret.push_back(std::move(desc));
    }

    return ret;
}

PipelineParser::PipelineParser()
{


}

bool PipelineParser::LoadFile(const std::string& filePath)
{

    bool ret = true;
    m_rootNode = YAML::LoadFile(filePath);
    if (m_rootNode && m_rootNode["Pipeline"] && m_rootNode["Pipeline"].IsSequence())
    {
        m_filePath = filePath;
        m_rootNode = m_rootNode["Pipeline"][0];
        // std::string pipeLineName = m_rootNode["pipelinename"].as<std::string>();
    }
    else
    {
        SPDLOG_ERROR("load file[{}] failed", filePath);
        ret = false;
    }

    if (ret && m_rootNode["NodeList"] && m_rootNode["NodeList"].IsSequence())
    {
        m_nodeListNode = m_rootNode["NodeList"];
    }
    else 
    {
        SPDLOG_ERROR("file {} has no valid Node sequence, check it", filePath);
        ret = false;
    }

    if (ret && m_rootNode["LinkList"] && m_rootNode["LinkList"].IsSequence())
    {
        m_edgeListNode = m_rootNode["LinkList"];
    }
    else 
    {
        SPDLOG_ERROR("file {} has no valid List sequence, check it", filePath);
        ret = false;
    }

    return ret;
}

std::vector<YamlNode> PipelineParser::ParseNodes()
{
    SPDLOG_INFO("start to parse node from file[{}], m_nodeListNode.size() = {}",  m_filePath, m_nodeListNode.size());
    std::vector<YamlNode> result;

    for (YAML::const_iterator iter = m_nodeListNode.begin(); iter != m_nodeListNode.end(); ++iter)
    {
        result.push_back(iter->as<YamlNode>());
    }
    SPDLOG_INFO("ParseNodes Dode, collected {} nodes", result.size());
    return result;
}


// do we need more syntax check ?
std::vector<YamlEdge> PipelineParser::ParseEdges()
{
    std::vector<YamlEdge> result;
    if (!m_edgeListNode)
    {
        SPDLOG_ERROR("m_edgeListNode is invalid!");
        return result;
    }
    SPDLOG_INFO("start to parse edges from file[{}], m_edgeListNode.size() = {}",  m_filePath, m_edgeListNode.size());

    for (size_t edgeIterIdx = 0; edgeIterIdx < m_edgeListNode.size(); ++edgeIterIdx)
    {
        if (m_edgeListNode[edgeIterIdx] && m_edgeListNode[edgeIterIdx].IsMap() && m_edgeListNode[edgeIterIdx]["SrcPort"])
        {
            YamlPort srcPort = m_edgeListNode[edgeIterIdx]["SrcPort"].as<YamlPort>();

            YAML::Node dstPortsNode = m_edgeListNode[edgeIterIdx]["DstPort"];
            if (dstPortsNode && dstPortsNode.IsSequence())
            {
                for (size_t portIterIdx = 0; portIterIdx < dstPortsNode.size(); ++portIterIdx)
                {
                    result.emplace_back(srcPort, dstPortsNode[portIterIdx].as<YamlPort>());
                }
            }
            else 
            {
                SPDLOG_ERROR("dstPortNode is invalid or dstPortNode is not sequence");
                SPDLOG_ERROR("DstPort node YAML:\n{}", YAML::Dump(dstPortsNode));
            }
        }
        else
        {
            SPDLOG_ERROR("invalid node, checkinfo : isEdgeNodeValid[{}] | isEdgeNodeMap[{}]", m_edgeListNode[edgeIterIdx]? true : false, m_edgeListNode[edgeIterIdx].IsMap());
            SPDLOG_ERROR("Edge node YAML:\n{}", YAML::Dump(m_edgeListNode[edgeIterIdx]));
        }
    }
    SPDLOG_INFO("parseEdges Dode, collected {} edges", result.size());

    return result;
}
} // namespace SimpleNodeEditor

