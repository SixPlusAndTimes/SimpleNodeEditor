#ifndef YAMLPARSER_H
#define YAMLPARSER_H
#include <string>
#include <yaml-cpp/yaml.h>
#include "NodeDescription.hpp"
#include "Node.hpp"

namespace YAML
{

inline bool isInvalidKey(const Node& node, const std::string& key)
{
    return node[key] ? true : false;
}
template <>
struct convert<SimpleNodeEditor::YamlPruningRule>
{
    static YAML::Node encode(const SimpleNodeEditor::YamlPruningRule& rhs)
    {
        YAML::Node node;
        node.force_insert("group", rhs.m_Group);
        node.force_insert("type", rhs.m_Type);
        return node;
    }

    static bool decode(const Node& node, SimpleNodeEditor::YamlPruningRule& rhs)
    {
        if (!node.IsMap())
        {
            return false;
        }

        rhs.m_Group = node["group"].as<std::string>();
        rhs.m_Type  = node["type"].as<std::string>();
        SPDLOG_INFO("decode yamlpruningrule success, group = [{}], type[{}] ", rhs.m_Group,
                    rhs.m_Type);
        return true;
    }
};

template <>
struct convert<SimpleNodeEditor::YamlNode>
{
    static YAML::Node encode(const SimpleNodeEditor::YamlNode& rhs)
    {
        YAML::Node node;
        node.force_insert("NodeName", rhs.m_nodeName);
        node.force_insert("NodeId", rhs.m_nodeYamlId);
        node.force_insert("IsSrcNode", rhs.m_isSrcNode);
        node.force_insert("NodeType", rhs.m_nodeYamlType);

        for (const auto& pruneRule : rhs.m_PruningRules)
        {
            node["PruneRule"].push_back(pruneRule);
        }

        return node;
    }

    static bool decode(const Node& node, SimpleNodeEditor::YamlNode& rhs)
    {
        if (!node.IsMap())
        {
            return false;
        }

        rhs.m_nodeName     = node["NodeName"].as<std::string>();
        rhs.m_nodeYamlId   = node["NodeId"].as<SimpleNodeEditor::YamlNode::NodeYamlId>();
        rhs.m_isSrcNode    = node["IsSrcNode"].as<int>();
        rhs.m_nodeYamlType = node["NodeType"].as<SimpleNodeEditor::YamlNodeType>();

        std::string pruneRuleKey = "PruneRule";
        if (isInvalidKey(node, pruneRuleKey))
        {
            for (YAML::const_iterator iter = node[pruneRuleKey].begin();
                 iter != node[pruneRuleKey].end(); ++iter)
            {
                rhs.m_PruningRules.push_back(iter->as<SimpleNodeEditor::YamlPruningRule>());
            }
        }
        else
        {
            SPDLOG_ERROR("invalide key {}", pruneRuleKey);
        }
        SPDLOG_INFO(
            "decode yamlnode success, nodename = [{}], nodeYamlId[{}], issourcenode[{}], "
            "yamlNodeType[{}]",
            rhs.m_nodeName, rhs.m_nodeYamlId, rhs.m_isSrcNode, rhs.m_nodeYamlType);
        return true;
    }
};

template <>
struct convert<SimpleNodeEditor::YamlPort>
{
    static YAML::Node encode(const SimpleNodeEditor::YamlPort& rhs)
    {
        YAML::Node node;
        node.force_insert("NodeName", rhs.m_nodeName);
        node.force_insert("NodeId", rhs.m_nodeYamlId);
        node.force_insert("PortName", rhs.m_portName);
        node.force_insert("PortId", rhs.m_portYamlId);

        for (const auto& pruneRule : rhs.m_PruningRules)
        {
            node["PruneRule"].push_back(pruneRule);
        }

        return node;
    }

    static bool decode(const Node& node, SimpleNodeEditor::YamlPort& rhs)
    {
        if (!node.IsMap())
        {
            return false;
        }

        if (isInvalidKey(node, "NodeName") && isInvalidKey(node, "NodeId") &&
            isInvalidKey(node, "PortName") && isInvalidKey(node, "PortId"))
        {
            rhs.m_nodeName   = node["NodeName"].as<std::string>();
            rhs.m_nodeYamlId = node["NodeId"].as<SimpleNodeEditor::YamlNode::NodeYamlId>();
            rhs.m_portName   = node["PortName"].as<std::string>();
            rhs.m_portYamlId = node["PortId"].as<SimpleNodeEditor::YamlPort::PortYamlId>();

            std::string pruneRuleKey = "PruneRule";
            if (isInvalidKey(node, pruneRuleKey))
            {
                for (YAML::const_iterator iter = node[pruneRuleKey].begin();
                     iter != node[pruneRuleKey].end(); ++iter)
                {
                    rhs.m_PruningRules.push_back(iter->as<SimpleNodeEditor::YamlPruningRule>());
                }
            }
            else
            {
                SPDLOG_ERROR(
                    "invalide key[{}], when parsing ports, nodeName[{}], nodeyamlId[{}], "
                    "portName[{}], portYamlId[{}]",
                    pruneRuleKey, rhs.m_nodeName, rhs.m_nodeYamlId, rhs.m_portName,
                    rhs.m_portYamlId);
            }

            SPDLOG_INFO(
                "decode yamlport success, portname[{}], portYamlId[{}], nodename[{}], nodeid[{}]",
                rhs.m_portName, rhs.m_portYamlId, rhs.m_nodeName, rhs.m_nodeYamlId);
            return true;
        }
        else
        {
            SPDLOG_ERROR("Invalid key when yaml-cpp decode from yaml");
            return false;
        }
    }
};

} // namespace YAML

namespace SimpleNodeEditor
{

class YamlParser
{
public:
    [[nodiscard]] virtual bool LoadFile(const std::string& filePath);

protected:
    YAML::Node       m_rootNode;
    std::string_view m_filePath;

private:
};

class ConfigParser : public YamlParser
{
public:
    ConfigParser(const std::string& filePath);

    template <typename T>
    T GetConfigValue(const std::string& key)
    {
        if (!m_rootNode)
        {
            SPDLOG_ERROR("rootNode is not invalid");
            return T();
        }

        if (m_rootNode[key])
        {
            return m_rootNode[key].as<T>();
        }
        else
        {
            SPDLOG_ERROR("Configparser get error, key is {}", key);
            return T();
        }
    }
};

class NodeDescriptionParser : public YamlParser
{
public:
    NodeDescriptionParser(const std::string& filePath);
    std::vector<NodeDescription> ParseNodeDescriptions();
};

class PipelineParser : public YamlParser
{
public:
    PipelineParser();
    std::vector<YamlNode> ParseNodes();
    std::vector<YamlEdge> ParseEdges();

    [[nodiscard]] virtual bool LoadFile(const std::string& filePath);

private:
    YAML::Node m_nodeListNode;
    YAML::Node m_edgeListNode;
};

} // namespace SimpleNodeEditor

#endif // YAMLPARSER_H