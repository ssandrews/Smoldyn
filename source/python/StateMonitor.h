/******************************************************************************
 * File:             StateMonitor.h
 *
 * Author:           StateMonitor class.
 * Created:          05/07/20
 * Description:
 *****************************************************************************/

#ifndef STATEMONITOR_H_HJXS4V2O
#define STATEMONITOR_H_HJXS4V2O

#include <map>
using namespace std;

class StateMonitor {
public:
    StateMonitor();
    ~StateMonitor();

    void addData(const string& name, double val);

private:
    /* data */
    map<string, vector<double>> data_;
};

#endif /* end of include guard: STATEMONITOR_H_HJXS4V2O */

