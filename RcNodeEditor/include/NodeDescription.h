#ifndef NODEDESCRIPTION_H
#define NODEDESCRIPTION_H
#include <string>
#include <vector>

struct NodeDescription
{
    std::string m_nodeName;
    std::vector<std::string> m_inputPortNames;
    std::vector<std::string> m_outputPortNames;
};

class NodeDescriptionParser
{

public:
    NodeDescriptionParser();
    std::vector<NodeDescription> ParseNodeDescriptions(const std::string& yamlFilePath);
private:
    // std::vector<NodeDescription> m_nodeDescriptions;

    // bool m_isParsed; // Flag indicating whether we have parsed the file?

};
#endif // NODEDESCRIPTION_H