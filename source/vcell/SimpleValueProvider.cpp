#include "SimpleValueProvider.h"
#include <math.h>
#include <sstream>
using namespace std;

SimpleValueProvider::SimpleValueProvider(string& rateExp) {
	rateExpStr = rateExp;
	rate = getConstantValue();
}

double SimpleValueProvider::getConstantValue() {
	stringstream ss(rateExpStr);
	double v;
	ss >> v;
	if (ss.bad() || ss.fail()) {
		const char* errorMsg = "not a constant";
		throw errorMsg;
	}
	return v;
}

double SimpleValueProvider::getValue(double t, double x, double y, double z, rxnptr rxn) {
	//return sqrt(x*x+y*y+z*z) + t;
	//return rate;
	return getConstantValue();
}

double SimpleValueProvider::getValue(double t, double x, double y, double z, rxnptr rxn, char* panelName) {
	//return sqrt(x*x+y*y+z*z) + t;
	return rate /*getConstantValue()*/;
}

double SimpleValueProvider::getValue(double t, double x, double y, double z, surfactionptr actiondetails, char* panelName){
		//return sqrt(x*x+y*y+z*z) + t;
	return rate/*getConstantValue()*/;
}

ValueProvider* SimpleValueProviderFactory::createValueProvider(string& rateExp){
	return new SimpleValueProvider(rateExp);
}
