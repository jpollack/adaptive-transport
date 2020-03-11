#include <stdio.h>
#include "UDPEndpoint.hpp"

int main (int argc, char **argv)
{
    if (argc != 2)
    {
	fprintf (stderr, "Usage: %s PORT\n", argv[0]);
	return -1;
    }

    UDPEndpoint endpoint;
    Address a0 ("127.0.0.1", argv[1]);
    endpoint.setLocal (a0);
    while (true)
    {
    	const auto [ when, payload ] = endpoint.recv ();
    	printf ("%lu\t%s\n", when, payload.c_str ());
    }

    return 0;
}
