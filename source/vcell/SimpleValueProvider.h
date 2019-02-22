#ifndef SIMPLE_VALUE_PROVIDER_H
#define SIMPLE_VALUE_PROVIDER_H

#include <smoldyn.h>

class SimpleValueProvider : public ValueProvider {
public:
	SimpleValueProvider(string& rateExp);
	double getConstantValue();
	double getValue(double t, double x, double y, double z, rxnptr rxn);
	double getValue(double t, double x, double y, double z, rxnptr rxn, char* panelName);
	double getValue(double t, double x, double y, double z, surfactionptr actiondetails, char* panelName);
private:
	string rateExpStr;
	double rate;
};

class SimpleValueProviderFactory : public ValueProviderFactory{
public:
	ValueProvider* createValueProvider(string& rateExp);
};
#endif
