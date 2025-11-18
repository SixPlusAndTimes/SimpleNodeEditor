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
struct convert<SimpleNodeEditor::YamlNodeProperty>
{
    static YAML::Node encode(const SimpleNodeEditor::YamlNodeProperty& rhs)
    {
        YAML::Node node;
        node.force_insert("NodePropertyName", rhs.m_propertyName);
        node.force_insert("NodePropertyId", rhs.m_propertyId);
        node.force_insert("NodePropertyValue", rhs.m_propertyValue);
        return node;
    }

    static bool decode(const Node& node, SimpleNodeEditor::YamlNodeProperty& rhs)
    {
        if (!node.IsMap())
        {
            return false;
        }

        rhs.m_propertyName = node["NodePropertyName"].as<std::string>();
        rhs.m_propertyValue  = node["NodePropertyValue"].as<std::string>();
        // rhs.m_propertyId  = node["NodePropertyId"].as<int32_t>();
        SPDLOG_INFO("decode yamlnodeproperty success, propertyName = [{}], propertyValue[{}] ", rhs.m_propertyName,
                    rhs.m_propertyValue);
        return true;
    }
};

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
            SPDLOG_ERROR("invalide nodePruneRule key {}", pruneRuleKey);
        }

        std::string nodePropertyKey = "NodeProperty";
        if (isInvalidKey(node, nodePropertyKey))
        {
            for (YAML::const_iterator iter = node[nodePropertyKey].begin();
                 iter != node[nodePropertyKey].end(); ++iter)
            {
                rhs.m_Properties.push_back(iter->as<SimpleNodeEditor::YamlNodeProperty>());
            }
        }
        else
        {
            SPDLOG_ERROR("invalide nodeproperty key {}", nodePropertyKey);
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
        if(isInvalidKey(node, "NodeName")) rhs.m_nodeName = node["NodeName"].as<std::string>();
        if(isInvalidKey(node, "NodeId")) rhs.m_nodeYamlId = node["NodeId"].as<SimpleNodeEditor::YamlNode::NodeYamlId>();
        if(isInvalidKey(node, "PortName")) rhs.m_portName = node["PortName"].as<std::string>();
        if(isInvalidKey(node, "PortId")) rhs.m_portYamlId = node["PortId"].as<SimpleNodeEditor::YamlPort::PortYamlId>();

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
        return true;
    }
};

} // namespace YAML

namespace SimpleNodeEditor
{

class YamlParser
{
public:
    [[nodiscard]] virtual bool LoadFile(const std::string& filePath);
    virtual void Clear();

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
    const std::string& GetPipelineName();

    [[nodiscard]] virtual bool LoadFile(const std::string& filePath);
    virtual void Clear() override;

private:
    std::string m_pipelineName;
    YAML::Node m_nodeListNode;
    YAML::Node m_edgeListNode;
};

} // namespace SimpleNodeEditor

#endif // YAMLPARSER_H