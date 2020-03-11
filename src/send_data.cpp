#include <stdio.h>
#include "UDPStream.hpp"
#include "fullio.hpp"
#include <unistd.h>
#include <deque>
#include "matplotlibcpp.h"
#include "Plot.hpp"
#include <cstdlib>

namespace plt = matplotlibcpp;

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
    std::deque<double> xq, yq;
    
    int windLen = 200;
    int wPer = 50;

    Plot po;

    UDPStream ustream;
    Address peer (argv[1], argv[2]);
    ustream.setRemote (peer);
    ustream.bandwidth = std::atoi (argv[3]);
    
    ustream.onDroppedFunc = [](uint32_t seq)
				{
				    fprintf (stderr, "Dropped %d\n", seq);
				};
    ustream.onAckedFunc = [&](uint32_t seq, uint64_t tsSent, uint64_t tsRecv)
			      {
				  xq.push_back (tsSent);
				  yq.push_back (tsRecv - tsSent);

				  if ( (xq.size () > (windLen + wPer)) && ((xq.size () % wPer) == 0))
				  {
				      /// copy to vectors
				      std::vector<double> xv (xq.begin (), xq.end ());
				      std::vector<double> yv (yq.begin (), yq.end ());
				      po.plot (xv,yv);
				      for (int ii=0; ii <wPer; ++ii) { xq.pop_front (); yq.pop_front (); }
				  }
				  // fprintf (stderr, "%u\t%lu\t%lu\n", seq, tsSent, tsRecv);
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
    ustream.onAckedFunc = nullptr;
}
