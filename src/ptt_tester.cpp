#include <stdio.h>
#include "PTT.hpp"
#include <vector>
#include "TSReturn.hpp"
int main (int argc, char **argv)
{
    PTT buf;
    std::vector<SeqNum> seq;
    
    for (int ii=0; ii < 100; ++ii)
    {
	seq.emplace_back (buf.setSent ());
    }

    for (int ii=0; ii < 100; ii += 3)
    {
	buf.setRecv (seq[ii]);
    }

    while (buf.readable ())
    {
	const auto * el = buf.rptr ();
	printf ("%d\t%lu\t%lu\n", el->seq, el->tsSent, el->tsRecv);
	buf.readAdvance ();
    }
    std::vector<std::pair<uint32_t,uint64_t>> vec;
    std::string str = TSReturn::ToString (vec);
    for (int ii = 0; ii < str.size (); ++ii)
    {
	printf ("%02x ", str[ii]);
    }
    printf ("\n");
    return 0;
}
