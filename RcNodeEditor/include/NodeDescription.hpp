#ifndef NODEDESCRIPTION_H
#define NODEDESCRIPTION_H
#include <string>
#include <vector>
// TODO : refactor the Yaml related infomation
// NodeDescription is store in yaml file, we need parse it, see YamlParser::NodeDescriptionParser
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
} // namespace SimpleNodeEditor

#endif // NODEDESCRIPTION_H