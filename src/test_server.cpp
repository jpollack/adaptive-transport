#include <stdio.h>
#include "UDPStream.hpp"
#include "fullio.hpp"
#include <unistd.h>
#include <deque>
#include <cstdlib>
#include <random>
#include "Timing.hpp"
#include "wyhash.h"
// namespace plt = matplotlibcpp;

void populateRandom (void *in, uint32_t bytes);

int main (int argc, char **argv)
{
    if (argc != 3)
    {
	fprintf (stderr, "usage: %s host port\n", argv[0]);
	return -1;
    }

    size_t blocksize = 1024 * 128;
    uint8_t tbuf[blocksize];
    
    UDPStream ustream;
    ustream.setLocal (Address (argv[1], argv[2]));

    // ustream.bandwidth = std::atoi (argv[3]);
    // ustream.updateBandwidth = std::atoi (argv[4]);


    while (true)
    {
	// alternate between listening for a new "connection", and sending the
	// number of requested bytes.

	while (ustream.readable () < 9)
	{
	    std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	int32_t testSize = *(int32_t *)ustream.rptr ();
	ustream.rptrAdvance (4);

	uint32_t bandwidth = *(uint32_t *)ustream.rptr ();
	ustream.rptrAdvance (4);
	
	char updateBandwidth = *(char *)ustream.rptr ();
	ustream.rptrAdvance (1);

	ustream.bandwidth = bandwidth;
	ustream.updateBandwidth = updateBandwidth;
	
	FILE *fh = fopen ("metadata.tsv", "w");
	ustream.onPacketMetadata = [&](uint32_t seq, uint64_t tsSent, uint64_t tsRecv)
				       {
					   fprintf (fh, "%d\t%lu\t%lu\n", seq, tsSent, tsRecv);
				       }; 
	printf ("Starting testSize = %d, bandwidth = %d, updateBandwidth = %d\n", testSize, bandwidth, updateBandwidth);
	while (testSize)
	{
	    uint32_t nBytes = (testSize < blocksize) ? testSize : blocksize;
	    populateRandom (tbuf, nBytes);
	    while(ustream.bytesQueued ())
	    {
		std::this_thread::yield ();
	    }
	    ustream.send (std::string (tbuf, tbuf+nBytes));
	    testSize -= nBytes;
	}

	while(ustream.bytesQueued ())
	{
	    std::this_thread::yield ();
	}

	fclose (fh);
	printf ("Test finished.\n");
    }
    
}

void populateRandom (void *in, uint32_t bytes)
{
    
}
