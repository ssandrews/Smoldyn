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
  , on_(0.0)
  , off_(0.0)
  , step_(0.0)
  , multiplier_(1)
  , added_(false)
{}

Command::Command(const simptr sim,
                 const string& cmd,
                 char cmd_type,
                 const map<string, double>& options)
  : Command(sim, cmd)
{
    cmd_type_ = cmd_type;
    options_ = options;
    from_string_ = false;
}

Command::~Command() {}

void
Command::addCommandToSimptr()
{
    if (__allowed_cmd_type__.find_first_of(cmd_type_) == string::npos) {
        cerr << "Command type '" << cmd_type_ << "' is not supported." << endl;
        throw std::runtime_error("invalid command type");
    }

    char* cmd = strdup(cmd_.c_str());

    if (from_string_) {
        auto k = smolAddCommandFromString(sim_, cmd);
        if (k != ErrorCode::ECok)
            throw std::runtime_error("Failed to add command " + cmd_);
        added_ = true;
        return;
    }

    if ('@' == cmd_type_)
        on_ = off_ = options_["time"];
    else if (string("aAbBeE").find(cmd_type_) != string::npos) {
    } else if (string("nN").find(cmd_type_) != string::npos)
        step_ = options_.at("step");
    else if (string("ixjI").find(cmd_type_) != string::npos) {
        on_ = options_.at("on");
        off_ = options_.at("off");
        step_ = options_.at("step");
        if ('x' == cmd_type_)
            multiplier_ = (size_t)options_.at("multiplier");
    } else
        throw py::value_error(("Command type " + cmd_type_) +
                              string(" is not supported"));
    if (addCommand() != ErrorCode::ECok)
        throw std::runtime_error("Failed to add command " + cmd_);
    added_ = true;
}

bool
Command::isAddedToSimptr() const
{
    return added_;
}

ErrorCode
Command::addCommand()
{
#if 0
    std::cout << "111: Adding " << cmd_ << " " << cmd_type_ << "on=" << on_
              << " off=" << off_ << " step=" << step_
              << " multiplier=" << multiplier_ << endl;
#endif
    return smolAddCommand(
      sim_, cmd_type_, on_, off_, step_, multiplier_, cmd_.c_str());
}

