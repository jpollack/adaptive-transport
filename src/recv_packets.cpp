#include <stdio.h>
#include "UDPEndpoint.hpp"

int main (int argc, char **argv)
{
    if (argc != 3)
    {
	fprintf (stderr, "Usage: %s HOST PORT\n", argv[0]);
	return -1;
    }

    UDPEndpoint endpoint;
    endpoint.setLocal (Address (argv[1], argv[2]));

    int psize = 0;
    while (true)
    {
    	const auto [ when, addr, header, payload ] = endpoint.queueRecv ();
	uint32_t seq = (header >> 32);
	uint32_t tsLow = (0x00000000FFFFFFFF & header);
    	printf ("%lu\t%u\t%u\t%u\n", when, seq, tsLow, payload.size ());
	if (payload.size () < psize)
	{
	    // packet size decreased, so should be last packet
	    break;
	}
	psize = payload.size ();
    }

    return 0;
}
