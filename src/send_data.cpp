#include <stdio.h>
#include "UDPStream.hpp"
#include "fullio.hpp"
#include <unistd.h>
#include <deque>
#include "matplotlibcpp.h"
#include "Plot.hpp"
#include <cstdlib>
#include "ParamTracker.hpp"
#include <random>
#include "Timing.hpp"

namespace plt = matplotlibcpp;

int main (int argc, char **argv)
{
    if (argc != 4)
    {
	fprintf (stderr, "usage: %s host port bandwidth\n", argv[0]);
	return -1;
    }

    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution(0.0,1.0);

    size_t blocksize = 1024 * 128;
    uint8_t tbuf[blocksize];
    uint32_t nBytes;
    bool eof = false;
    std::deque<double> xq, yq;
    
    int windLen = 200;
    int wPer = 50;

    Plot po;
    ParamTracker pt;
    UDPStream ustream;
    Address peer (argv[1], argv[2]);
    ustream.setRemote (peer);
    ustream.bandwidth = std::atoi (argv[3]);
    
    // ustream.onDroppedFunc = [](uint32_t seq)
    // 				{
    // 				    fprintf (stderr, "Dropped %d\n", seq);
    // 				};
    uint64_t tsUpdated = MicrosecondsSinceEpoch () + 100000;
    FILE *fh = fopen ("metadata.tsv", "w");
    ustream.onPacketMetadata = [&](uint32_t seq, uint64_t tsSent, uint64_t tsRecv)
				   {
				       fprintf (fh, "%d\t%lu\t%lu\n", seq, tsSent, tsRecv);
				   };
	
    // ustream.onAckedFunc = [&](uint32_t seq, uint64_t tsSent, uint64_t tsRecv)
    // 			      {
    // 				  pt.update (seq, tsSent, tsRecv, 0);
    // 				  if (tsSent > tsUpdated)
    // 				  {
    // 				      double gbar = (double)pt.Q[0].g_owd_max_ma / 200.0;
    // 				      double pbackoff = 1.0 - exp (-1.0 * gbar);
    // 				      double x = distribution (generator);
    // 				      tsUpdated = MicrosecondsSinceEpoch ();
    // 				      bool didBackoff = false;
    // 				      if ((pt.Q[0].g_owd_min_ma > 0) && (pbackoff > x))
    // 				      {
    // 					  // backoff
    // 					  ustream.bandwidth = (double)ustream.bandwidth * 0.9;
    // 					  tsUpdated += 1000000;
    // 					  didBackoff = true;
    // 				      }
    // 				      else
    // 				      {
    // 					  ustream.bandwidth += (1024 * 8);				      
    // 				      }
    // 				      fprintf (stderr, "%s\t%d\t%d\t%d\t%d\n", (didBackoff ? "DOWN" : "UP" ), ustream.bandwidth, pt.Q[0].owd_min, pt.Q[0].g_owd_min_ma, pt.Q[0].g_owd_max_ma);
    // 				  }
				  
    // 				  if ( (pt.Q.size () > (windLen + wPer)) && ((pt.Q.size () % wPer) == 0))
    // 				  {
    // 				      /// copy to vectors
    // 				      std::vector<double> xv, yv, zv;
    // 				      for (const auto& p : pt.Q)
    // 				      {
    // 					  xv.push_back (p.tsSent);
    // 					  yv.push_back (p.g_owd_min_ma);
    // 					  zv.push_back (p.g_owd_max_ma);
    // 				      }
    // 				      po.plot (xv,yv,zv);
    // 				      for (int ii=0; ii <wPer; ++ii) { pt.Q.pop_back (); }
    // 				  }
    // 				  // fprintf (stderr, "%u\t%lu\t%lu\n", seq, tsSent, tsRecv);
    // 			      };
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
    // ustream.onAckedFunc = nullptr;
}
