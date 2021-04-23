/***
 *        Created:  2021-04-07

 *         Author:  Dilawar Singh <dilawar@subcom.tech>
 *    Description:  Command class.
 */

#ifndef COMMAND_H
#define COMMAND_H

#include <cassert>
#include <map>
#include <string>

#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "../Smoldyn/libsmoldyn.h"
#include "../Smoldyn/smoldyn.h"

const std::string __allowed_cmd_type__("@&aAbBeEnNiIjx");

using namespace std;

/*! \class Command
 *  \brief Create command.
 *
 */
class Command
{
  public:
    Command(const simptr sim, const string& cmd);

    Command(const simptr sim,
            const string& cmd,
            const char cmd_type,
            const map<string, double>& options);

    ~Command();

    ErrorCode addCommand();

    void addCommandToSimptr();
    bool isAddedToSimptr() const;

  private:
    const simptr sim_;
    const string cmd_;
    bool from_string_;

    char cmd_type_;
    map<string, double> options_;

    double on_;
    double off_;
    double step_;
    size_t multiplier_;
    bool added_;
};

#endif /* end of include guard: COMMAND_H */
