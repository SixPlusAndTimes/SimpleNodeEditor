#ifndef YAMLPARSER_H
#define YAMLPARSER_H
#include <string>
#include <yaml-cpp/yaml.h>
#include "NodeDescription.hpp"

class YamlParser
{
public:
    YamlParser(const std::string& filePath);
private:
    [[nodiscard]] bool LoadFile(const std::string& filePath);
protected:
    std::string_view m_filePath;
    YAML::Node m_rootNode;
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

#endif // YAMLPARSER_H