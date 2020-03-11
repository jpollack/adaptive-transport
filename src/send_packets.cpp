#include <stdio.h>
#include "UDPEndpoint.hpp"
#include "Timing.hpp"
#include <vector>
#include <iostream>

int main (int argc, char **argv)
{
    if (argc != 3)
    {
	fprintf (stderr, "Usage: %s HOST PORT\n", argv[0]);
	return -1;
    }

    uint64_t tStart = MicrosecondsSinceEpoch () + 1000;

    UDPEndpoint endpoint;
    Address peer (argv[1], argv[2]);

    // when	size	
    std::string line;
    std::future<uint64_t> fut;
    while (std::getline (std::cin, line))
    {
	// parse out timestamp (as difference from program start) and size
	size_t dpos = line.find ('\t');
	if (dpos == std::string::npos)
	{
	    fprintf (stderr, "Error: misformatted line\n");
	    exit (1);
	}
	uint64_t when = tStart + std::stoull (line.substr (0, dpos));
	int size = std::stoi (line.substr (dpos + 1));

	std::string payload (size, '*');
	fut = endpoint.queueSend (when, peer, payload);
    }

    // wait for last packet to be sent
    fut.get ();

    // report base time
    printf ("tStart\t%lu\n", tStart);
    
    return 0;
}
