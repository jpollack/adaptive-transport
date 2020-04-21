#include <stdio.h>
#include "RollingWindow.hpp"
#include "RollingStats.hpp"

int main (int argc, char **argv)
{
    RollingWindow wnd;
    wnd.size (9);

    wnd (15.5);
    printf ("%lf\n", wnd[0]);

    wnd (16.5);
    printf ("%lf\n", wnd[0]);

    wnd (17.5);
    printf ("%lf\n", wnd[0]);

    printf ("%lf\n", wnd[1]);
    printf ("%lf\n", wnd[2]);

    // RollingFilter ma ([](RollingWindow& wnd, std::vector<double>& reg)
    // 			  {
    // 			      reg[0] += wnd[0];
    // 			      reg[0] -= wnd[-1];
    // 			      return reg[0] / (double) wnd.size ();
    // 			  });

    // ma.size (8);

    RollingStats st;
    st.size (4);
    
    for (int ii = 1; ii < 16; ii++)
    {
	st (ii);
	printf ("%d\t%lf\t%lf\t%lf\n", ii, st.wmin (), st.mean (), st.wmax ());
    }

    return 0;
}
