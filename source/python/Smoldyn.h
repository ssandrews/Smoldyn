// =====================================================================================
//
//       Filename:  Smoldyn.h
//
//    Description: Header file.
//
//         Author:  Dilawar Singh (), dilawar.s.rajput@gmail.com
//   Organization:  NCBS Bangalore
//
// =====================================================================================

#ifndef SMOLDYN_H
#define SMOLDYN_H

#include <map>


#include "../source/pybind11/include/pybind11/pybind11.h"
namespace py = pybind11;

using namespace std;

class Smoldyn {

public:
    Smoldyn();
    ~Smoldyn();

    py::object getAttr(const string& name) const;

    void setAttr(const string& name, const py::object& val);

private:
    /* data */
    // map<string, value_t> attr_;
    map<string, py::object> attr_;
};

#endif /* end of include guard: SMOLDYN_H */
