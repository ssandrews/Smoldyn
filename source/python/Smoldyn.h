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

#include <sstream>
#include <map>
using namespace std;

#include "../source/pybind11/include/pybind11/pybind11.h"
namespace py = pybind11;

#include "SmoldynDefine.hh"


class Smoldyn {

public:
    Smoldyn();
    ~Smoldyn();

    SmoldynDefine& getDefine();

    size_t getDim() const;
    void setDim(size_t dim);

private:
    /* data */
    SmoldynDefine define_;
    size_t dim_;
};

#endif /* end of include guard: SMOLDYN_H */
