/***
 *    Description:  Species in smoldyn.
 *
 *        Created:  2020-04-21

 *         Author:  Dilawar Singh <dilawars@ncbs.res.in>
 *   Organization:  NCBS Bangalore
 *        License:  MIT License
 */

#include <string>
using namespace std;

#include "SmoldynSpecies.h"

SmoldynSpecies::SmoldynSpecies(const string& name)
    : name_(name), diffConst_(0.0)
{
}

SmoldynSpecies::~SmoldynSpecies()
{
}

void SmoldynSpecies::setDiffConst(const double D)
{
    diffConst_ = D;
}

double SmoldynSpecies::getDiffConst(void) const
{
    return diffConst_;
}

double SmoldynSpecies::getDisplaySize() const
{
    return displaySize_;
} 

void SmoldynSpecies::setDisplaySize(const double value) 
{
    displaySize_ = value;
    return;
}

string SmoldynSpecies::getColor() const
{
    return color_;
}

void SmoldynSpecies::setColor(const string& value)
{
    color_ = value;
    return;
} 

string SmoldynSpecies::getName() const
{
    return name_;
}
