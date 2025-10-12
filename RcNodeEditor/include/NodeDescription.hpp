#ifndef NODEDESCRIPTION_H
#define NODEDESCRIPTION_H
#include <string>
#include <vector>
// NodeDescription is store in yaml file, we need parse it, see YamlParser::NodeDescriptionParser
namespace SimpleNodeEditor
{
    
struct NodeDescription
{
    std::string m_nodeName;
    std::vector<std::string> m_inputPortNames;
    std::vector<std::string> m_outputPortNames;
};
} // namespace SimpleNodeEditor


#endif // NODEDESCRIPTION_H