#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
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
        for(int j=0; j<TargetLevelList.size(); j++)
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
        for(int j=0; j<TargetLevelList.size(); j++)
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

bool CalSlackAndPower(double tolerance)
{
    int flag = 0;
    graph->resetCriticalPath();
    for(int i=0; i<graph->getNodeArraySize(); i++)
    {
        double slack = 0;
        double difference = graph->getNode(i)->getRqTime() - graph->getNode(i)->getArTime();      
        if(abs(difference) <= tolerance)
            slack = 0;
        else
            slack = difference;
        graph->getNode(i)->setSlack(slack);

        if(slack == 0)
            graph->addCritical(graph->getNode(i));
        graph->setTotalPower(graph->getTotalPower()+graph->getNode(i)->getPower()); 
        if( slack < 0 )
            flag = 1;
    }
    if( flag== 1 )
        return 1;
    else if( flag== 0 )
        return 0;
}

bool CheckCritical(int nodeId)
{
    int flag = 0;
    for(int i=0; i<graph->getCriticalSize(); i++)
    {
        if( nodeId == graph->getCritical(i)->getId() )
            flag = 1;
    }
    if( flag == 1 )
        return 1;
    else if( flag == 0 )
        return 0;
}

int main(int argc, char** argv)
{
	Abc_Ntk_t *ntk;
	cout<<"Read lib: "<<argv[argc-1]<<endl;
    string inputFile, outputFile;

	// << Setup ABC >>
	Abc_Start();
	
    // << Read blif file >>
    if (!(ntk = Io_ReadBlifAsAig(argv[1], 1))) 
        return 1; 
    inputFile = argv[1];
    outputFile = inputFile.substr(inputFile.rfind('/')+1, inputFile.rfind('.')-inputFile.rfind('/')-1) + ".mbench";
	//Extract node
	int invId, invNum;
    double tolerance = 1e-9;
    invId = Abc_NtkObjNum(ntk);
    invNum = 0;
	for(int i=0; i<Abc_NtkObjNum(ntk); i++)
    {
        if(Abc_ObjType(Abc_NtkObj(ntk, i)) == 7)
        {
            
            Node* graphNandNode = new Node(Abc_ObjId(Abc_NtkObj(ntk, i)));
            graphNandNode->setLevel(Abc_ObjLevel(Abc_NtkObj(ntk, i)));
            graphNandNode->setName(Abc_ObjName(Abc_NtkObj(ntk, i)));
            graphNandNode->setGateType("NAND");
            graphNandNode->setPatternType("NAND1");
            graphNandNode->setArTime(0);
            graphNandNode->setRqTime(1.79769e+308);
            graphNandNode->setPower(25.76);
            graphNandNode->setDelay(0.56+1.44*Abc_ObjFanoutNum(Abc_NtkObj(ntk, i)));
            graph->addNode(graphNandNode);
            graph->insertMap(graphNandNode->getId(), graphNandNode);
            graph->insertLevelList(graphNandNode->getLevel(), graphNandNode->getId());
            for(int j=0; j<Abc_ObjFaninNum(Abc_NtkObj(ntk, i)); j++)
            {
                if( Abc_ObjType(Abc_ObjFanin(Abc_NtkObj(ntk, i),j)) == 7)
                {
                    if( Abc_ObjFaninC(Abc_NtkObj(ntk, i),j) == 0 )//phase complement == 1
                    {
                        Node* graphInvNode = new Node(invId);   
                        graph->idToNode(Abc_ObjFaninId(Abc_NtkObj(ntk, i),j))->addFanOut(invId);    
                        graphInvNode->addFanOut(i);                                                 
                        graphInvNode->addFanIn(Abc_ObjFaninId(Abc_NtkObj(ntk, i),j));               
                        graphNandNode->addFanIn(invId);                                            

                        graphInvNode->setLevel(graphNandNode->getLevel()-0.5);
                        graphInvNode->setName("INV"+to_string(invNum));
                        graphInvNode->setGateType("INV");
                        graphInvNode->setPatternType("INV1");
                        graphInvNode->setArTime(0);
                        graphInvNode->setRqTime(1.79769e+308);
                        graphInvNode->setPower(20.92);
                        graphInvNode->setDelay(0.28+0.72);
                        graph->addNode(graphInvNode);
                        graph->insertMap(graphInvNode->getId(), graphInvNode);
                        graph->insertLevelList(graphInvNode->getLevel(), graphInvNode->getId());
                        invId++;
                        invNum++;
                    }
                    else if( Abc_ObjFaninC(Abc_NtkObj(ntk, i),j) == 1 )//phase complement == 0
                    {
                        graph->idToNode(Abc_ObjFaninId(Abc_NtkObj(ntk, i),j))->addFanOut(i);        
                        graphNandNode->addFanIn(Abc_ObjFaninId(Abc_NtkObj(ntk, i),j));              
                    }
                }
                else if( Abc_ObjType(Abc_ObjFanin(Abc_NtkObj(ntk, i),j)) == 2)
                {
                    if( Abc_ObjFaninC(Abc_NtkObj(ntk, i),j) == 0 )//phase == 0
                    {
                        graph->idToNode(Abc_ObjFaninId(Abc_NtkObj(ntk, i),j))->addFanOut(i);        
                        graphNandNode->addFanIn(Abc_ObjFaninId(Abc_NtkObj(ntk, i),j));              
                    }
                    else if( Abc_ObjFaninC(Abc_NtkObj(ntk, i),j) == 1 )//phase == 1
                    {
                        Node* graphInvNode = new Node(invId);
                        graph->idToNode(Abc_ObjFaninId(Abc_NtkObj(ntk, i),j))->addFanOut(invId);    
                        graphInvNode->addFanOut(i);                                                 
                        graphInvNode->addFanIn(Abc_ObjFaninId(Abc_NtkObj(ntk, i),j));               
                        graphNandNode->addFanIn(invId);                                             

                        graphInvNode->setLevel(graphNandNode->getLevel()-0.5);
                        graphInvNode->setName("INV"+to_string(invNum));
                        graphInvNode->setGateType("INV");
                        graphInvNode->setPatternType("INV1");
                        graphInvNode->setArTime(0);
                        graphInvNode->setRqTime(1.79769e+308);
                        graphInvNode->setPower(20.92);
                        graphInvNode->setDelay(0.28+0.72);
                        graph->addNode(graphInvNode);
                        graph->insertMap(graphInvNode->getId(), graphInvNode);
                        graph->insertLevelList(graphInvNode->getLevel(), graphInvNode->getId());
                        invId++;
                        invNum++;
                    }
                }
            }
        }
        else if(Abc_ObjType(Abc_NtkObj(ntk, i)) == 2)
        {
            Node* graphPiNode = new Node(Abc_ObjId(Abc_NtkObj(ntk, i)));
            graphPiNode->setLevel(Abc_ObjLevel(Abc_NtkObj(ntk, i)));
            graphPiNode->setName(Abc_ObjName(Abc_NtkObj(ntk, i)));
            graphPiNode->setGateType("PI");
            graphPiNode->setPatternType("PI");
            graphPiNode->setArTime(0);
            graphPiNode->setRqTime(1.79769e+308);
            graphPiNode->setPower(0);
            graphPiNode->setDelay(0);
            graph->addNode(graphPiNode);
            graph->addPi(graphPiNode);
            graph->insertMap(graphPiNode->getId(), graphPiNode);
            graph->insertLevelList(graphPiNode->getLevel(), graphPiNode->getId());      
        }
	}

    for(int i=0; i<Abc_NtkPoNum(ntk); i++)
    {
        if( Abc_ObjFaninC(Abc_NtkPo(ntk, i),0) == 0 )//phase complement == 1
        {
            Node* graphInvNode = new Node(invId);
            graph->idToNode(Abc_ObjFaninId(Abc_NtkPo(ntk, i),0))->addFanOut(invId);    
            graphInvNode->addFanIn(Abc_ObjFaninId(Abc_NtkPo(ntk, i),0));               

            graphInvNode->setLevel(Abc_ObjLevel(Abc_ObjFanin(Abc_NtkPo(ntk, i),0))+0.5);
            graphInvNode->setName(Abc_ObjName(Abc_NtkPo(ntk, i)));//"INV"+to_string(invNum)
            graphInvNode->setGateType("INV");
            graphInvNode->setPatternType("INV1");
            graphInvNode->setArTime(0);
            graphInvNode->setRqTime(1.79769e+308);
            graphInvNode->setPower(20.92);
            graphInvNode->setDelay(0.28);
            graph->addNode(graphInvNode);
            graph->insertMap(graphInvNode->getId(), graphInvNode);
            graph->insertLevelList(graphInvNode->getLevel(), graphInvNode->getId());
            invId++;
            invNum++;
            graph->addPo(graphInvNode);
        }
        else if(( Abc_ObjFaninC(Abc_NtkPo(ntk, i),0) == 1 ) &&  (Abc_ObjFaninId(Abc_NtkPo( ntk, i ), 0) != 0 ))//phase complement == 0
        {                    
            graph->idToNode(Abc_ObjFaninId(Abc_NtkPo(ntk, i),0))->setDelay(0.56);
            graph->idToNode(Abc_ObjFaninId(Abc_NtkPo(ntk, i),0))->setName(Abc_ObjName(Abc_NtkPo(ntk, i)));
            graph->addPo(graph->idToNode(Abc_ObjFaninId(Abc_NtkPo(ntk, i),0)));
        }
    }
    graph->sortLevelArray(0);
    
    double MaxArTime;
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
    while(1)
    {
        for(int i=0; i<graph->getLevelArraySize(); i++)
        {
            vector<int> TargetLevelList = graph->levelToList(graph->getLevel(i));
            for(int j=0; j<TargetLevelList.size(); j++)
            {
                Node* TargetNode = graph->idToNode(TargetLevelList[j]);
                if( CheckCritical(TargetLevelList[j]) ) //TargetNode is on critical path
                    continue;

                if( TargetNode->getGateType() == "NAND" )
                {
                    if( TargetNode->getSlack() >= ( (2.31+5.95*TargetNode->getFanOutSize()) - TargetNode->getDelay() ) )
                    {
                        TargetNode->setPatternType("NAND2");
                        TargetNode->setDelay(2.31+5.95*TargetNode->getFanOutSize());
                        TargetNode->setPower(1.4);                       
                    }
                    else if ( TargetNode->getSlack() >= ( (1.16+3*TargetNode->getFanOutSize()) - TargetNode->getDelay() ) )
                    {
                        TargetNode->setPatternType("NAND3");
                        TargetNode->setDelay(1.16+3*TargetNode->getFanOutSize());
                        TargetNode->setPower(10.82);
                    }
                }
                else if( TargetNode->getGateType() == "INV" )
                {
                    if( TargetNode->getSlack() >= ( (1.03+2.64*TargetNode->getFanOutSize()) - TargetNode->getDelay()  ) )
                    {
                        TargetNode->setPatternType("INV2");
                        TargetNode->setDelay(1.03+2.64*TargetNode->getFanOutSize());
                        TargetNode->setPower(1);
                    }
                    else if ( TargetNode->getSlack() >= ( (0.47+1.2*TargetNode->getFanOutSize()) - TargetNode->getDelay() ) )
                    {
                        TargetNode->setPatternType("INV3");
                        TargetNode->setDelay(0.47+1.2*TargetNode->getFanOutSize());
                        TargetNode->setPower(2.75);
                    }
                }
            }
            graph->setTotalPower(0);
            MaxArTime = ASAP();
            graph->sortLevelArray(1);
            ALAP(MaxArTime, tolerance);
            TimingViolation = CalSlackAndPower(tolerance);
            graph->setMinimizedPower(graph->getTotalPower());     
        }
        if(LastPower == graph->getMinimizedPower())
            break;
        else if(LastPower != graph->getMinimizedPower()) 
            LastPower =  graph->getMinimizedPower();
    }
        

    //Write file
    ofstream ofs;
    ofs.open(outputFile);
    if (!ofs.is_open()) {
        cout << "Failed to open file.\n";
        return 1; // EXIT_FAILURE
    }
    ofs<<"Initial delay: "<<graph->getInitialDelay()<<"\n";
    ofs<<"Original power: "<<originTotalPower<<"\n";
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
