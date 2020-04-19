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
    if (argc != 4)
    {
	fprintf (stderr, "usage: %s host port bandwidth\n", argv[0]);
	return -1;
    }

    size_t blocksize = 1024 * 128;
    uint8_t tbuf[blocksize];
    uint32_t nBytes;
    bool eof = false;
    
    UDPStream ustream;
    ustream.setRemote (Address (argv[1], argv[2]));
    ustream.bandwidth = std::atoi (argv[3]);
    
    FILE *fh = fopen ("metadata.tsv", "w");
    ustream.onPacketMetadata = [&](uint32_t seq, uint64_t tsSent, uint64_t tsRecv)
				   {
				       fprintf (fh, "%d\t%lu\t%lu\n", seq, tsSent, tsRecv);
				   };
    while (!eof)
    {
	while (ustream.bytesQueued ())
	{
	    std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	
	std::tie (eof, nBytes) = full_read (STDIN_FILENO, tbuf, blocksize);
	ustream.send ((eof ? "E" : "D") + std::string ((const char*)&nBytes, 4) + std::string (tbuf, tbuf+nBytes));
    } while (!eof);
    while (ustream.bytesQueued ())
    {
    	std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    fclose (fh);

}
