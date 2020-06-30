#include "ExampleFunction.h"
#include <math.h>

// minimize 3*x^2 + 2*x*y + 2*y^2 + 7

ExampleFunction::ExampleFunction(Placement& placement)
	:_placement(placement)
{
	int areaM = 0;
	int numM  = _placement.numModules();
	for(int i=0; i<numM; ++i){
		areaM += _placement.module(i).area();
	}
	Mb = (double)areaM / ((_placement.boundryRight() - _placement.boundryLeft()) * (_placement.boundryTop() - _placement.boundryBottom()));
}

void ExampleFunction::evaluateFG(const vector<double> &x, double &f, vector<double> &g)
{
    //f = 3*x[0]*x[0] + 2*x[0]*x[1] + 2*x[1]*x[1] + 7; // objective function
    //g[0] = 6*x[0] + 2*x[1]; // gradient function of X
    //g[1] = 2*x[0] + 4*x[1]; // gradient function of Y
    
    double f1 = 0;
    double f2 = 0;
    //g[2*i]<->module[i]'s x-gradient, g[2*i+1]<->module[i]'s y-gradient
    int numg = g.size();
	for(int i=0; i<numg; ++i){
		g[i] = 0;
	}
    
    int binWidth  = (_placement.boundryRight() - _placement.boundryLeft())/16;
    int binHeight = (_placement.boundryTop() - _placement.boundryBottom())/16;
    //for bin[0]~bin[255], bin[i]=bin[i%16][i/16]
    for(int i=0; i<256; ++i){   	
    	int xindex = i%16, yindex = i/16;
    	int binLeft = _placement.boundryLeft() + xindex * binWidth;
    	int binBott = _placement.boundryBottom() + yindex * binHeight;
    	int xCenter = binLeft + binWidth/2;
    	int yCenter = binBott + binHeight/2;
    	
    	double Db = 0;
    	vector<double> dDb(2*_placement.numModules(), 0);
    	
    	int numM  = _placement.numModules();
    	for(int j=0; j<numM; ++j){
    		Module v  = _placement.module(j);
    		if((binLeft<x[2*j]+v.width())&&(x[2*j]<binLeft+binWidth)&&(binBott<x[2*j+1]+v.height())&&(x[2*j+1]<binBott+binHeight)){
    	    	double Fx = 0;
    	    	double Fy = 0;
    	
    			double t;
    			int	u = x[2*j], l = u + v.width(); 
				t = xCenter+v.width()/2+binWidth;
    			Fx += (double)1/_a/(1-exp(_a*(l-u)))*( log(exp(_a*(l-t))+1) - log(exp(-1*_a*u)+exp(-1*_a*t)) );
    		
				t = xCenter-v.width()/2-binWidth;
    			Fx -= (double)1/_a/(1-exp(_a*(l-u)))*( log(exp(_a*(l-t))+1) - log(exp(-1*_a*u)+exp(-1*_a*t)) );
    		
				u = x[2*j+1], l = u + v.height();
				t = yCenter+v.height()/2+binHeight;
				Fy += (double)1/_a/(1-exp(_a*(l-u)))*( log(exp(_a*(l-t))+1) - log(exp(-1*_a*u)+exp(-1*_a*t)) );
			
				t = yCenter-v.height()/2-binHeight;
				Fy -= (double)1/_a/(1-exp(_a*(l-u)))*( log(exp(_a*(l-t))+1) - log(exp(-1*_a*u)+exp(-1*_a*t)) );
			
				Db += (Fx * Fy) / (binWidth * binHeight);
			
				//Compute dDb[2*j]->g[2*j], dDb[2*j+1]->g[2*j+1] for module[j] in this case
				u = x[2*j], l = u + v.width();
				t = xCenter+v.width()/2+binWidth;
				int S1 = (double)1/(1-exp(_a*(l-u)))*(exp(_a*(l-t))/(1+exp(_a*(l-t))) + exp(-1*_a*u)/(exp(-1*_a*u)+exp(-1*_a*t)));
			
				t = xCenter-v.width()/2-binWidth;
				int S2 = (double)1/(1-exp(_a*(l-u)))*(exp(_a*(l-t))/(1+exp(_a*(l-t))) + exp(-1*_a*u)/(exp(-1*_a*u)+exp(-1*_a*t)));
				
				u = x[2*j+1], l = u + v.height();
				t = yCenter+v.height()/2+binHeight;
				int S3 = (double)1/(1-exp(_a*(l-u)))*(exp(_a*(l-t))/(1+exp(_a*(l-t))) + exp(-1*_a*u)/(exp(-1*_a*u)+exp(-1*_a*t)));
			
				t = yCenter-v.height()/2-binHeight;
				int S4 = (double)1/(1-exp(_a*(l-u)))*(exp(_a*(l-t))/(1+exp(_a*(l-t))) + exp(-1*_a*u)/(exp(-1*_a*u)+exp(-1*_a*t)));
				
				dDb[2*j] = (S1-S2)*Fy / (binWidth * binHeight);
				dDb[2*j+1] = Fx*(S3-S4) / (binWidth * binHeight);
		    }
    	}
    	f2 += (Db-Mb) * (Db-Mb);
    	for(int j=0; j<numM; ++j){
    		g[2*j]   += 2 * (Db-Mb) * dDb[2*j];
    		g[2*j+1] += 2 * (Db-Mb) * dDb[2*j+1];
    	}
    }
    f2 *= _lambda;
    int numMdl = _placement.numModules();
    for(int j=0; j<numMdl; ++j){
    	g[2*j]   *= _lambda;
    	g[2*j+1] *= _lambda;
    }
    f  = f1 + f2;
}

void ExampleFunction::evaluateF(const vector<double> &x, double &f)
{
	// objective function
    double f1 = 0;
    double f2 = 0;
    
    int binWidth  = (_placement.boundryRight() - _placement.boundryLeft())/16;
    int binHeight = (_placement.boundryTop() - _placement.boundryBottom())/16;
    //for bin[0]~bin[255], bin[i]=bin[i%16][i/16]
    for(int i=0; i<256; ++i){   	
    	int xindex = i%16, yindex = i/16;
    	int binLeft = _placement.boundryLeft() + xindex * binWidth;
    	int binBott = _placement.boundryBottom() + yindex * binHeight;
    	int xCenter = binLeft + binWidth/2;
    	int yCenter = binBott + binHeight/2;
    	
    	double Db = 0;

    	int numM  = _placement.numModules();
    	for(int j=0; j<numM; ++j){
    		Module v  = _placement.module(j);
    		if((binLeft<x[2*j]+v.width())&&(x[2*j]<binLeft+binWidth)&&(binBott<x[2*j+1]+v.height())&&(x[2*j+1]<binBott+binHeight)){
    	    	double Fx = 0;
    	    	double Fy = 0;
    	
    			double t;
    			int	u = x[2*j], l = u + v.width(); 
				t = xCenter+v.width()/2+binWidth;
    			Fx += (double)1/_a/(1-exp(_a*(l-u)))*( log(exp(_a*(l-t))+1) - log(exp(-1*_a*u)+exp(-1*_a*t)) );
    		
				t = xCenter-v.width()/2-binWidth;
    			Fx -= (double)1/_a/(1-exp(_a*(l-u)))*( log(exp(_a*(l-t))+1) - log(exp(-1*_a*u)+exp(-1*_a*t)) );
    		
				u = x[2*j+1], l = u + v.height();
				t = yCenter+v.height()/2+binHeight;
				Fy += (double)1/_a/(1-exp(_a*(l-u)))*( log(exp(_a*(l-t))+1) - log(exp(-1*_a*u)+exp(-1*_a*t)) );
			
				t = yCenter-v.height()/2-binHeight;
				Fy -= (double)1/_a/(1-exp(_a*(l-u)))*( log(exp(_a*(l-t))+1) - log(exp(-1*_a*u)+exp(-1*_a*t)) );
			
				Db += (Fx * Fy) / (binWidth * binHeight);
		    }
    	}
    	f2 += (Db-Mb) * (Db-Mb);
    	
    }
    f2 *= _lambda;
    f  = f1 + f2;
}

unsigned ExampleFunction::dimension()
{
	return 2*_placement.numModules(); // num_blocks*2 
    // each two dimension represent the X and Y dimensions of each block
}