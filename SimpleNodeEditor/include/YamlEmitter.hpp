#ifndef YAMLEMITTER_H
#define YAMLEMITTER_H
#include "yaml-cpp/yaml.h"
#include "DataStructureEditor.hpp"
#include "DataStructureYaml.hpp"
namespace SimpleNodeEditor
{

class YamlEmitter
{
public:
    YamlEmitter();
    YamlEmitter(const YamlEmitter&)            = delete;
    YamlEmitter& operator=(const YamlEmitter&) = delete;
    ~YamlEmitter()                             = default;
    YAML::Emitter& GetEmitter();
    void           Clear();

protected:
    void BeginMap();
    void EndMap();
    void BeginSequence();
    void EndSequence();
    void EmitKeyValue();
    void BeginValue();

    template <typename T>
    void EmitKey(const T& key)
    {
        *m_Emitter << YAML::Key << key;
    }

    template <typename T>
    void EmitValue(const T& value)
    {
        *m_Emitter << YAML::Value << value;
    }

    template <typename T>
    void EmitKeyValue(const std::string& key, const T& value)
    {
        EmitKey(key);
        EmitValue(value);
    }

private:
    std::unique_ptr<YAML::Emitter> m_Emitter;
};

class PipelineEmitter : public YamlEmitter
{
public:
    PipelineEmitter();
    explicit PipelineEmitter(std::ostream& stream);
    PipelineEmitter(const PipelineEmitter&)            = delete;
    PipelineEmitter& operator=(const PipelineEmitter&) = delete;
    ~PipelineEmitter()                                 = default;
    void EmitPipeline(const std::string&                            pipelineName,
                      const std::unordered_map<NodeUniqueId, Node>& nodesMap,
                      const std::unordered_map<NodeUniqueId, Node>& prunedNodesMap,
                      const std::unordered_map<EdgeUniqueId, Edge>& egesMap,
                      const std::unordered_map<NodeUniqueId, Edge>& prunedEdgesMap);

private:
    void EmitNodeList(const std::unordered_map<NodeUniqueId, Node>& nodesMap,
                      const std::unordered_map<NodeUniqueId, Node>& prunedNodesMap);
    void EmitYamlNode(const YamlNode& yamlNode);
    void EmitLinkList(const std::unordered_map<EdgeUniqueId, Edge>& edgesMap,
                      const std::unordered_map<EdgeUniqueId, Edge>& prunedEdgesMap);
    void EmitYamlEdge(const YamlPort& srcPort, const std::vector<YamlPort>& dstPortVec);
};

} // namespace SimpleNodeEditor

#endif // YAMLEMITTER_H