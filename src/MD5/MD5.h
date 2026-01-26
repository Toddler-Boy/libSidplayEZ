#pragma once

/*
 * This code has been derived by Michael Schwendt <mschwendt@yahoo.com>
 * from original work by L. Peter Deutsch <ghost@aladdin.com>.
 *
 * The original C code (md5.c, md5.h) is available here:
 * ftp://ftp.cs.wisc.edu/ghost/packages/md5.tar.gz
 */

 /*
   Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

   This software is provided 'as-is', without any express or implied
   warranty.  In no event will the authors be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
	  claim that you wrote the original software. If you use this software
	  in a product, an acknowledgment in the product documentation would be
	  appreciated but is not required.
   2. Altered source versions must be plainly marked as such, and must not be
	  misrepresented as being the original software.
   3. This notice may not be removed or altered from any source distribution.

   L. Peter Deutsch
   ghost@aladdin.com
  */

#include <cstdint>
#include <string>

class MD5
{
public:
	// Initialize the algorithm. Reset starting values.
	MD5 ();

	// Append a string to the message.
	void append ( const void* data, int nbytes );

	// Finish the message.
	void finish ();

	// Return pointer to 16-byte fingerprint.
	const uint8_t* getDigest ();
	std::string getAscIIDigest ();

	// Initialize the algorithm. Reset starting values.
	void reset ();

private:

	/* Define the state of the MD5 Algorithm. */
	uint32_t count[ 2 ];	/* message length in bits, lsw first */
	uint32_t abcd[ 4 ];		/* digest buffer */
	uint8_t buf[ 64 ];		/* accumulate block */

	uint8_t digest[ 16 ];

	uint32_t tmpBuf[ 16 ];
	const uint32_t* X;

	void process ( const uint8_t data[ 64 ] );

	uint32_t ROTATE_LEFT ( const uint32_t x, const int n );

	uint32_t F ( const uint32_t x, const uint32_t y, const uint32_t z );

	uint32_t G ( const uint32_t x, const uint32_t y, const uint32_t z );

	uint32_t H ( const uint32_t x, const uint32_t y, const uint32_t z );

	uint32_t I ( const uint32_t x, const uint32_t y, const uint32_t z );

	typedef uint32_t ( MD5::* md5func )( const uint32_t x, const uint32_t y, const uint32_t z );

	void SET ( md5func func, uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d, const int k, const int s, const uint32_t Ti );
};

inline uint32_t MD5::ROTATE_LEFT ( const uint32_t x, const int n )
{
	return ( ( x << n ) | ( x >> ( 32 - n ) ) );
}

inline uint32_t MD5::F ( const uint32_t x, const uint32_t y, const uint32_t z )
{
	return ( ( x & y ) | ( ~x & z ) );
}

inline uint32_t MD5::G ( const uint32_t x, const uint32_t y, const uint32_t z )
{
	return ( ( x & z ) | ( y & ~z ) );
}

inline uint32_t MD5::H ( const uint32_t x, const uint32_t y, const uint32_t z )
{
	return ( x ^ y ^ z );
}

inline uint32_t MD5::I ( const uint32_t x, const uint32_t y, const uint32_t z )
{
	return ( y ^ ( x | ~z ) );
}

inline void MD5::SET ( md5func func, uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d, const int k, const int s, const uint32_t Ti )
{
	uint32_t t = a + ( this->*func )( b, c, d ) + X[ k ] + Ti;
	a = ROTATE_LEFT ( t, s ) + b;
}
