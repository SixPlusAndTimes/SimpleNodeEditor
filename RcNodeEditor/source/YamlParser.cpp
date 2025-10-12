#include "spdlog/spdlog.h"
#include "YamlParser.h"

YamlParser::YamlParser(const std::string& filePath)
:m_filePath(filePath)
{
    if (LoadFile(m_filePath.data()))
    {
        SPDLOG_INFO("LoadYaml file success : {}", filePath);
    }
    else 
    {
        SPDLOG_ERROR("LoadYaml file failed : {}", filePath);
    }
}

bool YamlParser::LoadFile(const std::string& filePath)
{
    try {
        m_rootNode = YAML::LoadFile(filePath);
    } catch (const std::runtime_error& e) {
        SPDLOG_ERROR("LoadFile exception : {}", e.what());
    }

    return true;
}

ConfigParser::ConfigParser(const std::string& filePath)
: YamlParser(filePath)
{

}