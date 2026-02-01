#ifndef HELPERS_H
#define HELPERS_H
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <type_traits>
#include <ranges>
#include "Log.hpp"
#include "DataStructureEditor.hpp"
#include "Common.hpp"
#include <imgui.h>
#include <imnodes.h>

namespace SimpleNodeEditor
{
using PortUniqueId = int32_t;
using EdgeUniqueId = int32_t;
using NodeUniqueId = int32_t;


template <typename UidType, typename = std::enable_if_t<std::is_integral_v<UidType> &&
                                                        !std::is_same_v<UidType, bool>>>
class UniqueIdAllocator
{
public:
    explicit UniqueIdAllocator(const std::string& name = "UnknownAllocator", UidType start = 0)
        : m_initial_start(start), m_nextUid(start), m_name(name)
    {
    }

    UidType AllocUniqueID()
    {
        while (m_registeredUids.contains(m_nextUid))
        {
            ++m_nextUid;
        }
        UidType allocated = m_nextUid++;
        m_registeredUids.insert(allocated);
        return allocated;
    }

    UidType RegisterUniqueID(UidType uid)
    {
        if (m_registeredUids.contains(uid))
        {
            SNELOG_INFO("allocator[{}] uid[{}] has already been allocated or registered", m_name,
                        uid);
        }
        if (m_registeredUids.insert(uid).second)
        {
            return uid;
        }
        else
        {
            return UidType();
        }
    }

    bool UnregisterUniqueID(UidType uid)
    {
        bool existed = m_registeredUids.erase(uid);
        if (existed && uid < m_nextUid)
        {
            m_nextUid = uid;
        }
        return existed;
    }

    bool IsRegistered(UidType uid) const
    {
        return m_registeredUids.contains(uid);
    }

    void Clear()
    {
        m_registeredUids.clear();
        m_nextUid = m_initial_start;
    }

private:
    const UidType               m_initial_start;
    UidType                     m_nextUid;
    std::unordered_set<UidType> m_registeredUids;
    std::string                 m_name;
};

template <typename T>
struct UniqueIdGenerator
{
    T m_Uid{};
    T AllocUniqueID()
    {
        return m_Uid++;
    }
};



template <typename T>
size_t GetMatchedIndex(const std::vector<T>& inputVec, const T& comp)
{
    for (size_t index = 0; index < inputVec.size(); ++index)
    {
        if (inputVec[index] == comp)
        {
            return index;
        }
    }
    SNELOG_INFO("no matched index, comp[{}]", comp);
    return std::numeric_limits<size_t>::max();
}

inline void DumpEdge(const Edge& edge)
{
    SNELOG_INFO("EdgeInfo , EdgeUid[{}]:", edge.GetEdgeUniqueId());
    SNELOG_INFO("\t srcNodeUid[{}] srcYamlNodeName[{}], srcYamlportName[{}]",
                edge.GetSourceNodeUid(), edge.GetYamlEdge().m_yamlSrcPort.m_nodeName,
                edge.GetYamlEdge().m_yamlSrcPort.m_portName);
    SNELOG_INFO("\t dstNodeUid[{}] dstYajmlNodeName[{}], dstYamlportName[{}]",
                edge.GetDestinationNodeUid(), edge.GetYamlEdge().m_yamlDstPort.m_nodeName,
                edge.GetYamlEdge().m_yamlDstPort.m_portName);
}

// must be a inline function to avoid vialation of OneDefinitionRule
inline std::vector<std::vector<NodeUniqueId>> TopologicalSort(
    std::unordered_map<NodeUniqueId, Node>& nodesMap,
    std::unordered_map<EdgeUniqueId, Edge>& edgesMap)
{
    if (nodesMap.size() == 0)
    {
        SNELOG_WARN("nodesMap size == 0");
        return {};
    }

    if (edgesMap.size() == 0)
    {
        auto nodeIds = nodesMap | std::views::keys;
        SNELOG_WARN("edgesMap size == 0, return one topo order, nodesize = [{}]", nodeIds.size());
        return {{nodeIds.begin(), nodeIds.end()}};
    }

    std::vector<std::vector<NodeUniqueId>> result;
    std::unordered_map<NodeUniqueId, int>  degrees;
    // init all node's degree to 0
    for (const auto& [nodeUid, _] : nodesMap)
    {
        degrees.emplace(nodeUid, 0);
    }

    // sum up all node's degree
    for (const auto& [_, edge] : edgesMap)
    {
        if (!degrees.contains(edge.GetDestinationNodeUid()))
        {
            SNELOG_ERROR("error dstnodeuid [{}]", edge.GetDestinationNodeUid());
        }
        ++degrees.at(edge.GetDestinationNodeUid());
    }

    // collect zero degree node
    std::vector<NodeUniqueId> zeroDegreeNodes;
    for (const auto& [nodeUid, degree] : degrees)
    {
        if (!degree)
        {
            zeroDegreeNodes.push_back(nodeUid);
        }
    }

    // do the sorting 
    while (zeroDegreeNodes.size() != 0)
    {
        result.push_back(zeroDegreeNodes);
        std::vector<NodeUniqueId> newZeroDegreeNodes;

        for (NodeUniqueId zeroDegreeNodeUid : zeroDegreeNodes)
        {
            for (const OutputPort& outPort : nodesMap.at(zeroDegreeNodeUid).GetOutputPorts())
            {
                for (const EdgeUniqueId outEdgeUid : outPort.GetEdgeUids())
                {
                    NodeUniqueId decDegreeNodeUid = edgesMap.at(outEdgeUid).GetDestinationNodeUid();
                    if (--degrees.at(decDegreeNodeUid) == 0)
                    {
                        newZeroDegreeNodes.push_back(decDegreeNodeUid);
                    }
                }
            }
        }
        zeroDegreeNodes.swap(newZeroDegreeNodes);
    }

    // finally, all nodes' degree must be zero
    bool allNodeDegreeZero = true;
    for (const auto& [nodeUid, degree] : degrees)
    {
        if (degree != 0)
        {
            SNELOG_ERROR("toposort done, but node[{}] has degree[{}]", nodeUid, degree);
            allNodeDegreeZero = false;
        }
    }
    SNE_ASSERT(allNodeDegreeZero, "Not All NodeDegrees are zero, checkit");

    return result;
}

inline void IMNODES_POP_STYLE_COL(uint32_t number) {
    for (uint32_t i = 0; i < number; ++i)
        ImNodes::PopColorStyle();
}

inline void IMGUI_POP_STYLE_COL(uint32_t number) {
    for (uint32_t i = 0; i < number; ++i)
        ImGui::PopStyleColor();
}

class OpacitySetter
{
public:
    void SetOpacity(ImGuiCol_  type)
    {
        const ImVec4 originalColor = ImGui::GetStyle().Colors[type];
        ImVec4 destColor = originalColor;
        destColor.w = m_destOpacity;
        ImGui::PushStyleColor(type, destColor);
        m_imguiCount++;
    }

    void SetOpacity(ImNodesCol_ type)
    {
        const ImVec4 originalColor = ImGui::ColorConvertU32ToFloat4(ImNodes::GetStyle().Colors[type]);
        ImVec4 destColor = originalColor; destColor.w = m_destOpacity;
        ImNodes::PushColorStyle(type, ImGui::ColorConvertFloat4ToU32(destColor));
        m_imnodesCount++;
    }

    template <typename... Args>
    OpacitySetter(float destOpacity, Args... cols)
        : m_destOpacity(destOpacity),
          m_imnodesCount(0), m_imguiCount(0)
    {
        static_assert((std::disjunction_v<std::is_same<Args, ImNodesCol_>, 
                                          std::is_same<Args, ImGuiCol_>> && ...), 
                      "only support type ImNodesCol_ or type ImGuiCol_");

        (SetOpacity(cols), ...);
    }

    ~OpacitySetter()
    {
        IMNODES_POP_STYLE_COL(m_imnodesCount);
        IMGUI_POP_STYLE_COL(m_imguiCount);
    }
private:
    float           m_destOpacity;
    uint32_t        m_imguiCount;
    uint32_t        m_imnodesCount;

};


} // end namespace SimpleNodeEditor

#endif // HELPERS_H