#ifndef HELPERS_H
#define HELPERS_H
#include <vector>
#include <unordered_map>
#include "Node.hpp"
#include "spdlog/spdlog.h"

std::vector<std::vector<NodeUniqueId>> TopologicalSort(std::unordered_map<NodeUniqueId, Node>& nodesMap, std::unordered_map<EdgeUniqueId, Edge>& edgesMap)
{
    std::vector<std::vector<NodeUniqueId>> result;
    if (nodesMap.size() ==  0 || edgesMap.size()  == 0)
    {
        SPDLOG_WARN("nodesMap or edgesMap size == 0");
        return result;
    }

    std::unordered_map<NodeUniqueId, int> degrees; 
    // init all node's degree to 0
    for (const auto& [nodeUid, _]: nodesMap)
    {
        degrees.emplace(nodeUid, 0);
    }

    // sum up all node's degree
    for (const auto& [_, edge] : edgesMap)
    {
        ++degrees.at(edge.GetDestinationNodeUid());
    }

    // collect zero degree node
    std::vector<NodeUniqueId> zeroDegreeNodes;
    for (const auto& [nodeUid, degree] : degrees)
    {
        if (!degree) {
            zeroDegreeNodes.push_back(nodeUid);
        }
    }

    while (zeroDegreeNodes.size() != 0)
    {
        result.push_back(zeroDegreeNodes);
        std::vector<NodeUniqueId> newZeroDegreeNodes;

        for (NodeUniqueId zeroDegreeNodeUid : zeroDegreeNodes)
        {
            for (const OutputPort outPort : nodesMap.at(zeroDegreeNodeUid).GetOutputPorts())
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

    // all nodes' dgree must be zero
    bool allNodeDegreeZero = true;
    for (const auto&[nodeUid, degree] : degrees)
    {
        if (degree != 0)
        {
            SPDLOG_ERROR("toposort done, but node[{}] has degree[{}]", nodeUid, degree);
            allNodeDegreeZero = false;
        }
    }
    assert(allNodeDegreeZero);

    return result;
}

#endif // HELPERS_H