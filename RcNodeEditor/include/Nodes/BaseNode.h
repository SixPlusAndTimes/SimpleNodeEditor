#ifndef BASENODE_H
#define BASENODE_H
#include <vector>
#include <set>
#include <string>

class BaseNode {
   public:
               BaseNode();
        bool   AddInputPort(const std::string& inputport);
        bool   AddOutputPort(const std::string& outputport);
   private: 
        std::set<std::string> m_inputPorts;
        std::set<std::string> m_outputPorts;
      
};

#endif // BASENODE_H