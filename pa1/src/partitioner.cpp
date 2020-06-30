#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cassert>
#include <vector>
#include <cmath>
#include <map>
#include "cell.h"
#include "net.h"
#include "partitioner.h"
using namespace std;


void Partitioner::parseInput(fstream& inFile)
{
    string str;
    // Set balance factor
    inFile >> str;
    _bFactor = stod(str);

    // Set up whole circuit
    while (inFile >> str) {
        if (str == "NET") {
            string netName, cellName, tmpCellName = "";
            inFile >> netName;
            int netId = _netNum;
            _netArray.push_back(new Net(netName));
            _netName2Id[netName] = netId;
            while (inFile >> cellName) {
                if (cellName == ";") {
                    tmpCellName = "";
                    break;
                }
                else {
                    // a newly seen cell
                    if (_cellName2Id.count(cellName) == 0) {
                        int cellId = _cellNum;
                        _cellArray.push_back(new Cell(cellName, 0, cellId));
                        _cellName2Id[cellName] = cellId;
                        _cellArray[cellId]->addNet(netId);
                        _cellArray[cellId]->incPinNum();
                        _netArray[netId]->addCell(cellId);
                        ++_cellNum;
                        tmpCellName = cellName;
                    }
                    // an existed cell
                    else {
                        if (cellName != tmpCellName) {
                            assert(_cellName2Id.count(cellName) == 1);
                            int cellId = _cellName2Id[cellName];
                            _cellArray[cellId]->addNet(netId);
                            _cellArray[cellId]->incPinNum();
                            _netArray[netId]->addCell(cellId);
                            tmpCellName = cellName;
                        }
                    }
                }
            }
            ++_netNum;
        }
    }
    return;
}

void Partitioner::printSummary() const
{
    cout << endl;
    cout << "==================== Summary ====================" << endl;
    cout << " Cutsize: " << _cutSize << endl;
    cout << " Total cell number: " << _cellNum << endl;
    cout << " Total net number:  " << _netNum << endl;
    cout << " Cell Number of partition A: " << _partSize[0] << endl;
    cout << " Cell Number of partition B: " << _partSize[1] << endl;
    cout << "=================================================" << endl;
    cout << endl;
    return;
}

void Partitioner::reportNet() const
{
    cout << "Number of nets: " << _netNum << endl;
    for (size_t i = 0, end_i = _netArray.size(); i < end_i; ++i) {
        cout << setw(8) << _netArray[i]->getName() << ": ";
        vector<int> cellList = _netArray[i]->getCellList();
        for (size_t j = 0, end_j = cellList.size(); j < end_j; ++j) {
            cout << setw(8) << _cellArray[cellList[j]]->getName() << " ";
        }
        cout << endl;
    }
    return;
}

void Partitioner::reportCell() const
{
    cout << "Number of cells: " << _cellNum << endl;
    for (size_t i = 0, end_i = _cellArray.size(); i < end_i; ++i) {
        cout << setw(8) << _cellArray[i]->getName() << ": ";
        vector<int> netList = _cellArray[i]->getNetList();
        for (size_t j = 0, end_j = netList.size(); j < end_j; ++j) {
            cout << setw(8) << _netArray[netList[j]]->getName() << " ";
        }
        cout << endl;
    }
    return;
}

void Partitioner::writeResult(fstream& outFile)
{
    stringstream buff;
    buff << _cutSize;
    outFile << "Cutsize = " << buff.str() << '\n';
    buff.str("");
    buff << _partSize[0];
    outFile << "G1 " << buff.str() << '\n';
    for (size_t i = 0, end = _cellArray.size(); i < end; ++i) {
        if (_cellArray[i]->getPart() == 0) {
            outFile << _cellArray[i]->getName() << " ";
        }
    }
    outFile << ";\n";
    buff.str("");
    buff << _partSize[1];
    outFile << "G2 " << buff.str() << '\n';
    for (size_t i = 0, end = _cellArray.size(); i < end; ++i) {
        if (_cellArray[i]->getPart() == 1) {
            outFile << _cellArray[i]->getName() << " ";
        }
    }
    outFile << ";\n";
    return;
}

void Partitioner::clear()
{
    for (size_t i = 0, end = _cellArray.size(); i < end; ++i) {
        delete _cellArray[i];
    }
    for (size_t i = 0, end = _netArray.size(); i < end; ++i) {
        delete _netArray[i];
    }
    return;
}

void Partitioner::partition()
{
	this->buildPart();
	this->buildGain();
	int time = 0;
	while(1){
		bool result = this->setMaxGainCell();
		if(result == 0) break;
		else if(result == 1){
			this->updateGain();
			this->moveCell();
		} 	
	}
	this->backToBest();

}


void Partitioner::buildPart(){
	int i = 0, size = _cellArray.size(), border = size/2;
	int maxPinNum = 0;
	//Go over all Cells; O(sigma #pin)=O(P).
	for(int i=0; i<size; ++i){
		Cell* choscell = _cellArray[i];
		if(i<border){
			choscell->setPart(0);
			//Go over each Cell's connected Nets; O(#pin)
			vector<int> tmpNetList = choscell->getNetList();
			int sizeList = tmpNetList.size();
			for(int j=0; j<sizeList; ++j){
				int contNetId = tmpNetList[j];
				_netArray[contNetId]->incPartCount(0);
			}
		}
		else{
			choscell->setPart(1);
			//Go over each Cell's connected Nets; O(#pin)
			vector<int> tmpNetList = choscell->getNetList();
			int sizeList = tmpNetList.size(); 
			for(int j=0; j<sizeList; ++j){
				int contnetId = tmpNetList[j];
				_netArray[contnetId]->incPartCount(1);
			}
		}
		//To find maxPinNum among all cells
		if(choscell->getPinNum()>maxPinNum) maxPinNum = choscell->getPinNum();	
	}
	_maxPinNum = maxPinNum;
	//set _partSize and _cutSize
	_partSize[0]  = border;
	_partSize[1] = (size - border);
	
	int sizeArray = _netArray.size();
	for(int k=0; k<sizeArray; ++k){
		Net* eachnet = _netArray[k];
		if(eachnet->getPartCount(0)>0 && eachnet->getPartCount(1)>0) ++_cutSize;
	}
} 

void Partitioner::buildGain(){
	//Go over each Net and their connected Cells, compute the Cells' Gain; O(simga #pin)=O(P).
	int sizeArray = _netArray.size(); 
	for(int i=0; i<sizeArray; ++i){
		Net* eachNet = _netArray[i];
		if(eachNet->getCellList().size()>1){
			vector<int> tmpCellList = eachNet->getCellList();
			int sizeList = tmpCellList.size();
			for(int j=0; j<sizeList; ++j){
				int contCellId = tmpCellList[j];
				Cell* contCell = _cellArray[contCellId];
				int F=-1, T=-1;
				if(contCell->getPart()==0) F=eachNet->getPartCount(0), T=eachNet->getPartCount(1);
				else                       F=eachNet->getPartCount(1), T=eachNet->getPartCount(0);
				assert(F+T>1);
				if(F==1) contCell->incGain();
				if(T==0) contCell->decGain();
			}
		}
	}
	//To construct bList[0] for A and bList[1] for B.
	int sizeA = _cellArray.size();
	for(int i=0; i<sizeA; ++i){
		Cell* eachCell = _cellArray[i];
		this->addNode(eachCell->getNode());
	}
}

bool Partitioner::setMaxGainCell(){
	//To decide choose which Part
	bool chosPart;
	bool lockPart[2];
	//Can not move any cell from Part0
	lockPart[0] = (_bList[0].size()==0 || _partSize[0]-1<(1-_bFactor)/2*_cellNum);
	//Can not move any cell from Part1
	lockPart[1] = (_bList[1].size()==0 || _partSize[1]-1<(1-_bFactor)/2*_cellNum);
	if(lockPart[0] && lockPart[1])       return 0;
	else if(lockPart[0] && !lockPart[1]) chosPart = 1;
	else if(!lockPart[0] && lockPart[1]) chosPart = 0;
	else if(!lockPart[0] && !lockPart[1]){
		map<int, Node*>::iterator maxIter0 = _bList[0].end(); --maxIter0;
		map<int, Node*>::iterator maxIter1 = _bList[1].end(); --maxIter1;
		if(maxIter0->first >= maxIter1->first)	chosPart = 0;
		else                                    chosPart = 1;
	}
	
	//Start to decide the cell to move in _bList[chosPart] 
	assert(!lockPart[chosPart]);
	map<int, Node*>::iterator maxIter = _bList[chosPart].end(); --maxIter;
	Node* moveNode = maxIter->second;
	//To remove Node from _bList[chosPart]
	assert(chosPart == _cellArray[moveNode->getId()]->getPart());
	this->removeNode(moveNode);
	
    _maxGainCell = moveNode;
    return 1;
}

void Partitioner::updateGain(){
	//Go over the connected Net of chosen Cell (_maxGainCell).
	Cell* moveCell = _cellArray[_maxGainCell->getId()];
	moveCell->lock();
	vector<int> tmpNetList = moveCell->getNetList();
	int sizeNet = tmpNetList.size();
	for(int i=0; i<sizeNet; ++i){
		int contNetId = tmpNetList[i];
		Net* contNet  = _netArray[contNetId];
		bool chosPart = moveCell->getPart();
		//On each connected Net, decide the delta gain on the connected Cell
		int F = contNet->getPartCount(chosPart), T = contNet->getPartCount(!chosPart);
		int deltaF = 0, deltaT = 0; 
		if(F==2) ++deltaF;
		if(F==1) ++deltaT;
		if(T==1) ++deltaT;
		if(T==0) ++deltaF;
		//For each connected Cell, visit its conected Cell and update the value 
		vector<int> tmpCellList = contNet->getCellList();
		int sizeCell = tmpCellList.size();
		for(int j=0; j<sizeCell; ++j){
			int contCellId = tmpCellList[j];
			Cell* contCell = _cellArray[contCellId];
			//For the Cell with the same Part(old Part)
			if(!contCell->getLock() && contCell->getPart()==chosPart && deltaF>0){
				//Remove old Gain
                this->removeNode(contCell->getNode());
				//Update Gain value
				assert(deltaF==1||deltaF==2);
				if(deltaF==1){
					contCell->incGain();
				}
				else if(deltaF==2){
					contCell->incGain(); contCell->incGain();
				}
				//Add the new Gain now
                this->addNode(contCell->getNode());
            }
			//For the Cell with different Part(old Part)
			else if(!contCell->getLock() && contCell->getPart()!=chosPart && deltaT>0){
				//Remove old Gain first
				this->removeNode(contCell->getNode());
				//Update Gain value
				assert(deltaT==1||deltaT==2);
				if(deltaT==1){
					contCell->decGain();
				}
				else if(deltaT==2){
					contCell->decGain(); contCell->decGain();
				}
				//Add the new Gain now
                this->addNode(contCell->getNode());
			}
			//Didn't Need to do anything
			else{
			}
		}
	} 
} 

void Partitioner::removeNode(Node* rmNode){
	Cell* rmCell = _cellArray[rmNode->getId()]; 
	bool noPrev = (rmNode->getPrev()==NULL), noNext = (rmNode->getNext()==NULL);
	if(noPrev&&noNext){
		map<int, Node*>::iterator iter = _bList[rmCell->getPart()].find(rmCell->getGain());
		assert( iter != _bList[rmCell->getPart()].end());
		assert( iter->second == rmNode);
		_bList[rmCell->getPart()].erase(iter);
	}
	else if(noPrev&&~noNext){
		map<int, Node*>::iterator iter = _bList[rmCell->getPart()].find(rmCell->getGain());
		assert( iter != _bList[rmCell->getPart()].end());
		assert(iter->second == rmNode);
		iter->second = rmNode->getNext();
		rmNode->getNext()->setPrev(NULL);
		rmNode->setNext(NULL);
		
	}
	else if(~noPrev&&noNext){
		rmNode->getPrev()->setNext(NULL);
		rmNode->setPrev(NULL);
	}
	else if(~noPrev&&~noNext){
		(rmNode->getPrev())->setNext(rmNode->getNext());
		(rmNode->getNext())->setPrev(rmNode->getPrev());
		rmNode->setNext(NULL);
		rmNode->setPrev(NULL); 
	}
	else{
    	assert(0);
	}
	
}

void Partitioner::addNode(Node* adNode){
	Cell* adCell = _cellArray[adNode->getId()];
	map<int, Node*>::iterator newiter = _bList[adCell->getPart()].find(adCell->getGain());
	if(newiter==_bList[adCell->getPart()].end()){
		_bList[adCell->getPart()].insert(pair<int, Node*> (adCell->getGain(), adCell->getNode()));
	}
	else if(newiter!=_bList[adCell->getPart()].end()){
		Node* firstNode = newiter->second;
		while(firstNode->getNext()!=NULL) firstNode = firstNode->getNext();
		firstNode->setNext(adNode); 
		adNode->setPrev(firstNode);
	}
	else{
		assert(0);
	}
	
} 

void Partitioner::moveCell(){
	//To lock and move this Cell
	Cell* moveCell = _cellArray[_maxGainCell->getId()];
	bool  chosPart = moveCell->getPart();
	//Move function on this Cell
	moveCell->move();	//moveCell->lock() -> We have locked in updateGain()
	//Move function on the connected Nets of this Cell
	vector<int> tmpNetList = moveCell->getNetList();
	int sizeList = tmpNetList.size();
	for(int i=0; i<sizeList; ++i){
		int contNetId = tmpNetList[i];
		Net* contNet = _netArray[contNetId];
		contNet->decPartCount(chosPart);	//PartCount is for F and T
		contNet->incPartCount(!chosPart);
	}
	//Move function on partition 
	--_partSize[chosPart];
	++_partSize[!chosPart];
	++_moveNum;
	_accGain += moveCell->getGain();
	if(_accGain>_maxAccGain){
		_maxAccGain  = _accGain;
		_bestMoveNum = _moveNum; 
	}
	_moveStack.push_back(_maxGainCell->getId());
}

void Partitioner::backToBest(){
	for(int i=0; i<_bestMoveNum; ++i){
		int wantCellId = _moveStack[i];
		Cell* wantCell  = _cellArray[wantCellId];
		++_partSize[wantCell->getPart()];
		--_partSize[!wantCell->getPart()]; 
	}
	for(int i=_bestMoveNum; i<_moveNum; ++i){
		int throwCellId = _moveStack[i];
		Cell* throwCell = _cellArray[throwCellId];
		throwCell->move(); 
	}
	_cutSize -= _maxAccGain;
}

void Partitioner::reportbList(){
	cout << "In the bList[0]" << endl;
	for(map<int, Node*>::iterator iter = _bList[0].begin(); iter!=_bList[0].end(); iter++){
		if(iter->second!=NULL){
			Node* sameGainNode = iter->second;
			cout << "Gain value = " << iter->first << endl;
			while(sameGainNode != NULL){
				cout << "The Node and Cell with the same Gain has name: " << _cellArray[sameGainNode->getId()]->getName() << endl;
				sameGainNode = sameGainNode->getNext(); 
				
			}
		}
	}
	cout << endl;
	cout << endl;
	cout << "In the bList[1]" << endl;
	for(map<int, Node*>::iterator iter = _bList[1].begin(); iter!=_bList[1].end(); iter++){
		if(iter->second!=NULL){
			Node* sameGainNode = iter->second;
			cout << "Gain value = " << iter->first << endl;
			while(sameGainNode != NULL){
				cout << "The Node and Cell with the same Gain has name: " << _cellArray[sameGainNode->getId()]->getName() << endl;
				sameGainNode = sameGainNode->getNext(); 
				
			}
		}
	}
}