// =====================================================================================
//
//
//    Description: 
//
//
//         Author:  Dilawar Singh (), dilawar.s.rajput@gmail.com
//   Organization:  NCBS Bangalore
//
// =====================================================================================

#include "Smoldyn.h"

Smoldyn::Smoldyn()
{}

Smoldyn::~Smoldyn()
{}

void Smoldyn::setAttr(const string& name, const py::object& val)
{
    attr_[name] = val;
}

py::object Smoldyn::getAttr(const string& name) const
{
    return attr_.at(name);
}
