//    Description:
//
//
//         Author:  Dilawar Singh (), dilawar.s.rajput@gmail.com
//   Organization:  NCBS Bangalore
//

#ifndef SMOLDYNDEFINE_H
#define SMOLDYNDEFINE_H

/* --------------------------------------------------------------------------*/
/**
 * @Synopsis  This class stores `define` statement.
 */
/* ----------------------------------------------------------------------------*/
class SmoldynDefine {

public:
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
        for(const auto& v : defines_) {
            ss << "'" << v.first << "': " << py::str(v.second).cast<string>()
               << ", ";
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

#endif /* end of include guard: SMOLDYNDEFINE_H */
