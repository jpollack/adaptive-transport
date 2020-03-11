#include <stdio.h>
#include "matplotlibcpp.h"
#include <thread>
#include <chrono>

namespace plt = matplotlibcpp;

int main()
{
    int n = 1000;
    std::vector<double> x (n), y (n), z (n), w (n, 2), xx (n);
	
    for(int i=0; i<n; i++) {
	x.at (i) = i * i;
	y.at (i) = sin(2*M_PI*i/360.0);
	z.at (i) = log(i);
	xx.at (i) = i;
    }
    // plt::figure ();
    plt::plot (x, y);
    plt::show ();
    std::this_thread::sleep_for (std::chrono::milliseconds (1000));
    return 0;
}

// int main (int argc, char **argv)
// {
//     plt::plot ({1,3,2,4});
//     plt::show ();
//     plt::pause (0.01);
//     std::this_thread::sleep_for (std::chrono::milliseconds (5));
    
//     plt::show ();
//     std::this_thread::sleep_for (std::chrono::milliseconds (5000));
//     return 0;
// }
