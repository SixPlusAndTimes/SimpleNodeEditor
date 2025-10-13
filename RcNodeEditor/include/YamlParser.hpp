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
    Node node;
    node.force_insert("NodeName", rhs.m_nodeName);
    node.force_insert("NodeId", rhs.m_nodeYamlId);
    node.force_insert("IsSrcNode", rhs.m_isSrcNode);
    return node;
  }

  static bool decode(const Node& node, SimpleNodeEditor::Node& rhs) 
  {
    if(!node.IsMap()) {
      return false;
    }
    rhs.SetNodeTitle(node["NodeName"].as<std::string>());
    rhs.SetNodeYamlId(node["NodeId"].as<SimpleNodeEditor::YamlNode::NodeYamlId>());
    return true;
  }
};

} // namespace YAML


namespace SimpleNodeEditor
{

class YamlParser
{
public:
protected:
    [[nodiscard]] virtual bool LoadFile(const std::string& filePath);
    YAML::Node m_rootNode;
private:
    std::string_view m_filePath;
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
    PipelineParser(const std::vector<NodeDescription>& nodeDescriptions);
    std::vector<Node> ParseNodes(const std::string& filePath);
    std::vector<Edge> parseEdges(const std::string& filePath);
private:
    const std::vector<NodeDescription>& m_nodeDescriptions;

    YAML::Node m_nodesNode;
    YAML::Node m_edgesNode;
};
} // namespace SimpleNodeEditor

#endif // YAMLPARSER_H