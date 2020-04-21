#pragma once
#include <stdint.h>
#include <functional>
#include <vector>
#include "RollingWindow.hpp"
class RollingFilter
{
public:
    RollingFilter (const std::function<double(RollingWindow&, std::vector<double>&)> &);
    ~RollingFilter ();

    double operator () (double);
    operator double();
    int size (void) const;
    void size (int);
    
private:
    std::function<double(RollingWindow&, std::vector<double>&)> m_func;
    double m_val;
    std::vector<double> m_reg;
    RollingWindow m_wnd;
};
