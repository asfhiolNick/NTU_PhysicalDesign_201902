#include <iostream>
#include <fstream>
#include <vector>
#include <assert.h>
#include <math.h>
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
	_name2Mac[root->getBlkName()] = root;
	
	//bool tall = (height>=width);
	for(int i=0; i<numBlk; ++i){
		string name = _blkList[i]->getName();
		_macList.push_back(new Macro(name));
		if(_name2Mac.count(name) == 0) _name2Mac[name] = _macList[i];
		
		//if(tall && _blkList[i]->getWidth()>_blkList[i]->getHeight()) _blkList[i]->setRotate();
		//else if(!tall && _blkList[i]->getHeight()>_blkList[i]->getWidth()) _blkList[i]->setRotate();
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
		int iteration;
		if(T>100)		iteration = 500;
		else if(T>30)	iteration = 1000;
		else			iteration = 500;

		for(int i=0; i<iteration; ++i){
			//Compute cost(S) = cost1
			double X, H, W; 
			double cost1 = buildplan(alpha, X, H, W);
			if(cost1 <= 0){
				buildplan(alpha, outputX, outputH, outputW);
				return;
			}
			clear();

			//Compute cost(S') = cost2
			int prob = rand() % 3;
			//Useless variable
			double x, h, w;
			double cost2;
			switch(prob){
				//prob < 0.33
				//OP1: To rotate one block
				case 0:{
					int chosen = rand() % numBlk;
					_blkList[chosen]->setRotate();
					cost2 = buildplan(alpha,x ,h, w);
					clear();
					double prob2 = (double) rand() / (RAND_MAX + 1.0);
					if(cost1 < cost2 && prob2 >= exp((double)(cost1-cost2)/T)){
						_blkList[chosen]->setRotate();
					}
					break;
				}
				//0.33 <= prob < 0.66
				//OP2: To remove chosen leaf and insert into hole
				case 1:{
    	    		//To count #leaf and choose which one
					int count = 0;
    	    		for(int i=0; i<_macList.size(); ++i){ 
						if(_macList[i]->getLeft()==NULL && _macList[i]->getRight()==NULL) ++count;
					}
					int chosnum = (rand() % count) +1;

					//To visit the chosen leaf and remove it!
					int chosleaf;
					bool sideleaf;
					count = 0;
					for(int i=0; i<_macList.size(); ++i){
						if(_macList[i]->getLeft()==NULL && _macList[i]->getRight()==NULL) ++count;
						if(count == chosnum){
							chosleaf = i;
							break;
						}
					}
					Macro* macro1 = _macList[chosleaf];
					Macro* parent = _macList[chosleaf]->getPrev();
					if(parent->getLeft()!=NULL && parent->getLeft()->getBlkName()==macro1->getBlkName())		sideleaf = 0;
					else if(parent->getRight()!=NULL && parent->getRight()->getBlkName()==macro1->getBlkName())	sideleaf = 1; 
					else assert(0);

					(sideleaf == 0)? parent->setLeft(NULL) : parent->setRight(NULL);
					macro1->setPrev(NULL);
			
					//To count #hole and choose which one
					count = 0;
					for(int i=0; i<_macList.size(); ++i){
						if(i == chosleaf) continue;
						if(_macList[i]->getLeft()==NULL)	++count;
						if(_macList[i]->getRight()==NULL) 	++count;
					}
					chosnum = (rand() % count)+1;

					//To visit the chosen hole and fillin it!
					int choshole;
					bool sidehole;
					count = 0;
					for(int i=0; i<_macList.size(); ++i){
						if(i == chosleaf) continue;
						if(_macList[i]->getLeft()==NULL)	++count;
						if(count == chosnum){
							choshole = i;
							sidehole = 0;
							break;
						}
						if(_macList[i]->getRight()==NULL) 	++count;
						if(count == chosnum){
							choshole = i;
							sidehole = 1;
							break;
						}					
					}
					Macro* macro2 = _macList[choshole];
					(sidehole == 0)? macro2->setLeft(macro1) : macro2->setRight(macro1);		  
					macro1->setPrev(macro2);    	
    	    	
    	 	   		cost2 = buildplan(alpha, x, h, w);
					clear();
					double prob2 = (double) rand() / (RAND_MAX + 1.0);
					if(cost1 < cost2 && prob2 >=exp((double)(cost1-cost2)/T)){
						(sidehole == 0)? macro2->setLeft(NULL) : macro2->setRight(NULL);		
						(sideleaf == 0)? parent->setLeft(macro1) : parent->setRight(macro1);      		
						macro1->setPrev(parent);
					}
					break;
				}
				//0.66 <= prob < 1
				//OP3: To switch two blocks 
				case 2:{
					int chosen1 = rand() % numBlk;
					int chosen2 = chosen1;
					while(chosen2 == chosen1){
						chosen2 = rand() % numBlk;
						if(_macList[chosen1]->getPrev()!=NULL&&_macList[chosen1]->getPrev()->getBlkName()==_macList[chosen2]->getBlkName()) chosen2 = chosen1;
						if(_macList[chosen1]->getLeft()!=NULL&&_macList[chosen1]->getLeft()->getBlkName()==_macList[chosen2]->getBlkName()) chosen2 = chosen1;
						if(_macList[chosen1]->getRight()!=NULL&&_macList[chosen1]->getRight()->getBlkName()==_macList[chosen2]->getBlkName()) chosen2 = chosen1;
					} 
					assert(chosen1 != chosen2);
					swaprelation(chosen1, chosen2);
					
					cost2 = buildplan(alpha, x, h, w);
					clear();
					double prob2 = (double) rand() / (RAND_MAX + 1.0);
					if(cost1 < cost2 && prob2 >=exp((double)(cost1-cost2)/T)){
						swaprelation(chosen1, chosen2);
					}
					break;
				}
			}
	    }	
    }
    double cost = buildplan(alpha, outputX, outputH, outputW);
	return;
}

void Floorplanner::swaprelation(int chosen1, int chosen2){
	Macro* chos1	= _macList[chosen1];
	Macro* parent1	= chos1->getPrev();
	Macro* left1	= chos1->getLeft();
	Macro* right1	= chos1->getRight();
	bool side1;
	if(parent1->getLeft()!=NULL && parent1->getLeft()->getBlkName()==chos1->getBlkName())			side1 = 0;
	else if(parent1->getRight()!=NULL && parent1->getRight()->getBlkName()==chos1->getBlkName()) 	side1 = 1;
	else assert(0);
	/*
	cout << "chos1Name = "<< chos1->getBlkName();
	cout << " parent1Name = " << parent1->getBlkName();
	if(left1!=NULL)cout << " left1Name = " << left1->getBlkName() << " ";
	if(right1!=NULL)cout << " right1Name = " << right1->getBlkName() << " ";
	cout << side1 << endl;
	*/
	Macro* chos2 = _macList[chosen2];
	Macro* parent2 = chos2->getPrev();
	Macro* left2   = chos2->getLeft();
	Macro* right2  = chos2->getRight();
	bool side2;
	if(parent2->getLeft()!=NULL && parent2->getLeft()->getBlkName()==chos2->getBlkName())			side2 = 0;
	else if(parent2->getRight()!=NULL && parent2->getRight()->getBlkName()==chos2->getBlkName())	side2 = 1;
	else assert(0);
	/*
	cout << "chos2Name = "<< chos2->getBlkName();
	cout << " parent2Name = " << parent2->getBlkName();
	if(left2!=NULL)cout << " left2Name = " << left2->getBlkName() << " ";
	if(right2!=NULL)cout << " right2Name = " << right2->getBlkName() << " ";
	cout << side2 << endl;
	*/
	contrelation(chos2, parent1, left1, right1, side1);
	contrelation(chos1, parent2, left2, right2, side2);
}

void Floorplanner::contrelation(Macro* chos, Macro* parent, Macro* left, Macro* right, bool side){
	if(parent!=NULL) (side==0) ? parent->setLeft(chos) : parent->setRight(chos);
	chos->setPrev(parent);
	if(left!=NULL)	left->setPrev(chos);
	chos->setLeft(left);
	if(right!=NULL)right->setPrev(chos);
	chos->setRight(right);
}

double Floorplanner::buildplan(double alpha, double& OutputX, double& OutputH, double &Wire){
	coordinate(_BTreeRoot, _BTreeLvRoot);
	size_t xmax = 0;
	size_t ymax = 0;
	Range(xmax, ymax);
	OutputX = xmax, OutputH = ymax;
	double Area  = (double)xmax*ymax;
	double Ratio = (double)ymax/xmax;
	Wire  = Length();

	double xmore = max((int)xmax-(int)width, 0);
	double ymore = max((int)ymax-(int)height, 0);
	return xmore+ymore;
}

//Assume nowlev's height is old, update it!
void Floorplanner::coordinate(Macro* nowmac, Level* nowlev){
	if(nowmac==NULL)
		return;	
	if(nowmac->getBlkName()=="HEADMACRO"){
		assert(_name2Blk.count(nowmac->getLeft()->getBlkName())>0);
		coordinate(nowmac->getLeft(), new Level(0, 0));
		return;
	}
	
	assert(_name2Blk.count(nowmac->getBlkName())>0);
	Level* updatelev= nowlev, * updatelevPrev;
	size_t now_x_beg = nowlev->getX(), now_x_end = nowlev->getX() + _name2Blk[nowmac->getBlkName()]->getWidth();
	size_t max_y, last_y;
	while(updatelev!=NULL){
		size_t current_x = updatelev->getX();

		if(current_x == now_x_beg){
			max_y = updatelev->getH();
			last_y = updatelev->getH();
		}
		else if(current_x < now_x_end){
			max_y = max(updatelev->getH(), max_y);
			last_y = updatelev->getH();
		}
		else if(current_x >= now_x_end){
			break;
		}
		else
			assert(0);
		updatelevPrev = updatelev;
		updatelev = updatelev->getNext();
	}
	nowlev->setH(max_y+_name2Blk[nowmac->getBlkName()]->getHeight());
	nowmac->setX(now_x_beg), nowmac->setY(max_y);
	
	/*Level* iter = nowlev, *iternext;
	while( iter && (!updatelev&&(iter->getX()!=updatelevPrev->getX()) || updatelev&&(iter->getX()!=updatelev->getX())) ){
		iternext = iter->getNext();
		delete iter;
		iter = iternext;
	}*/

	if(updatelev!=NULL&&updatelev->getX()==now_x_end){
		nowlev->setNext(updatelev);
		updatelev->setPrev(updatelev);
		coordinate(nowmac->getLeft(), updatelev);
	}
	else{
		Level* middle = new Level(now_x_end, last_y);
		nowlev->setNext(middle);
		middle->setPrev(nowlev);
		if(updatelev!=NULL){
			middle->setNext(updatelev);
			updatelev->setPrev(middle);
		}
		coordinate(nowmac->getLeft(), middle);
	}
	coordinate(nowmac->getRight(), nowlev);
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
	_BTreeLvRoot = new Level(0, 0);
	return;
}