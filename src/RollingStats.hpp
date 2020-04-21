#pragma once
#include <array>
#include <set>
#include <vector>

class RollingStats
{
public:
    RollingStats ();
    ~RollingStats ();

    void operator ()(double);
    const double& operator[] (int);
    int size (void) const;
    void size (int);
    int count (void) const;
    bool populated (void) const;
    double mean () const;
    double stdev () const;
    double variance () const;
    double wmin () const;
    double wmax () const;
    double min () const;
    double max () const;
    
private:
    double& at (int);
    void update (void);
    int m_head;
    int m_count;
    int m_size;
    double m_total;
    double m_mean;
    double m_variance;
    double m_max;
    double m_min;
    std::vector<double> m_buf;
    std::multiset<double> m_set;
};
