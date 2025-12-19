#include "spdlog/spdlog.h"
#include "YamlParser.hpp"
#include <filesystem>
#include "Log.hpp"

namespace SimpleNodeEditor
{

bool YamlParser::LoadFile(const std::string& filePath)
{
    if (std::filesystem::is_regular_file(filePath))
    {
        m_rootNode = YAML::LoadFile(filePath);

        if (m_rootNode)
        {
            m_filePath = filePath;
            return true;
        }
        else
        {
            SNELOG_ERROR("no valid rootNode, check the file [{}]", filePath);
            return false;
        }
    }
    else
    {
        SNELOG_ERROR("[{}] is not a regular file", filePath);
        return false;
    }
}

void YamlParser::Clear()
{
    m_rootNode = YAML::Node();
    m_filePath = {};
}

ConfigParser::ConfigParser(const std::string& filePath)
{
    // is it useful to check LoadFile here?
    if (LoadFile(filePath))
    {
        SNELOG_INFO("ConfigParser LoadFile success : {}", filePath);
    }
    else
    {
        SNELOG_ERROR("ConfigParser failed : {}", filePath);
    }
}

NodeDescriptionParser::NodeDescriptionParser(const std::string& filePath)
{
    if (LoadFile(filePath))
    {
        SNELOG_INFO("NodeDescriptionParser LoadFile success : {}", filePath);
    }
    else
    {
        SNELOG_ERROR("ConfigParser LoadFile failed : {}", filePath);
    }
}

std::vector<NodeDescription> NodeDescriptionParser::ParseNodeDescriptions()
{
    std::vector<NodeDescription> ret;

    // 1. whether m_rootNode is valid
    if (!m_rootNode)
    {
        SNELOG_ERROR("rootNode is not invalid");
        return ret;
    }

    // 2. whether NodeDescriptions Node exists in the yaml file
    if (!m_rootNode["NodeDescriptions"])
    {
        SNELOG_ERROR("Cannot find NodeDescriptions in yaml file, check it!");
        return ret;
    }
    // 3. whether NodeDescriptions Node is a sequence ndoe
    YAML::Node nodeDescriptions = m_rootNode["NodeDescriptions"];
    if (!nodeDescriptions.IsSequence())
    {
        SNELOG_ERROR("NodeDescription is not a sequence, check it!");
        return ret;
    }

    // 4 iterate through all node descriptions
    for (const auto& node : nodeDescriptions)
    {
        NodeDescription desc;
        // parse NodeName
        if (node["NodeName"] && node["NodeName"].IsScalar())
        {
            desc.m_nodeName = node["NodeName"].as<std::string>();
        }
        else
        {
            SNELOG_ERROR("Skipping node: missing or invalid NodeName");
            continue;
        }

         // parse NodeType
        if (node["NodeType"] && node["NodeType"].IsScalar())
        {
            desc.m_yamlNodeType = node["NodeType"].as<YamlNodeType>();
        }
        else
        {
            SNELOG_ERROR("Skipping node: missing or invalid NodeType");
            continue;
        }

        // parse InputPorts
        if (node["InputPorts"] && node["InputPorts"].IsSequence())
        {
            for (const auto& port : node["InputPorts"])
            {
                if (port.IsScalar())
                {
                    desc.m_inputPortNames.push_back(port.as<std::string>());
                }
                else
                {
                    SNELOG_ERROR("Skipping invalid InputPort in node: {}", desc.m_nodeName);
                }
            }
        }
        else
        {
            SNELOG_ERROR("Node {} has no valid InputPorts sequence", desc.m_nodeName);
        }
        // parse OutputPorts
        if (node["OutputPorts"] && node["OutputPorts"].IsSequence())
        {
            for (const auto& port : node["OutputPorts"])
            {
                if (port.IsScalar())
                {
                    desc.m_outputPortNames.push_back(port.as<std::string>());
                }
                else
                {
                    SNELOG_ERROR("Skipping invalid OutputPort in node: {}", desc.m_nodeName);
                }
            }
        }
        else
        {
            SNELOG_ERROR("Node {} has no valid OutputPorts sequence", desc.m_nodeName);
        }
        ret.push_back(std::move(desc));
    }

    return ret;
}

PipelineParser::PipelineParser() : m_pipelineName(), m_nodeListNode(), m_edgeListNode() {}

const std::string& PipelineParser::GetPipelineName()
{
    return m_pipelineName;
}

// invalid NodeList should trigger an error
// invalid LinkList or Pipelinename just trigger a warning
bool PipelineParser::LoadFile(const std::string& filePath)
{
    bool ret   = true;
    m_rootNode = YAML::LoadFile(filePath);
    if (m_rootNode && m_rootNode["Pipeline"] && m_rootNode["Pipeline"].IsSequence())
    {
        m_filePath = filePath;
        m_rootNode = m_rootNode["Pipeline"][0];
        // std::string pipeLineName = m_rootNode["pipelinename"].as<std::string>();
    }
    else
    {
        SNELOG_ERROR("load file[{}] failed", filePath);
        ret = false;
    }

    if (ret && m_rootNode["pipelinename"] && m_rootNode["pipelinename"].IsScalar())
    {
        m_pipelineName = m_rootNode["pipelinename"].as<std::string>();
    }
    else
    {
        SNELOG_WARN("file {} has no valid pipelinename sequence, check it", filePath);
        // ret = false;
    }

    if (ret && m_rootNode["NodeList"] && m_rootNode["NodeList"].IsSequence() &&
        m_rootNode["NodeList"].size() != 0)
    {
        m_nodeListNode = m_rootNode["NodeList"];
    }
    else
    {
        SNELOG_ERROR("file {} has no valid Node sequence, check it", filePath);
        ret = false;
    }

    if (ret && m_rootNode["LinkList"] && m_rootNode["LinkList"].IsSequence() &&
        m_rootNode["LinkList"].size() != 0)
    {
        m_edgeListNode = m_rootNode["LinkList"];
    }
    else
    {
        SNELOG_WARN("file {} has no valid LinkList sequence, better to check it", filePath);
        Notifier::Add(Message(Message::Type::WARNING, "", "invalid Linklist, better to check it"));
        // ret = false;
    }

    return ret;
}

std::vector<YamlNode> PipelineParser::ParseNodes()
{
    SNELOG_INFO("start to parse node from file[{}], m_nodeListNode.size() = {}", m_filePath,
                m_nodeListNode.size());
    std::vector<YamlNode> result;

    for (YAML::const_iterator iter = m_nodeListNode.begin(); iter != m_nodeListNode.end(); ++iter)
    {
        result.push_back(iter->as<YamlNode>());
    }
    SNELOG_INFO("ParseNodes Dode, collected {} nodes", result.size());
    return result;
}

// do we need more syntax check ?
std::vector<YamlEdge> PipelineParser::ParseEdges()
{
    std::vector<YamlEdge> result;
    if (!m_edgeListNode)
    {
        SNELOG_ERROR("m_edgeListNode is invalid!");
        return result;
    }
    SNELOG_INFO("start to parse edges from file[{}], m_edgeListNode.size() = {}", m_filePath,
                m_edgeListNode.size());

    for (size_t edgeIterIdx = 0; edgeIterIdx < m_edgeListNode.size(); ++edgeIterIdx)
    {
        if (m_edgeListNode[edgeIterIdx] && m_edgeListNode[edgeIterIdx].IsMap() &&
            m_edgeListNode[edgeIterIdx]["SrcPort"])
        {
            YamlPort srcPort = m_edgeListNode[edgeIterIdx]["SrcPort"].as<YamlPort>();

            YAML::Node dstPortsNode = m_edgeListNode[edgeIterIdx]["DstPort"];
            if (dstPortsNode && dstPortsNode.IsSequence())
            {
                for (size_t portIterIdx = 0; portIterIdx < dstPortsNode.size(); ++portIterIdx)
                {
                    result.emplace_back(srcPort, dstPortsNode[portIterIdx].as<YamlPort>(), true);
                }
            }
            else
            {
                SNELOG_ERROR("dstPortNode is invalid or dstPortNode is not sequence");
                SNELOG_ERROR("DstPort node YAML:\n{}", YAML::Dump(dstPortsNode));
            }
        }
        else
        {
            SNELOG_ERROR("invalid node, checkinfo : isEdgeNodeValid[{}] | isEdgeNodeMap[{}]",
                         m_edgeListNode[edgeIterIdx] ? true : false,
                         m_edgeListNode[edgeIterIdx].IsMap());
            SNELOG_ERROR("Edge node YAML:\n{}", YAML::Dump(m_edgeListNode[edgeIterIdx]));
        }
    }
    SNELOG_INFO("parseEdges Dode, collected {} edges", result.size());

    return result;
}

void PipelineParser::Clear()
{
    m_pipelineName = {};
    m_edgeListNode = YAML::Node();
    m_nodeListNode = YAML::Node();
    YamlParser::Clear();
}

} // namespace SimpleNodeEditor
