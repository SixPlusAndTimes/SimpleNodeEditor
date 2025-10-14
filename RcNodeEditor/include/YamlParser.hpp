#ifndef YAMLPARSER_H
#define YAMLPARSER_H
#include <string>
#include <yaml-cpp/yaml.h>
#include "NodeDescription.hpp"
#include "Node.hpp"

namespace YAML {
template<>
struct convert<SimpleNodeEditor::YamlNode> {
  static YAML::Node encode(const SimpleNodeEditor::YamlNode& rhs) 
  {
    YAML::Node node;
    node.force_insert("NodeName", rhs.m_nodeName);
    node.force_insert("NodeId", rhs.m_nodeYamlId);
    node.force_insert("IsSrcNode", rhs.m_isSrcNode);
    return node;
  }

  static bool decode(const Node& node, SimpleNodeEditor::YamlNode& rhs) 
  {
    if(!node.IsMap()) {
      return false;
    }

    rhs.m_nodeName = node["NodeName"].as<std::string>();
    rhs.m_nodeYamlId = node["NodeId"].as<SimpleNodeEditor::YamlNode::NodeYamlId>();
    rhs.m_isSrcNode = node["IsSrcNode"].as<int>();

    SPDLOG_INFO("decode yamlnode success, nodename = [{}], nodeYamlId[{}], issourcenode[{}]", rhs.m_nodeName, rhs.m_nodeYamlId, rhs.m_isSrcNode);
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
protected:

    YAML::Node m_rootNode;
    std::string_view m_filePath;
private:
};

class ConfigParser : public YamlParser
{
public:
    ConfigParser(const std::string& filePath);

    template<typename T>
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
    [[nodiscard]] virtual bool LoadFile(const std::string& filePath);
    
    // std::vector<YamlEdge> parseEdges(const std::string& filePath);
private:
    YAML::Node m_nodeListNode;
    YAML::Node m_edgeListNode;
};

} // namespace SimpleNodeEditor

#endif // YAMLPARSER_H