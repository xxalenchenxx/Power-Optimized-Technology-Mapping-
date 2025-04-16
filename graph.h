#ifndef GRAPH_H
#define GRAPH_H
#include <stdlib.h>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
using namespace std;
bool greaterCompare(double a, double b) {
    return a > b;
}
class Node
{
    friend class Graph;

public:
    // Constructor and destructor
    Node(const int& id) :
        _id(id), _fanIn(NULL), _fanOut(NULL) {}
    ~Node() { }

    // Basic access methods
    int getId() const                   { return _id; }
    int getFanIn(int fanInNum) const    { return _fanIn[fanInNum]; }
    int getFanOut(int fanOutNum) const  { return _fanOut[fanOutNum]; }
    int getFanInSize()                  { return _fanIn.size(); }
    int getFanOutSize()                 { return _fanOut.size(); }
    double getLevel()                   { return _level; }
    string getName()                    { return _Name; }
    string getGateType()                { return _gateType; }
    string getPatternType()             { return _patternType; }
    double getArTime()                  { return _arrivalTime; }
    double getRqTime()                  { return _requireTime; }
    double getSlack()                   { return _slack; }
    double getPower()                   { return _power; }
    double getDelay()                   { return _delay; }

    // Set functions
    void setId(int id)                          { _id = id; }
    void addFanIn(int fanInId)                  { _fanIn.push_back(fanInId); }
    void addFanOut(int fanOutId)                { _fanOut.push_back(fanOutId); }
    void setLevel(double level)                 { _level = level; }
    void setName(string name)                   { _Name = name; } 
    void setGateType(string gateType)           { _gateType = gateType; }
    void setPatternType(string patternType)     { _patternType = patternType; }
    void setArTime(double arrivalTime)          { _arrivalTime = arrivalTime; }
    void setRqTime(double requireTime)          { _requireTime = requireTime; }
    void setSlack(double slack)                 { _slack = slack; }
    void setPower(double power)                 { _power = power; }
    void setDelay(double delay)                 { _delay = delay; }

private:
    int             _id;    // id of the node (indicating the cell)
    double          _level;
    string          _Name;
    string          _gateType;
    string          _patternType;
    double          _arrivalTime;
    double          _requireTime;
    double          _slack;
    double          _power;
    double          _delay;
    vector<int>     _fanIn;  
    vector<int>     _fanOut; 
};

class Graph
{
public:
    // Constructor and destructor
    Graph() :
        _initialDelay(0), _totalPower(0), _NodeArray(NULL), _criticalPath(NULL) { }
    ~Graph() { }

    // Basic access methods
    double getInitialDelay()                    { return _initialDelay; }
    double getTotalPower()                      { return _totalPower; }
    double getMinimizedPower()                  { return _MinimizedPower; }
    Node* getNode(int nodeId)                   { return _NodeArray[nodeId]; }
    Node* getPi(int piId)                       { return _PiArray[piId]; }
    Node* getPo(int poId)                       { return _PoArray[poId]; }

    
    Node* getCritical(int criticalId)           { return _criticalPath[criticalId]; }
    double getLevel(int i)                      { return _LevelArray[i]; }
    int getNodeArraySize()                      { return _NodeArray.size(); }
    int getPiArraySize()                        { return _PiArray.size(); }
    int getPoArraySize()                        { return _PoArray.size(); }
    int getCriticalSize()                       { return _criticalPath.size(); }
    double getLevelArraySize()                  { return _LevelArray.size(); }
    Node* idToNode(int id)                      { return _Id2Node[id]; }
    vector<int> levelToList(double level)       { return _Level2NodeList[level]; }

    // Set functions
    void setInitialDelay(double initialDelay)                       { _initialDelay = initialDelay; }
    void setTotalPower(double totalPower)                           { _totalPower = totalPower; }
    void setMinimizedPower(double minimizedPower)                   { _MinimizedPower = minimizedPower; }
    void addNode(Node* node)                                        { _NodeArray.push_back(node); }
    void addPi(Node* pi)                                            { _PiArray.push_back(pi); }
    void addPo(Node* po)                                            { _PoArray.push_back(po); }
    void addCritical(Node* critical)                                { _criticalPath.push_back(critical); }
    void insertMap(int id, Node* insertNode)                        { _Id2Node.insert(pair<int, Node*> (id,insertNode)); }
    void resetCriticalPath()                                        { _criticalPath.clear(); }
    void insertLevelList(double level, int id)  
    { 
        if (_Level2NodeList.find(level) == _Level2NodeList.end())
        {
            _LevelArray.push_back(level);
            _Level2NodeList[level] = { id };
        }
        else
            _Level2NodeList[level].push_back(id);
    }
    void sortLevelArray(int mode)                       
    { 
        if(mode == 0)
            sort(_LevelArray.begin(), _LevelArray.end()); 
        else if(mode == 1)
            sort(_LevelArray.begin(), _LevelArray.end(), greaterCompare);
    }

private:
    double                      _initialDelay;
    double                      _totalPower;
    double                      _MinimizedPower;
    vector<Node*>               _NodeArray;
    vector<Node*>               _PiArray;
    vector<Node*>               _PoArray;
    vector<double>              _LevelArray;
    vector<Node*>               _criticalPath;
    map<int, Node*>             _Id2Node;
    map<double, vector<int>>    _Level2NodeList;
};

#endif  // GRAPH_H