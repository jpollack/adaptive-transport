#include <unistd.h>
#include <utility>
#include <cstdint>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

std::pair<bool,size_t> full_read (int fd, void *buf, size_t size)
{
    uint8_t *p = (uint8_t *)buf;
    size_t total = 0;
    while (size > 0)
    {
	ssize_t bytes = 0;
	do
	{
	    bytes = read (fd, p, size);
	} while ((bytes == -1) && (errno == EINTR));

	if (bytes == -1)
	{
	    perror ("read");
	    abort ();
	}
	if (bytes == 0)
	{
	    // eof condition
	    return std::make_pair (true, total);
	}
	total += bytes;
	p += bytes;
	size -= bytes;
    }
    return std::make_pair (false, total);
}

size_t full_write (int fd, const void *buf, size_t size)
{
    const uint8_t *p = (const uint8_t *)buf;
    size_t total = 0;
    while (size > 0)
    {
	ssize_t bytes = 0;
	do
	{
	    bytes = write (fd, p, size);
	} while ((bytes == -1) && (errno == EINTR));

	if (bytes == -1)
	{
	    perror ("write");
	    abort ();
	}
	total += bytes;
	p += bytes;
	size -= bytes;
    }
    return total;
}
