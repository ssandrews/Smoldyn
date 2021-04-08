/***
 *    Description:  Callback Functions support.
 *
 *        Created:  2020-07-26
 *         Author:  Dilawar Singh <dilawars@ncbs.res.in>
 *   Organization:  NCBS Bangalore
 *
 *        License:  GNU GPLv3
 */

#include "CallbackFunc.h"
#include <cmath>
#include <iostream>
#include <pybind11/embed.h>
#include <pybind11/eval.h>

namespace py = pybind11;

CallbackFunc::CallbackFunc()
  : val_(0.0)
  , funcName_("")
  , func_(py::none())
  , step_(1)
  , args_({})
{}

CallbackFunc::~CallbackFunc() {}

bool
CallbackFunc::evalAndUpdate(double t)
{
    if (!isValid()) {
        cerr << "Not a valid statement: '" << funcName_ << "'." << endl;
        return std::nan("invalid.");
    }

    py::module m = py::module::import("__main__");

    double v = func_ ? func_(t, args_).cast<double>()
                     : m.attr(funcName_.c_str())(t, args_).cast<double>();

    // If target_ is a function, call the function, else use the magic string.
    if (true == py::isinstance<py::function>(target_))
        target_(v);
    else
        py::exec(target_.cast<string>() + '=' + std::to_string(v),
                 m.attr("__dict__"));
    return true;
}

bool
CallbackFunc::isValid() const
{
    return (funcName_.size() > 0);
}

void
CallbackFunc::setFunc(const py::function& func)
{
    func_ = func;
    funcName_ = func_.attr("__name__").cast<string>();
}

py::function
CallbackFunc::getFunc() const
{
    return func_;
}

void
CallbackFunc::setFuncName(const string& fname)
{
    assert(fname.size() > 0);
    funcName_ = fname;
}

string
CallbackFunc::getFuncName() const
{
    return funcName_;
}

void
CallbackFunc::setStep(size_t step)
{
    step_ = step;
}

size_t
CallbackFunc::getStep() const
{
    return step_;
}

void
CallbackFunc::setTarget(const py::handle& target)
{
    target_ = target;
}

py::handle
CallbackFunc::getTarget() const
{
    return target_;
}

void
CallbackFunc::setArgs(const py::list& args)
{
    args_ = args;
}

py::list
CallbackFunc::getArgs() const
{
    return args_;
}
