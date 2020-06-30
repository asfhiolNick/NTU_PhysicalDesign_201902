#ifndef FLOORPLANNER_H
#define FLOORPLANNER_H

#include <vector>
#include <map>
#include <string>
#include "module.h"
using namespace std;

class Floorplanner
{
public:
    // constructor and destructor
    Floorplanner(fstream& input_blk, fstream& input_net){
    	parseInput_blk(input_blk);
    	parseInput_net(input_net);
	}
	~Floorplanner(){
		clear();
	}
    // basic access methods
    
    
    
    // modify method
    void parseInput_blk(fstream& input_blk);
    void parseInput_net(fstream& input_net);
    void floorplan(double);
    double buildplan(double, double&, double&, double&);
    void packing(Macro* root);
    void coordinate(Macro* root, Level* lvroot, bool side);
    size_t nowX(Macro* root, Level* lvroot, bool side);
    size_t nowY(Macro* root, Level* lvroot, bool side, size_t nowx);
    void   Range(size_t& xmax, size_t& ymax);
    double Length();
    void report(double, fstream&);
    
    // set functions
    

private:
	Macro*            _BTreeRoot = new Macro("");
	Level*			  _BTreeLvRoot = new Level(0, 0, 0);
	vector<Macro*>    _macList;
	vector<Block*>    _blkList;
	vector<Terminal*> _tmlList;
	vector<Net*>      _netList;
	map<string, Block*> _name2Blk;
	map<string, Terminal*> _name2Tml;
	map<string, Macro*> _name2Mac;
	int               width;
	int               height;
	int               numBlk;
	int               numTml;
	int               numNet;
	double            outputX;
	double            outputH;
	double            outputW;
	
	// Clean up partitioner
    void clear();
};

#endif  // FLOORPLANNER.H 