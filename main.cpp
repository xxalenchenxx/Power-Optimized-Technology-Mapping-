#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <queue>
#include <cmath>
#include <cfloat>
#include <map>
#include "./abc90/src/base/abc/abc.h"
#include "graph.h"
using namespace std;

extern "C" {
    extern void        Abc_Start();
    extern void        Abc_Stop();
    extern void        util_getopt_reset ARGS((void));
    extern void        PrintEachObj(Abc_Ntk_t *ntk); 
    extern Abc_Ntk_t * Io_ReadBlifAsAig(char *, int);
    extern unsigned    Abc_ObjId( Abc_Obj_t * pObj );
    extern int         Abc_NtkIsAigNetlist( Abc_Ntk_t * pNtk );
    extern int         Abc_NtkLevel( Abc_Ntk_t * pNtk );
    extern int         Abc_ObjFaninNum( Abc_Obj_t * pObj );   
    extern int         Abc_ObjFaninId( Abc_Obj_t * pObj, int i);
    extern int         Abc_ObjFaninC( Abc_Obj_t * pObj, int i );
    extern int         Abc_ObjFanoutNum( Abc_Obj_t * pObj );
    extern int         Abc_ObjLevel( Abc_Obj_t * pObj );
    extern int         Abc_NtkObjNum( Abc_Ntk_t * pNtk );
    extern int         Abc_NtkPoNum( Abc_Ntk_t * pNtk );
    extern Abc_Obj_t * Abc_NtkObj( Abc_Ntk_t * pNtk, int i );
    extern Abc_Obj_t * Abc_NtkPo( Abc_Ntk_t * pNtk, int i );
    extern Abc_Obj_t * Abc_ObjFanin( Abc_Obj_t * pObj, int i );
    extern char *      Abc_ObjName( Abc_Obj_t * pNode );
}
Graph* graph = new Graph();
double ASAP()
{
    double MaxArTime = 0;
    for(int i=0; i<graph->getLevelArraySize(); i++)
    {
        vector<int> TargetLevelList = graph->levelToList(graph->getLevel(i));
        for(long unsigned int j=0; j<TargetLevelList.size(); j++)
        {
            Node* TargetNode = graph->idToNode(TargetLevelList[j]);
            for(int k=0; k<TargetNode->getFanOutSize(); k++)
            {
                Node* TargetFanOutNode = graph->idToNode(TargetNode->getFanOut(k));
                if( (TargetNode->getArTime() + TargetFanOutNode->getDelay()) > (TargetFanOutNode->getArTime()) )
                    TargetFanOutNode->setArTime(TargetNode->getArTime() + TargetFanOutNode->getDelay());
                if( MaxArTime < TargetNode->getArTime() + TargetFanOutNode->getDelay())
                    MaxArTime = TargetNode->getArTime() + TargetFanOutNode->getDelay();
            }
        }
    }
    return MaxArTime;
}

void ALAP(double MaxArTime, double tolerance)
{
    for(int i=0; i<graph->getLevelArraySize(); i++)
    {
        vector<int> TargetLevelList = graph->levelToList(graph->getLevel(i));
        for(long unsigned int j=0; j<TargetLevelList.size(); j++)
        {
            Node* TargetNode = graph->idToNode(TargetLevelList[j]);
            if(TargetNode->getFanOutSize() == 0)
                    TargetNode->setRqTime(MaxArTime);
            for(int k=0; k<TargetNode->getFanInSize(); k++)
            {
                Node* TargetFanInNode = graph->idToNode(TargetNode->getFanIn(k));
                if( (TargetNode->getRqTime() - TargetNode->getDelay()) < (TargetFanInNode->getRqTime()) )
                {
                    if(abs(TargetNode->getRqTime() - TargetNode->getDelay()) <= tolerance)
                        TargetFanInNode->setRqTime(0);
                    else
                        TargetFanInNode->setRqTime(TargetNode->getRqTime() - TargetNode->getDelay());
                }               
            }
        }
    }

}

void BacktraceCritical(double eps)
{
    graph->resetCriticalPath();
    queue<Node*> q;

    // 1. 從所有 PO 開始
    for (int i = 0; i < graph->getPoArraySize(); ++i) {
        Node* po = graph->getPo(i);
        if (fabs(po->getArTime() - graph->getInitialDelay()) <= eps) {
            po->setSlack(0);
            graph->addCritical(po);
            q.push(po);
        }
    }

    // 2. back-trace
    while (!q.empty()) {
        Node* cur = q.front(); q.pop();
        for (int k = 0; k < cur->getFanInSize(); ++k) {
            Node* pred = graph->idToNode(cur->getFanIn(k));
            double edgeDelay = cur->getDelay();   // 或存 edge delay
            if (fabs(pred->getArTime() + edgeDelay - cur->getArTime()) <= eps) {
                if (pred->getSlack() > 0) pred->setSlack(0);
                graph->addCritical(pred);
                q.push(pred);
            }
        }
    }
}

bool CalSlackAndPower(double tol)
{
    bool timingViol = false;
    graph->setTotalPower(0);

    // 先算 slack、累功耗
    for (int i = 0; i < graph->getNodeArraySize(); ++i) {
        Node* n = graph->getNode(i);
        double diff = n->getRqTime() - n->getArTime();
        double slack = (fabs(diff) <= tol) ? 0 : diff;
        n->setSlack(slack);
        graph->setTotalPower(graph->getTotalPower() + n->getPower());
        if (slack < 0) timingViol = true;
    }

    // 再用路徑回溯標 Critical
    const double CRIT_EPS = 1e-4;
    BacktraceCritical(CRIT_EPS);

    return timingViol;
}




bool CheckCritical(int nodeId)
{
    bool flag = false;
    for(int i=0; i<graph->getCriticalSize(); i++)
    {
        if( nodeId == graph->getCritical(i)->getId() )
            flag = true;
    }
    return flag;
}

void GlobalPowerMinimize(double tolerance) {
    bool updated = true;
    while (updated) {
        updated = false;
        graph->setTotalPower(0);

        for (int i = 0; i < graph->getLevelArraySize(); ++i) {
            vector<int> nodes = graph->levelToList(graph->getLevel(i));
            for (int j = 0; j < nodes.size(); ++j) {
                Node* node = graph->idToNode(nodes[j]);

                // 原本延遲與功耗
                double originalDelay = node->getDelay();
                double originalPower = node->getPower();
                string originalPattern = node->getPatternType();

                // 嘗試替換各種 pattern
                struct GateOption {
                    string pattern;
                    double delayBase;
                    double delayFactor;
                    double power;
                };

                vector<GateOption> options;

                if (node->getGateType() == "NAND") {
                    options = {
                        {"NAND2", 2.31, 5.95, 1.4},
                        {"NAND3", 1.16, 3.0, 10.82}
                    };
                } else if (node->getGateType() == "INV") {
                    options = {
                        {"INV2", 1.03, 2.64, 1},
                        {"INV3", 0.47, 1.2, 2.75}
                    };
                }

                for (auto& opt : options) {
                    double newDelay = opt.delayBase + opt.delayFactor * node->getFanOutSize();
                    double delayInc = newDelay - originalDelay;
                    //只要新的delay<=原本slack時間+忍受值 且 power有下降
                    if (delayInc <= node->getSlack() + tolerance  && originalPower > opt.power) {
                        // 成功替換
                        node->setPatternType(opt.pattern);
                        node->setDelay(newDelay);
                        node->setPower(opt.power);
                        updated = true;
                        break; // 每個 node 只換一種最合適 pattern
                    }
                }
            }
        }

        // Timing Re-evaluation
        double maxAr = ASAP();
        graph->sortLevelArray(1);
        ALAP(maxAr, tolerance);
        CalSlackAndPower(tolerance); // 更新 slack 與 totalPower
        graph->setMinimizedPower(graph->getTotalPower());
    }
}


int main(int argc, char** argv)
{
	Abc_Ntk_t *ntk;
	cout<<"Read lib: "<<argv[2]<<endl;
    string inputFile, outputFile;

	// Setup ABC
	Abc_Start();
	
    // Read blif file
    if (!(ntk = Io_ReadBlifAsAig(argv[1], 1))) 
        return 1; 
    inputFile = argv[1];
    outputFile = inputFile.substr(inputFile.rfind('/')+1, inputFile.rfind('.')-inputFile.rfind('/')-1) + ".mbench";
    printf("outputfile: %s\n", outputFile.c_str());
	//Extract node
	int invId, invNum;
    double tolerance = 1e-9;    //計算slack的誤差臨界(福點數用)
    invId = Abc_NtkObjNum(ntk); //總共邏輯元件 (包含 PI,PO,gate)，之後要新增INV時，可以使用這個做為新的ID
    invNum = 0;

    //建立node(中間邏輯以及PI)的graph資訊
	for(int i=0; i<Abc_NtkObjNum(ntk); i++)
    {   
        Abc_Obj_t * i_Obj = Abc_NtkObj(ntk, i);
        if(Abc_ObjType(i_Obj) == 7) //node 類型 (type == 7)
        {
            Abc_Obj_t * i_Obj = Abc_NtkObj(ntk, i);
            Node* graphNandNode = new Node(Abc_ObjId(i_Obj)); //新增gate的node,初始ID
            //紀錄gate相關資訊至node(預設為NAND1) time delay最小，但power最大
            graphNandNode->setLevel(Abc_ObjLevel(i_Obj));
            graphNandNode->setName(Abc_ObjName(i_Obj));
            graphNandNode->setGateType("NAND");
            graphNandNode->setPatternType("NAND1");
            graphNandNode->setArTime(0);
            graphNandNode->setRqTime(DBL_MAX);
            graphNandNode->setPower(25.76);
            graphNandNode->setDelay(0.56+1.44*(double)Abc_ObjFanoutNum(i_Obj)); //0.56+1.44 * #Fanout

            graph->addNode(graphNandNode);
            graph->insertMap(graphNandNode->getId(), graphNandNode);
            graph->insertLevelList(graphNandNode->getLevel(), graphNandNode->getId());
            
            for(int j=0; j<Abc_ObjFaninNum(i_Obj); j++) //紀錄fanin的INV
            {
                if( Abc_ObjType(Abc_ObjFanin(i_Obj,j)) == 7) //node類型
                {
                    if( Abc_ObjFaninC(i_Obj,j) == 0 )//phase complement == 1 (NAND)
                    {
                        Node* graphInvNode = new Node(invId);   
                        graph->idToNode(Abc_ObjFaninId(i_Obj,j))->addFanOut(invId);    
                        graphInvNode->addFanOut(i);                                                 
                        graphInvNode->addFanIn(Abc_ObjFaninId(i_Obj,j));               
                        graphNandNode->addFanIn(invId);                                            

                        graphInvNode->setLevel(graphNandNode->getLevel()-0.5);
                        graphInvNode->setName("INV"+to_string(invNum)); //新增的INV給新的名稱
                        graphInvNode->setGateType("INV");
                        graphInvNode->setPatternType("INV1");
                        graphInvNode->setArTime(0);
                        graphInvNode->setRqTime(DBL_MAX);
                        graphInvNode->setPower(20.92);
                        graphInvNode->setDelay(1.0);
                        graph->addNode(graphInvNode);
                        graph->insertMap(graphInvNode->getId(), graphInvNode);
                        graph->insertLevelList(graphInvNode->getLevel(), graphInvNode->getId());
                        invId++;
                        invNum++;
                    }
                    else if( Abc_ObjFaninC(i_Obj,j) == 1 )//phase complement == 0
                    {
                        graph->idToNode(Abc_ObjFaninId(i_Obj,j))->addFanOut(i);        
                        graphNandNode->addFanIn(Abc_ObjFaninId(i_Obj,j));              
                    }
                }
                else if( Abc_ObjType(Abc_ObjFanin(i_Obj,j)) == 2) //PI
                {
                    if( Abc_ObjFaninC(i_Obj,j) == 0 )//phase == 0
                    {
                        graph->idToNode(Abc_ObjFaninId(i_Obj,j))->addFanOut(i);        
                        graphNandNode->addFanIn(Abc_ObjFaninId(i_Obj,j));              
                    }
                    else if( Abc_ObjFaninC(i_Obj,j) == 1 )//phase == 1
                    {
                        Node* graphInvNode = new Node(invId);
                        graph->idToNode(Abc_ObjFaninId(i_Obj,j))->addFanOut(invId);    
                        graphInvNode->addFanOut(i);                                                 
                        graphInvNode->addFanIn(Abc_ObjFaninId(i_Obj,j));               
                        graphNandNode->addFanIn(invId);                                             

                        graphInvNode->setLevel(graphNandNode->getLevel()-0.5);
                        graphInvNode->setName("INV"+to_string(invNum));
                        graphInvNode->setGateType("INV");
                        graphInvNode->setPatternType("INV1");
                        graphInvNode->setArTime(0);
                        graphInvNode->setRqTime(DBL_MAX);
                        graphInvNode->setPower(20.92);
                        graphInvNode->setDelay(1.0);
                        graph->addNode(graphInvNode);
                        graph->insertMap(graphInvNode->getId(), graphInvNode);
                        graph->insertLevelList(graphInvNode->getLevel(), graphInvNode->getId());
                        invId++;
                        invNum++;
                    }
                }
            }
        }
        else if(Abc_ObjType(i_Obj) == 2) //Primary Input (PI)
        {
            Node* graphPiNode = new Node(Abc_ObjId(i_Obj));
            graphPiNode->setLevel(Abc_ObjLevel(i_Obj));
            graphPiNode->setName(Abc_ObjName(i_Obj));
            graphPiNode->setGateType("PI");
            graphPiNode->setPatternType("PI");
            graphPiNode->setArTime(0);
            graphPiNode->setRqTime(DBL_MAX);
            graphPiNode->setPower(0);
            graphPiNode->setDelay(0);
            graph->addNode(graphPiNode);
            graph->addPi(graphPiNode);
            graph->insertMap(graphPiNode->getId(), graphPiNode);
            graph->insertLevelList(graphPiNode->getLevel(), graphPiNode->getId());      
        }
	}


    //建立Po的graph資訊
    for(int i=0; i<Abc_NtkPoNum(ntk); i++)
    {
        Abc_Obj_t* Po_obj = Abc_NtkPo(ntk, i);
        if( Abc_ObjFaninC(Po_obj,0) == 0 )//phase complement == 1
        {
            Node* graphInvNode = new Node(invId);
            graph->idToNode(Abc_ObjFaninId(Po_obj,0))->addFanOut(invId);    
            graphInvNode->addFanIn(Abc_ObjFaninId(Po_obj,0));               

            graphInvNode->setLevel(Abc_ObjLevel(Abc_ObjFanin(Po_obj,0))+0.5);
            graphInvNode->setName(Abc_ObjName(Po_obj));//"INV"+to_string(invNum)
            graphInvNode->setGateType("INV");
            graphInvNode->setPatternType("INV1");
            graphInvNode->setArTime(0);
            graphInvNode->setRqTime(DBL_MAX);
            graphInvNode->setPower(20.92);
            graphInvNode->setDelay(0.28);
            graph->addNode(graphInvNode);
            graph->insertMap(graphInvNode->getId(), graphInvNode);
            graph->insertLevelList(graphInvNode->getLevel(), graphInvNode->getId());
            invId++;
            invNum++;
            graph->addPo(graphInvNode);
        }
        else if(( Abc_ObjFaninC(Po_obj,0) == 1 ) &&  (Abc_ObjFaninId(Po_obj, 0) != 0 ))//phase complement == 0
        {                    
            graph->idToNode(Abc_ObjFaninId(Po_obj,0))->setDelay(0.56);
            graph->idToNode(Abc_ObjFaninId(Po_obj,0))->setName(Abc_ObjName(Po_obj));
            graph->addPo(graph->idToNode(Abc_ObjFaninId(Po_obj,0)));
        }
    }
    graph->sortLevelArray(0);
    

    //利用ASAP以及ALAP找到slack以及power
    double MaxArTime=0.0;
    bool TimingViolation = 0;
    MaxArTime = ASAP();                                 //ASAP
    graph->sortLevelArray(1);
    ALAP(MaxArTime, tolerance);                         //ALAP
    graph->setInitialDelay(MaxArTime);
    TimingViolation = CalSlackAndPower(tolerance);      //Slack&total power calculate   
    graph->setMinimizedPower(graph->getTotalPower());
    double originTotalPower = graph->getTotalPower();


    //Power Minimization
    double LastPower = originTotalPower;
    int it=0;

    #pragma region mini_power_No_critical
    // while(1)
    // {   
    //     printf("it: %d\n",it);
    //     for(int i=0; i<graph->getLevelArraySize(); i++)
    //     {
    //         vector<int> TargetLevelList = graph->levelToList(graph->getLevel(i));
    //         for(long unsigned int j=0; j<TargetLevelList.size(); j++)
    //         {
    //             Node* TargetNode = graph->idToNode(TargetLevelList[j]);
    //             if( CheckCritical(TargetLevelList[j]) ) //critical path上的點跳過
    //                 continue;
    //             if( TargetNode->getGateType() == "NAND" )
    //             {
    //                 if( TargetNode->getSlack() >= ( (2.31+5.95*TargetNode->getFanOutSize()) - TargetNode->getDelay() ) )
    //                 {
    //                     TargetNode->setPatternType("NAND2");
    //                     TargetNode->setDelay(2.31+5.95*TargetNode->getFanOutSize());
    //                     TargetNode->setPower(1.4);                       
    //                 }
    //                 else if ( TargetNode->getSlack() >= ( (1.16+3*TargetNode->getFanOutSize()) - TargetNode->getDelay() ) )
    //                 {
    //                     TargetNode->setPatternType("NAND3");
    //                     TargetNode->setDelay(1.16+3*TargetNode->getFanOutSize());
    //                     TargetNode->setPower(10.82);
    //                 }
    //             }
    //             else if( TargetNode->getGateType() == "INV" )
    //             {
    //                 if( TargetNode->getSlack() >= ( (1.03+2.64*TargetNode->getFanOutSize()) - TargetNode->getDelay()  ) )
    //                 {
    //                     TargetNode->setPatternType("INV2");
    //                     TargetNode->setDelay(1.03+2.64*TargetNode->getFanOutSize());
    //                     TargetNode->setPower(1);
    //                 }
    //                 else if ( TargetNode->getSlack() >= ( (0.47+1.2*TargetNode->getFanOutSize()) - TargetNode->getDelay() ) )
    //                 {
    //                     TargetNode->setPatternType("INV3");
    //                     TargetNode->setDelay(0.47+1.2*TargetNode->getFanOutSize());
    //                     TargetNode->setPower(2.75);
    //                 }
    //             }
    //         }
    //         graph->setTotalPower(0);
    //         MaxArTime = ASAP();
    //         graph->sortLevelArray(1);
    //         ALAP(MaxArTime, tolerance);
    //         TimingViolation = CalSlackAndPower(tolerance);
    //         graph->setMinimizedPower(graph->getTotalPower());     
    //     }
    //     if(LastPower == graph->getMinimizedPower())
    //         break;
    //     else if(LastPower != graph->getMinimizedPower() || TimingViolation) 
    //         LastPower =  graph->getMinimizedPower();
    //     it++;
    // }
    #pragma endregion
    
    // 進一步優化：允許 critical path 上也換
    GlobalPowerMinimize(tolerance);

    //final dedlay
    double final_delay = ASAP();                                 //ASAP
    //Write file
    ofstream ofs;
    ofs.open(outputFile);
    if (!ofs.is_open()) {
        cout << "Failed to open file.\n";
        return 1; // EXIT_FAILURE
    }
    ofs<<"Initial delay  : "<<graph->getInitialDelay()<<"\n";
    ofs<<"Final delay    : "<<final_delay<<"\n";
    ofs<<"Original  power: "<<originTotalPower<<"\n";
    ofs<<"Optimized power: "<<graph->getTotalPower()<<"\n";
    ofs<<"\n";
    for(int i=0; i<graph->getPiArraySize(); i++)
    {
        ofs<<"INPUT(";
        ofs<<graph->getPi(i)->getName();
        ofs<<")\n";
    }
    ofs<<"\n";
    for(int i=0; i<graph->getPoArraySize(); i++)
    {
        ofs<<"OUTPUT(";
        ofs<<graph->getPo(i)->getName();
        ofs<<")\n";
    }
    ofs<<"\n";
    
    for(int i=0; i<graph->getNodeArraySize(); i++)
    {
        int print = 8;
        if(graph->getNode(i)->getFanInSize() != 0)
        {
            ofs<<left<<setw(5)<<graph->getNode(i)->getName();
            ofs<<" = ";
            ofs<<left<<setw(5)<<graph->getNode(i)->getPatternType();
            ofs<<"(";
            for(int j=0; j<graph->getNode(i)->getFanInSize(); j++)
            {
                ofs<<left<<setw(6)<<graph->idToNode(graph->getNode(i)->getFanIn(j))->getName();
                if(graph->getNode(i)->getFanInSize() != j+1)
                {
                    ofs<<", ";
                }
            }
            if( graph->getNode(i)->getFanInSize() == 1 )
                print = 16;
            
            ofs<<")\t";
            ofs<<right<<setw(print+5)<<"#slack = ";
            ofs<<left<<setw(8)<<graph->getNode(i)->getSlack()<<"\n";

        }
    }
    ofs.close();
	cout<<"done!!"<<endl;
	// << End ABC >>
	Abc_Stop();
	
	return 1;
}
