#include <stdio.h>
#include "UDPStream.hpp"
#include "fullio.hpp"
#include <unistd.h>
#include <deque>
#include <cstdlib>
#include <random>
#include "Timing.hpp"

// namespace plt = matplotlibcpp;

int main (int argc, char **argv)
{
    if (argc != 5)
    {
	fprintf (stderr, "usage: %s host port bandwidth updateBandwidth\n", argv[0]);
	return -1;
    }

    size_t blocksize = 1024 * 128;
    uint8_t tbuf[blocksize];
    uint32_t nBytes;
    bool eof = false;
    
    UDPStream ustream;
    ustream.setRemote (Address (argv[1], argv[2]));
    ustream.bandwidth = std::atoi (argv[3]);
    ustream.updateBandwidth = std::atoi (argv[4]);
    FILE *fh = fopen ("metadata.tsv", "w");
    ustream.onPacketMetadata = [&](uint32_t seq, uint64_t tsSent, uint64_t tsRecv, uint64_t tsAck)
				   {
				       fprintf (fh, "%d\t%lu\t%lu\t%lu\n", seq, tsSent, tsRecv, tsAck);
				   };
    while (!eof)
    {
	while (ustream.bytesQueued ())
	{
	    std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	
	std::tie (eof, nBytes) = full_read (STDIN_FILENO, tbuf, blocksize);
	// use the upper bit of nBytes to encode "is this EOF or not", to tell
	// to the receiver, so it can gracefully shut down after receiving all
	// packets.  This is useful for automated testing.  Otherwize, there's
	// no need to send that information.
	uint32_t header = nBytes | (eof << 31);
	ustream.send (std::string ((const char*)&header, 4) + std::string (tbuf, tbuf+nBytes));
    } while (!eof);

    while (ustream.bytesQueued ())
    {
    	std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    fclose (fh);

}
