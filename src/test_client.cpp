#include <stdio.h>
#include "UDPStream.hpp"
#include "fullio.hpp"

int main (int argc, char **argv)
{
    if (argc != 6)
    {
	fprintf (stderr, "Usage: %s host port bandwidth updateBandwidth testSize\n", argv[0]);
	return -1;
    }

    UDPStream ustream;
    ustream.setRemote (Address (argv[1], argv[2]));
    uint32_t bandwidth = std::atoi (argv[3]);
    char updateBandwidth = (bool)std::atoi (argv[4]);
    int32_t testSize = std::atoi (argv[5]);
    
    ustream.send (std::string ((const char *) &testSize, 4)
		  + std::string ((const char *)&bandwidth, 4)
		  + std::string (&updateBandwidth, 1));
    
    uint32_t bytesRemaining = testSize;
    while (bytesRemaining)
    {
	while (ustream.readable () == 0)
	{
	    std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	uint32_t nBytes = ustream.readable ();
	bytesRemaining -= nBytes;
	// throw away the data.  to use it, would access ustream.rptr();
	// printf ("Got %d bytes.\t%d bytes remain.\n", nBytes, bytesRemaining);
	ustream.rptrAdvance (nBytes);
    }

}

