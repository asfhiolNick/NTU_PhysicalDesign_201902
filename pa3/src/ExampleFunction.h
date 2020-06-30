#define _GLIBCXX_USE_CXX11_ABI 0
#ifndef EXAMPLEFUNCTION_H
#define EXAMPLEFUNCTION_H

#include "NumericalOptimizerInterface.h"
#include "Placement.h" 

class ExampleFunction : public NumericalOptimizerInterface
{
public:
    ExampleFunction(Placement& placement);

    void evaluateFG(const vector<double> &x, double &f, vector<double> &g);
    void evaluateF(const vector<double> &x, double &f);
    unsigned dimension();
    
private:
	Placement& _placement;
	double     _r      = 0.1;
	double     _lambda = 100;
	double     _a      = 0.1;
	double     Mb;

};
#endif // EXAMPLEFUNCTION_H