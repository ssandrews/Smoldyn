/***
 *    Description:  Callback functions.
 *        Created:  2020-07-26
 *         Author:  Dilawar Singh <dilawars@ncbs.res.in>
 *   Organization:  NCBS Bangalore
 */

#ifndef CALLBACKFUNC_H_XGBEYLQ3
#define CALLBACKFUNC_H_XGBEYLQ3

#include <array>
#include <functional>
#include <string>

#include <pybind11/pybind11.h>
namespace py = pybind11;

using namespace std;

class CallbackFunc
{
  public:
    CallbackFunc();
    ~CallbackFunc();

    void setFuncName(const string& runstr);
    std::string getFuncName() const;

    void setStep(size_t step);
    size_t getStep() const;

    void setTarget(const py::handle& target);
    py::handle getTarget() const;

    void setFunc(const py::function& target);
    py::function getFunc() const;

    bool isValid() const;
    bool evalAndUpdate(double t);

    void setArgs(const py::list& args);
    py::list getArgs() const;

  private:
    /* data */
    double val_;
    std::string funcName_;
    py::function func_;
    size_t step_;
    py::handle target_;
    py::list args_;
};

#endif /* end of include guard: CALLBACKFUNC_H_XGBEYLQ3 */
