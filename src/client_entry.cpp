#include <stdio.h>
#include "UDPEndpoint.hpp"
#include "Timing.hpp"
#include <vector>

int main (int argc, char **argv)
{
    if (argc != 3)
    {
	fprintf (stderr, "Usage: %s HOST PORT\n", argv[0]);
	return -1;
    }

    int nPackets = 1000;

    std::vector<std::future<uint64_t>> fvec;
    fvec.reserve (nPackets);

    std::vector <uint64_t> tvec;
    tvec.reserve (nPackets);
    
    UDPEndpoint endpoint;
    endpoint.setPeer (Address (argv[1], argv[2]));

    std::this_thread::sleep_for (std::chrono::milliseconds (1000));
    endpoint.queueWrite (0, "ping");

    int period = 1234;
    uint64_t timeBase = MicrosecondsSinceEpoch () + 50000;
    for (int ii = 0; ii < nPackets; ++ii)
    {
	uint64_t when = timeBase + (ii * period);
	fvec.emplace_back (endpoint.queueWrite (when, "msg " + std::to_string (ii)));
    }
    
    for (auto& f : fvec)
    {
	tvec.emplace_back (f.get ());
    }

    printf ("idx\tt0\tt1\n");
    for (int ii = 0; ii < nPackets; ++ii)
    {
	uint64_t when = timeBase + (ii * period);
	printf ("%d\t%lu\t%lu\n", ii, when, tvec[ii]);
    }
    return 0;
}
