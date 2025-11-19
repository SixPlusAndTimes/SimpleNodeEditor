#ifndef YAMLPARSER_H
#define YAMLPARSER_H
#include <string>
#include <yaml-cpp/yaml.h>
#include "NodeDescription.hpp"
#include "Node.hpp"

namespace YAML
{

inline bool isValidKey(const Node& node, const std::string& key)
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
        bool ret = true;
        if (!node.IsMap()) {
            SPDLOG_INFO("decode yamlnodeproperty fail not a sequence");
            ret = false;
        }

        if (isValidKey(node, "NodePropertyName") && isValidKey(node, "NodePropertyValue"))
        {
            rhs.m_propertyName = node["NodePropertyName"].as<std::string>();
            rhs.m_propertyValue  = node["NodePropertyValue"].as<std::string>();
        }
        rhs.m_propertyId  = 0; // hard code here

        SPDLOG_INFO("decode yamlnodeproperty done, propertyName = [{}], propertyValue[{}] ", rhs.m_propertyName,
                    rhs.m_propertyValue);
        return ret;
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
        bool ret = true;
        if (!node.IsMap())
        {
            ret = false;
        }

        if (isValidKey(node, "group") && isValidKey(node, "type"))
        {
            rhs.m_Group = node["group"].as<std::string>();
            rhs.m_Type  = node["type"].as<std::string>();
        }
        else
        {
            SPDLOG_WARN("invalid key when parsing PruningRule");
        }

        SPDLOG_INFO("decode yamlpruningrule done, group = [{}], type[{}] ", rhs.m_Group,
                    rhs.m_Type);
        return ret;
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
        bool ret = true;
        if (!node.IsMap())
        {
            ret = false;
        }

        if (isValidKey(node, "NodeName") && isValidKey(node, "NodeId") && isValidKey(node, "IsSrcNode") && isValidKey(node, "NodeType"))
        {
            rhs.m_nodeName     = node["NodeName"].as<std::string>();
            rhs.m_nodeYamlId   = node["NodeId"].as<SimpleNodeEditor::YamlNode::NodeYamlId>();
            rhs.m_isSrcNode    = node["IsSrcNode"].as<int>();
            rhs.m_nodeYamlType = node["NodeType"].as<SimpleNodeEditor::YamlNodeType>();
        }
        else
        {
            ret = false;
            SPDLOG_ERROR("invalid required key when parsing YamlNode, check it! "
                         "isValidKey(node, \"NodeName\")[{}] isValidKey(node, \"NodeId\")[{}] isValidKey(node, \"IsSrcNode\")[{}] isValidKey(node, \"NodeType\")[{}]",
                         isValidKey(node, "NodeName"), isValidKey(node, "NodeId"), isValidKey(node, "IsSrcNode"), isValidKey(node, "NodeType"));
        }

        std::string pruneRuleKey = "PruneRule";
        if (isValidKey(node, pruneRuleKey))
        {
            for (YAML::const_iterator iter = node[pruneRuleKey].begin();
                 iter != node[pruneRuleKey].end(); ++iter)
            {
                rhs.m_PruningRules.push_back(iter->as<SimpleNodeEditor::YamlPruningRule>());
            }
        }
        else
        {
            SPDLOG_WARN("invalide nodePruneRule key {}", pruneRuleKey);
        }

        std::string nodePropertyKey = "NodeProperty";
        if (isValidKey(node, nodePropertyKey))
        {
            for (YAML::const_iterator iter = node[nodePropertyKey].begin();
                 iter != node[nodePropertyKey].end(); ++iter)
            {
                rhs.m_Properties.push_back(iter->as<SimpleNodeEditor::YamlNodeProperty>());
            }
        }
        else
        {
            SPDLOG_WARN("invalide nodeproperty key {}", nodePropertyKey);
        }

        SPDLOG_INFO(
            "decode yamlnode done, nodename = [{}], nodeYamlId[{}], issourcenode[{}], "
            "yamlNodeType[{}]",
            rhs.m_nodeName, rhs.m_nodeYamlId, rhs.m_isSrcNode, rhs.m_nodeYamlType);

        return ret;
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
        bool ret = true;
        if (!node.IsMap())
        {
            ret = false;
        }
        if (isValidKey(node, "NodeName") && isValidKey(node, "NodeId") && isValidKey(node, "PortName") && isValidKey(node, "PortId"))
        {

            rhs.m_nodeName = node["NodeName"].as<std::string>();
            rhs.m_nodeYamlId = node["NodeId"].as<SimpleNodeEditor::YamlNode::NodeYamlId>();
            rhs.m_portName = node["PortName"].as<std::string>();
            rhs.m_portYamlId = node["PortId"].as<SimpleNodeEditor::YamlPort::PortYamlId>();
        }
        else
        {
            ret = false;
            SPDLOG_ERROR("invalid required key when parsing YamlPort, check it! "
                         "isValidKey(node, \"NodeName\")[{}] isValidKey(node, \"NodeId\")[{}] isValidKey(node, \"PortName\")[{}] isValidKey(node, \"PortId\")[{}]",
                         isValidKey(node, "NodeName"), isValidKey(node, "NodeId"), isValidKey(node, "PortName"), isValidKey(node, "PortId"));
        }

        std::string pruneRuleKey = "PruneRule";
        if (isValidKey(node, pruneRuleKey))
        {
            for (YAML::const_iterator iter = node[pruneRuleKey].begin();
                    iter != node[pruneRuleKey].end(); ++iter)
            {
                rhs.m_PruningRules.push_back(iter->as<SimpleNodeEditor::YamlPruningRule>());
            }
        }
        else
        {
            SPDLOG_WARN(
                "invalide key[{}], when parsing ports, nodeName[{}], nodeyamlId[{}], "
                "portName[{}], portYamlId[{}]",
                pruneRuleKey, rhs.m_nodeName, rhs.m_nodeYamlId, rhs.m_portName,
                rhs.m_portYamlId);
        }
        return ret;
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