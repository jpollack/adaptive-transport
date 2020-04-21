#pragma once
#include <vector>

class RollingWindow
{
public:
    RollingWindow ();
    ~RollingWindow ();

    void operator ()(double);
    const double& operator[] (int);
    int size (void) const;
    void size (int);
    int count (void) const;
    
private:
    int m_head;
    int m_count;
    int m_size;
    std::vector<double> m_buf;
};
