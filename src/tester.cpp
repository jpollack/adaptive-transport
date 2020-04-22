#include <stdio.h>
#include "RollingWindow.hpp"
#include "RollingStats.hpp"
#include "Packet.hpp"
#include <cstring>
#include "Timing.hpp"
#include <thread>

char nib2char (uint8_t nib)
{
    return (nib < 0xA) ? (nib + '0') : (nib + 'A' - 0xA);
}

uint32_t byte2hex (uint8_t val)
{
    uint32_t ret = 0;
    char *buf = reinterpret_cast<char *>(&ret);
    buf[0] = nib2char ((val & 0xF0) >> 4);
    buf[1] = nib2char (val & 0x0F);
    buf[2] = ' ';
    return ret;
}

void hexPrint (const void *buf, size_t size)
{
    uint8_t *p = (uint8_t *)buf;
    while (size--)
    {
	uint32_t t = byte2hex (*p);
	printf ("%s", &t);
	p++;
	if (!((p - (uint8_t*)buf) % 64))
	{
	    printf ("\n");
	}
    }
    printf ("\n");
}

int main (int argc, char **argv)
{
    Packet p;
    printf ("size: %lu\n", sizeof(Packet));
    printf ("MTU: %lu\n", Packet::MTU);

    p.seq (1234);

    printf ("seq: %d\n", p.seq ());

    hexPrint (&p, 16);
    p.isAck (true);
    hexPrint (&p, 16);
    memcpy (p.dataPayload (), "Hello World", 12);
    hexPrint (&p, 16);
    printf ("%llx\n", p.tsRecvAck ());
    p.tsRecvAck (MicrosecondsSinceEpoch ());
    hexPrint (&p, 16);
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
	printf ("%d\t%c\t%lf\t%lf\t%lf\t%lf\n", ii, nib2char (ii), st.wmin (), st.mean (), st.variance (), st.wmax ());
    }

    uint64_t t0, t1, t2;

    t0 = MicrosecondsSinceEpoch ();
    uint64_t tot = 0;
    for (int ii = 0; ii < 1000000; ++ii)
    {
	uint32_t hid = std::hash<std::thread::id>{}(std::this_thread::get_id());
	tot += hid;
    }
    t1 = MicrosecondsSinceEpoch ();
    t2 = t1 - t0;
    printf ("%llu\t%llu\n", t2, tot);
    
    return 0;
}
