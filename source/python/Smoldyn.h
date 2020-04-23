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


#include "pybind11/pybind11.h"
namespace py = pybind11;

using namespace std;

struct SmoldynDefine {

    py::object getDefine(const string& name) const
    {
        return defines_.at(name);
    }

    void setDefine(const string& name, const py::object& val)
    {
        defines_[name] = val;
    }

    string __repr__()
    {
        stringstream ss("{");
        for(const auto & v : defines_ ) {
            ss << "'" << v.first << "': " << py::str(v.second).cast<string>() << ", ";
        }
        string sofar = ss.str();
        if(sofar.size() > 1) {
            // Remove trailing " ," 
            sofar.pop_back();
            sofar.pop_back();
        }

        return sofar + '}';
    }

    /* data */
    map<string, py::object> defines_;

};

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
