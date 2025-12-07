#ifndef DATASTRUCTUREYAML_H
#define DATASTRUCTUREYAML_H

#include <string>
#include <vector>
namespace SimpleNodeEditor
{
using YamlNodeType = int32_t;

struct NodeDescription
{
    std::string              m_nodeName;
    YamlNodeType             m_yamlNodeType;
    std::vector<std::string> m_inputPortNames;
    std::vector<std::string> m_outputPortNames;
};


struct YamlNodeProperty
{
    std::string m_propertyName;
    int32_t     m_propertyId;
    std::string m_propertyValue;
};

struct YamlPruningRule
{
    std::string m_Group;
    std::string m_Type;
};

struct YamlNode
{
    using NodeYamlId = int32_t;

    YamlNode()
        : m_nodeName("Unknown"),
          m_nodeYamlId(-1),
          m_isSrcNode(false),
          m_nodeYamlType(-1),
          m_Properties(),
          m_PruningRules()
    {
    }
    std::string                   m_nodeName;
    NodeYamlId                    m_nodeYamlId;
    int                           m_isSrcNode;
    YamlNodeType                  m_nodeYamlType;
    std::vector<YamlNodeProperty> m_Properties;
    std::vector<YamlPruningRule>  m_PruningRules;
};

struct YamlPort
{
    using PortYamlId = int32_t;
    std::string          m_nodeName;
    YamlNode::NodeYamlId m_nodeYamlId;
    std::string          m_portName;
    PortYamlId           m_portYamlId;

    std::vector<YamlPruningRule> m_PruningRules; // only dst port has pruning rules
};

// srcport info maybe redundant in different edges, check it later
struct YamlEdge
{
    YamlEdge() : m_yamlSrcPort(), m_yamlDstPort(), m_isValid(false) {};
    YamlEdge(const YamlPort& srcPort, const YamlPort& dstPort, bool isValid)
        : m_yamlSrcPort(srcPort), m_yamlDstPort(dstPort), m_isValid(isValid) {};
    YamlPort m_yamlSrcPort;
    YamlPort m_yamlDstPort;
    bool     m_isValid;

    // TODO : add properties
};

} // namespace SimpleNodeEditor

#endif // DATASTRUCTUREYAML_H
