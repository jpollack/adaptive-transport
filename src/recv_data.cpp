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
    while (!ustream.reof)
    {
	if (ustream.readable ())
	{
	    uint32_t bytes = ustream.readable ();
	    full_write (STDOUT_FILENO, ustream.rptr (), bytes);
	    ustream.rptrAdvance (bytes);
	}
	else
	{
	    std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    while (ustream.readable ())
    {
	uint32_t bytes = ustream.readable ();
	full_write (STDOUT_FILENO, ustream.rptr (), bytes);
	ustream.rptrAdvance (bytes);
    }


}

