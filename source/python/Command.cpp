/***
 *        Created:  2021-04-07

 *         Author:  Dilawar Singh <dilawar@subcom.tech>
 *    Description:  Command class.
 */

#include "Command.h"

#include "Simulation.h"

using namespace std;

Command::Command(const simptr sim, const string& cmd)
  : sim_(sim)
  , cmd_(cmd)
  , from_string_(true)
{
    cout << "[INFO] command is " << cmd << endl;

    options_ = map<string, double>();
    on_ = 0.0;
    off_ = 0.0;
    step_ = sim->dt;
    multiplier_ = 1;
    added_ = false;
}

Command::Command(const simptr sim,
                 const string& cmd,
                 char cmd_type,
                 map<string, double> options)
  : Command(sim, cmd)
{
    cmd_type_ = cmd_type;
    options_ = options;
    from_string_ = false;
}

Command::~Command() {}

void
Command::finalize()
{
    assert(__allowed_cmd_type__.find_first_of(cmd_type_) != string::npos);

    char* cmd = strdup(cmd_.c_str());

    if (from_string_) {
        auto k = smolAddCommandFromString(sim_, cmd);
        assert(k == ErrorCode::ECok);
        return;
    }

    if ('@' == cmd_type_)
        on_ = off_ = options_["time"];
    else if (string("aAbBeE").find(cmd_type_) != string::npos) {
    } else if (string("nN").find(cmd_type_) != string::npos)
        step_ = options_["step"];
    else if (string("ixjI").find(cmd_type_) != string::npos) {
        on_ = options_["on"];
        off_ = options_["off"];
        step_ = options_["step"];
        if ('x' == cmd_type_)
            multiplier_ = (size_t)options_["multiplier"]; //.cast<size_t>();
    } else
        throw py::value_error(("Command type " + cmd_type_) +
                              string(" is not supported"));
    if (addCommand() != ErrorCode::ECok)
        throw std::runtime_error("Failed to add command " + cmd_);
    added_ = true;
}

bool
Command::isFinalized() const
{
    return added_;
}

ErrorCode
Command::addCommand()
{
    std::cout << "Adding" << cmd_ << " " << cmd_type_
              << "on, off, step, multiplier" << on_ << off_ << step_
              << multiplier_ << endl;
    return smolAddCommand(sim_, cmd_type_, on_, off_, step_, multiplier_, cmd_.c_str());
}

