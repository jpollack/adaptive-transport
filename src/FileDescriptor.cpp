#include "FileDescriptor.hpp"
#include "Util.hpp"

#include <unistd.h>

/* construct from fd number */
FileDescriptor::FileDescriptor( const int fd )
  : fd_( fd ),
    eof_( false ),
    read_count_( 0 ),
    write_count_( 0 )
{}

/* move constructor */
FileDescriptor::FileDescriptor( FileDescriptor && other )
  : fd_( other.fd_ ),
    eof_( other.eof_ ),
    read_count_( other.read_count_ ),
    write_count_( other.write_count_ )
{
  /* mark other file descriptor as inactive */
  other.fd_ = -1;
}

/* destructor */
FileDescriptor::~FileDescriptor()
{
  if ( fd_ < 0 ) { /* has already been moved away */
    return;
  }

  SystemCall( "close", close( fd_ ) );
}

/* attempt to write a portion of a string */
std::string::const_iterator FileDescriptor::write( const std::string::const_iterator & begin,
					      const std::string::const_iterator & end )
{
  if ( begin >= end ) {
      fprintf (stderr, "nothing to write\n");
      exit (1);
  }

  ssize_t bytes_written = SystemCall( "write", ::write( fd_, &*begin, end - begin ) );
  if ( bytes_written == 0 ) {
      fprintf (stderr, "write returned 0\n");
      exit (1);
  }

  register_write();

  return begin + bytes_written;
}

/* read method */
std::string FileDescriptor::read( const size_t limit )
{
  constexpr size_t BUFFER_SIZE = 1024 * 1024;   /* maximum size of a read */
  char buffer[ BUFFER_SIZE ];

  ssize_t bytes_read = SystemCall( "read", ::read( fd_, buffer, std::min( BUFFER_SIZE, limit ) ) );
  if ( bytes_read == 0 ) {
    set_eof();
  }

  register_read();

  return std::string( buffer, bytes_read );
}

/* write method */
std::string::const_iterator FileDescriptor::write( const std::string & buffer, const bool write_all )
{
  auto it = buffer.begin();

  do {
      it = write( it, buffer.end() );
  } while ( write_all and (it != buffer.end()) );

  return it;
}
