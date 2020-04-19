#include <stdio.h>
#include "UDPStream.hpp"
#include "fullio.hpp"

int main (int argc, char **argv)
{
    if (argc != 3)
    {
	fprintf (stderr, "Usage: %s HOST PORT\n", argv[0]);
	return -1;
    }

    UDPStream ustream;
    ustream.setLocal (Address (argv[1], argv[2]));
    bool eof = false;
    while (!eof)
    {
	while (ustream.readable () == 0)
	{
	    std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	const uint8_t *base = ustream.rptr ();
	uint32_t size = *(uint32_t *)base;
	eof = (size >> 31);
	size &= ~(1 << 31);
	ustream.rptrAdvance (4);
	while (size > ustream.readable ())
	{
	    std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	full_write (STDOUT_FILENO, ustream.rptr (), size);
	ustream.rptrAdvance (size);
    }

}

