#ifndef SNECONFIG_H
#define SNECONFIG_H
#include "YamlParser.hpp"

namespace SimpleNodeEditor
{
class SNEConfig
{
public:
    static SNEConfig& GetInstance()
    {
        static SNEConfig s_SNEConfig;
        return s_SNEConfig;
    };

    
    template <typename T>
    T GetConfigValue(const std::string& key)
    {
         return m_configParser.GetConfigValue<T>(key);
    }
private:
    SNEConfig() : m_configParser("./resource/config.yaml") { }
    ~SNEConfig() = default;
    SNEConfig(const SNEConfig&) = delete;
    SNEConfig& operator=(const SNEConfig&) = delete;

    ConfigParser m_configParser;

};
}

#endif // SNECONFIG_H