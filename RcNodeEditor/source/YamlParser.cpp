#include "spdlog/spdlog.h"
#include "YamlParser.h"

YamlParser::YamlParser()
{
}

bool YamlParser::LoadFile(const std::string& filePath)
{
    m_rootNode = YAML::LoadFile(filePath);

    return !m_rootNode ? false : true;
}

ConfigParser::ConfigParser(const std::string& filePath)
{
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