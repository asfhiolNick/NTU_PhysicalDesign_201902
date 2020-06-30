#include <iostream>
#include <fstream>
#include <vector>
#include <assert.h>
#include <math.h>
#include <time.h>
#include "floorplanner.h"
using namespace std;

void Floorplanner::parseInput_blk(fstream& inFile){
	string std;
	inFile >> std; //Outline:
	inFile >> std;	width = stod(std);
	
	inFile >> std;	height = stod(std);
    
	inFile >> std; //NumBlocks:
	inFile >> std;	numBlk = stod(std);
	
	inFile >> std; //NumTerminals:
	inFile >> std;	numTml = stod(std);
	
	for(int i=0; i<numBlk; ++i){
		string name;
		size_t w;
		size_t h;
		inFile >> std;	name = std;
		inFile >> std;  w    = (size_t) stod(std);
		inFile >> std;  h    = (size_t) stod(std);
		_blkList.push_back(new Block(name, w, h));
		if(_name2Blk.count(name) == 0) _name2Blk[name] = _blkList[i];
	}
	for(int i=0; i<numTml; ++i){
		string name;
		size_t x;
		size_t y;
		inFile >> std;	name = std;
		inFile >> std; //terminal
		inFile >> std;	x    = (size_t) stod(std);
		inFile >> std;  y    = (size_t) stod(std);
		_tmlList.push_back(new Terminal(name, x, y));
		if(_name2Tml.count(name) == 0) _name2Tml[name] = _tmlList[i];
	}
	return; 
} 

void Floorplanner::parseInput_net(fstream& inFile){
	string std;
	inFile >> std; //NumNets:
	inFile >> std;	numNet = stod(std);
	for(int i=0; i<numNet; ++i){
		int netDeg;
		inFile >> std; //NetDegree:
		inFile >> std;	netDeg = stod(std);
		
		_netList.push_back(new Net());
		for(int j=0; j<netDeg; ++j){
			string name;
			inFile >> std;	name = std;
			if(_name2Blk.count(name) > 0)	_netList[i]->addTerm(_name2Blk[name]);
			else{
				assert(_name2Tml.count(name) > 0);
				_netList[i]->addTerm(_name2Tml[name]); 
			}
		}
	}
	return;
}

void Floorplanner::packing(Macro* root){
	for(int i=0; i<numBlk; ++i){
		_macList.push_back(new Macro(_blkList[i]->getName()));
		if(_blkList[i]->getHeight()>_blkList[i]->getWidth()) _blkList[i]->setRotate();
	}
	
	root->setLeft(_macList[0]);
	_macList[0]->setPrev(root);
	
	for(int i=0; i<numBlk; ++i){
		int realidx = i+1;
		if(realidx*2-1 < numBlk){
			_macList[realidx*2-1]->setPrev(_macList[i]);
			_macList[i]->setLeft(_macList[realidx*2-1]);
		}
		if(realidx*2+1-1 < numBlk){
			_macList[realidx*2]->setPrev(_macList[i]);
			_macList[i]->setRight(_macList[realidx*2]);
		}
	}

	return;
}

void Floorplanner::floorplan(double alpha){
	packing(_BTreeRoot);
	for(double T=1000; T>1; T*=0.9){
		for(int i=0; i<2000*15/numBlk; ++i){
			//Compute cost(S) = cost1
			double X, H, W; 
			double cost1 = buildplan(alpha, X, H, W);
			clear();
		
			//Compute cost(S') = cost2
			double prob = (double) rand() / (RAND_MAX + 1.0);
			//Useless variable
			double x, h, w;
			double cost2;
			//prob < 0.33
			//OP1: To rotate one block
			if(prob < 0.33){
				int chosen = rand() % numBlk;
				_blkList[chosen]->setRotate();
				cost2 = buildplan(alpha,x ,h, w);
				clear();
				double prob2 = (double) rand() / (RAND_MAX + 1.0);
				if(cost1 < cost2 && prob2 >= exp((double)(cost1-cost2)/T)){
					_blkList[chosen]->setRotate();
				}
			}
			//0.33 <= prob < 0.66 
    	    else if(prob < 0.66){
    	    	//To count #leaf and choose which one
				int count = 0;
    	    	for(int i=0; i<_macList.size(); ++i){
   		     		if(_macList[i]->getLeft()==NULL && _macList[i]->getRight()==NULL) ++count;
				}
				int chosnum = (rand() % count) +1;
				assert(chosnum <= count);
				//To remove the chosen leaf
				bool done1 = 0;
				count = 0;
				int chosidx = 0;
				Macro* macro1 = NULL;
				Macro* parent = NULL;
				bool side1 = 0;
     		   	for(int i=0; i<_macList.size(); ++i){
    	    		if(_macList[i]->getLeft()==NULL && _macList[i]->getRight()==NULL) ++count;
    	    		if(count == chosnum){
    	    			chosidx = i;
    	    			macro1 = _macList[i]; 
    	    			parent = macro1->getPrev();
    	    			if(parent->getLeft()!=NULL && parent->getLeft()->getBlkName()==macro1->getBlkName())       side1 = 0;
    	    			else if(parent->getRight()!=NULL && parent->getRight()->getBlkName()==macro1->getBlkName()) side1 = 1;
    	    			else                                                            assert(0);
						done1 = 1;
   		     			break;
					}
				}
				assert(done1 == 1);
   		     	if(side1 == 0) parent->setLeft(NULL);
				else           parent->setRight(NULL);
				macro1->setPrev(NULL);
			
				bool done2 =0;
				count = 0;
				Macro* macro2 = NULL;
				bool side2 = 0;
				int choshole = rand() % (_macList.size()-3)+1;
				for(int i=0; i<_macList.size(); ++i){
					if(i!=chosidx && _macList[i]->getBlkName()!=parent->getBlkName()){
						if(_macList[i]->getLeft()==NULL) ++count;
						if(count == choshole){
							macro2 = _macList[i];
							side2 = 0;
							done2 = 1;
							break;
						}
						if(_macList[i]->getRight()==NULL) ++count;
						if(count == choshole){
							macro2 = _macList[i];
							side2 = 1;
							done2 = 1;
							break;
						}
					}
				}
				assert(done2 = 1);
				if(side2 == 0) macro2->setLeft(macro1);
				else           macro2->setRight(macro1);
				macro1->setPrev(macro2);     	
    	    	
    	 	   	cost2 = buildplan(alpha, x, h, w);
				clear();
				double prob2 = (double) rand() / (RAND_MAX + 1.0);
				if(cost1 < cost2 && prob2 >=exp((double)(cost1-cost2)/T)){
					if(side2==0) macro2->setLeft(NULL);
					else         macro2->setRight(NULL);
				
					if(side1==0) parent->setLeft(macro1);
					else         parent->setRight(macro1);
				
					macro1->setPrev(parent);
				}
				else{
				}
			}
			//0.66 <= prob < 1
			//OP3: To switch two blocks 
			else{
				int chosen1 = rand() % numBlk;
				int chosen2 = chosen1;
				while(chosen2 == chosen1){
					chosen2 = rand() % numBlk;
				}
				swap(_macList[chosen1], _macList[chosen2]);
				cost2 = buildplan(alpha, x, h, w);
				clear();
				double prob2 = (double) rand() / (RAND_MAX + 1.0);
				if(cost1 < cost2 && prob2 >=exp((double)(cost1-cost2)/T)){
					swap(_macList[chosen1], _macList[chosen2]);
				}
			}
	    }	
    }
    double cost = buildplan(alpha, outputX, outputH, outputW);
	return;
}

double Floorplanner::buildplan(double alpha, double& OutputX, double& OutputH, double &Wire){
	coordinate(_BTreeRoot, _BTreeLvRoot, 0);
	
	size_t xmax = 0;
	size_t ymax = 0;
	Range(xmax, ymax);
	OutputX = xmax, OutputH = ymax;
	double Area  = (double)xmax*ymax;
	double Ratio = (double)ymax/xmax;
	Wire  = Length();
	
	size_t xmore = 0;
	if(xmax-width>0) xmore = xmax-width;
	size_t ymore = 0;
	if(ymax-height>0) ymore = ymax-height;
	 
	return xmore*xmore+ymore*ymore*(1+alpha)/2+(Wire/10000000)*(1-alpha)/2;
}

void Floorplanner::coordinate(Macro* root, Level* lvroot, bool side){
	//First Block Case
	size_t nowx = nowX(root, lvroot, side);
	size_t nowy = nowY(root, lvroot, side, nowx);
	bool ftcase = 0;
	if(root->getBlkName()==""){
		ftcase = 1;
		root = root->getLeft();
	}
	
	root->setX(nowx);
	root->setY(nowy);
	Level* lvtmp = new Level( nowx, _name2Blk[root->getBlkName()]->getWidth(), nowy+_name2Blk[root->getBlkName()]->getHeight() );
	if(_name2Mac.count(root->getBlkName()) == 0) _name2Mac[root->getBlkName()] = root;
	
	//Check whether some Level in front of lvtmp
	Level* iter = lvroot;
	while(iter->getNext()!=NULL){
		iter = iter->getNext();
		if( (iter->getX()+iter->getL()) > (nowx+_name2Blk[root->getBlkName()]->getWidth()) ) break;
	}
	//Left Child Case
	if(lvtmp->getX()>lvroot->getX()){
		lvroot->setNext(lvtmp);
		lvtmp->setPrev(lvroot);		
	}
	//Right Child Case
	else if(lvtmp->getX()==lvroot->getX()){
		lvroot->setH(lvtmp->getH());
		lvroot->setL(lvtmp->getL());
		lvtmp = lvroot;
	}
	if( (iter->getX()+iter->getL()) > (nowx+_name2Blk[root->getBlkName()]->getWidth()) ){
		//Exist!
		lvtmp->setNext(iter);
		iter->setPrev(lvtmp);
		size_t end = iter->getX()+iter->getL();
		iter->setX(nowx+_name2Blk[root->getBlkName()]->getWidth());
		iter->setL( (iter->getX()+iter->getL())-(nowx+_name2Blk[root->getBlkName()]->getWidth())  );
	}

	
	if(root->getLeft()!=NULL)  coordinate(root->getLeft(), lvtmp, 0);
	if(root->getRight()!=NULL) coordinate(root->getRight(), lvtmp, 1);
	return;
}

size_t Floorplanner::nowX(Macro* root, Level* lvroot, bool side){
	if(root->getBlkName()=="") return 0;
	if(side == 0){
		assert(_name2Blk.count(root->getPrev()->getBlkName()) > 0);
		return ( root->getPrev()->getX() + _name2Blk[root->getPrev()->getBlkName()]->getWidth() );
	}
	else if(side == 1){
		return root->getPrev()->getX();
	}
	return -1;
}

size_t Floorplanner::nowY(Macro* root, Level* lvroot, bool side, size_t nowx){
 	if(root->getBlkName()=="") return 0;
	if(side == 0){
		size_t ans = 0;
		Level* iter = lvroot;
		while(iter->getNext()!=NULL && iter->getNext()->getX()<=(nowx+_name2Blk[root->getBlkName()]->getWidth()) ){
			iter = iter->getNext();
			if(iter->getX()>=nowx && iter->getH()>ans) ans = iter->getH();
		}
		return ans;
	}
	else if(side == 1){
		size_t ans = lvroot->getH();
		Level* iter = lvroot;
		while(iter->getNext()!=NULL && iter->getNext()->getX()<=(nowx+_name2Blk[root->getBlkName()]->getWidth()) ){
			iter = iter->getNext();
			if(iter->getH()>ans) ans = iter->getH();
		}
		return ans;
	}
	return -1;
}

void Floorplanner::Range(size_t& xmax, size_t& ymax){
	for(int i=0; i<numBlk; ++i){
		Macro* each;
		assert(_name2Mac.count(_blkList[i]->getName()) > 0);
		each = _name2Mac[_blkList[i]->getName()];
		size_t eachX = each->getX()+_blkList[i]->getWidth();
		size_t eachY = each->getY()+_blkList[i]->getHeight();
		if(eachX > xmax) xmax = eachX;
		if(eachY > ymax) ymax = eachY;
	}
	return;
}

double Floorplanner::Length(){
	double ans = 0;
	for(int i=0; i<numNet; ++i){
		vector<Terminal*> eachnet = _netList[i]->getTermList();
		double xmin, xmax, ymin, ymax;
		for(int j=0; j<eachnet.size(); ++j){
			double xmiddle, ymiddle;
			if(_name2Blk.count(eachnet[j]->getName())>0){
				Block* blk = _name2Blk[eachnet[j]->getName()];
				Macro* mac = _name2Mac[eachnet[j]->getName()];
				xmiddle = mac->getX() + ((double)blk->getWidth()/2);
				ymiddle = mac->getY()+ ((double)blk->getHeight()/2);
			}
			else{
				assert(_name2Tml.count(eachnet[j]->getName())>0);
				xmiddle = eachnet[j]->getX1();
				ymiddle = eachnet[j]->getY1();
				
			}
			if(j==0){
				xmin = xmiddle;	xmax = xmiddle;
				ymin = ymiddle; ymax = ymiddle;
			}
			else{
				if(xmiddle < xmin) xmin = xmiddle;
				if(ymiddle < ymin) ymin = ymiddle;
				if(xmiddle > xmax) xmax = xmiddle;
				if(ymiddle > ymax) ymax = ymiddle;
			}
		}
		double l = (xmax-xmin) + (ymax-ymin);
		ans += l;
	}
	return ans;
}

void Floorplanner::report(double alpha, fstream& output){
	output << (alpha*outputX*outputH+(1-alpha)*outputW) << endl;
	output << outputW << endl;
	output << outputX*outputH << endl;
	output << outputX << "  " << outputH << endl;
	output << (double)clock() / CLOCKS_PER_SEC << endl;
	for(int i=0; i<_macList.size(); ++i){
		output << _macList[i]->getBlkName() << "  " << _macList[i]->getX() << "  "<< _macList[i]->getY() << "  ";
		output << (_macList[i]->getX()+_name2Blk[_macList[i]->getBlkName()]->getWidth()) << "  ";
		output << (_macList[i]->getY()+_name2Blk[_macList[i]->getBlkName()]->getHeight()) << endl;
	}
	return;
	
}

void Floorplanner::clear(){
	Level* iter = _BTreeLvRoot;
	while(iter!=NULL && iter->getNext()!=NULL){
		iter = iter->getNext();
		delete iter->getPrev();
	} 
	_BTreeLvRoot = new Level(0, 0, 0);
	
	_name2Mac.clear();
	return;
}