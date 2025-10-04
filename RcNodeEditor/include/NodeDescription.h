#ifndef NODEDESCRIPTION_H
#define NODEDESCRIPTION_H
#include <string>
#include <vector>

struct NodeDescription
{
        std::string m_nodeName;
        std::vector<std::string> m_inputPortNames;
        std::vector<std::string> m_outputPortNames;

        NodeDescription() = default;
        template <typename T1, typename T2, typename T3>
        NodeDescription(T1&& nodeName, T2&& inportPorts, T3&& outputPorts)
        : m_nodeName(std::forward<T1>(nodeName))
        , m_inputPortNames(std::forward<T2>(m_inputPortNames))
        , m_outputPortNames(std::forward<T3>(m_outputPortNames))
        {

        }
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