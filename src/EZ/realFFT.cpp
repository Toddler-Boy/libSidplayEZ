#include <numbers>
#include <cmath>

#include "realFFT.h"

//-----------------------------------------------------------------------------

namespace libsidplayEZ::realFFT
{

void split ( std::span<float> data )
{
	const auto	n = int ( data.size () );

	int		i, j, k, i5, i6, i7, i8, i0, id, i1, i2, i3, i4, n2, n4, n8;
	float	t1, t2, t3, t4, t5, t6, a3, ss1, ss3, cc1, cc3, a, e;

	constexpr auto	sqrt2 = float ( std::numbers::sqrt2 );

	n4 = n - 1;

	//data shuffling
	for ( i = 0, j = 0, n2 = n / 2; i < n4; i++ )
	{
		if ( i < j )
		{
			t1 = data[ j ];
			data[ j ] = data[ i ];
			data[ i ] = t1;
		}
		k = n2;
		while ( k <= j )
		{
			j -= k;
			k >>= 1;
		}
		j += k;
	}
	/*----------------------*/

	//length two butterflies	
	i0 = 0;
	id = 4;
	do
	{
		for ( ; i0 < n4; i0 += id )
		{
			i1 = i0 + 1;
			t1 = data[ i0 ];
			data[ i0 ] = t1 + data[ i1 ];
			data[ i1 ] = t1 - data[ i1 ];
		}
		id <<= 1;
		i0 = id - 2;
		id <<= 1;
	} while ( i0 < n4 );

	/*----------------------*/
	//L shaped butterflies
	n2 = 2;
	for ( k = n; k > 2; k >>= 1 )
	{
		n2 <<= 1;
		n4 = n2 >> 2;
		n8 = n2 >> 3;
		e = float ( std::numbers::pi * 2.0f ) / n2;
		i1 = 0;
		id = n2 << 1;
		do
		{
			for ( ; i1 < n; i1 += id )
			{
				i2 = i1 + n4;
				i3 = i2 + n4;
				i4 = i3 + n4;
				t1 = data[ i4 ] + data[ i3 ];
				data[ i4 ] -= data[ i3 ];
				data[ i3 ] = data[ i1 ] - t1;
				data[ i1 ] += t1;
				if ( n4 != 1 )
				{
					i0 = i1 + n8;
					i2 += n8;
					i3 += n8;
					i4 += n8;
					t1 = ( data[ i3 ] + data[ i4 ] ) / sqrt2;
					t2 = ( data[ i3 ] - data[ i4 ] ) / sqrt2;
					data[ i4 ] = data[ i2 ] - t1;
					data[ i3 ] = -data[ i2 ] - t1;
					data[ i2 ] = data[ i0 ] - t2;
					data[ i0 ] += t2;
				}
			}
			id <<= 1;
			i1 = id - n2;
			id <<= 1;
		} while ( i1 < n );
		a = e;
		for ( j = 2; j <= n8; j++ )
		{
			a3 = 3 * a;
			cc1 = std::cos ( a );
			ss1 = std::sin ( a );
			cc3 = std::cos ( a3 );
			ss3 = std::sin ( a3 );
			a = j * e;
			i = 0;
			id = n2 << 1;
			do
			{
				for ( ; i < n; i += id )
				{
					i1 = i + j - 1;
					i2 = i1 + n4;
					i3 = i2 + n4;
					i4 = i3 + n4;
					i5 = i + n4 - j + 1;
					i6 = i5 + n4;
					i7 = i6 + n4;
					i8 = i7 + n4;
					t1 = data[ i3 ] * cc1 + data[ i7 ] * ss1;
					t2 = data[ i7 ] * cc1 - data[ i3 ] * ss1;
					t3 = data[ i4 ] * cc3 + data[ i8 ] * ss3;
					t4 = data[ i8 ] * cc3 - data[ i4 ] * ss3;
					t5 = t1 + t3;
					t6 = t2 + t4;
					t3 = t1 - t3;
					t4 = t2 - t4;
					t2 = data[ i6 ] + t6;
					data[ i3 ] = t6 - data[ i6 ];
					data[ i8 ] = t2;
					t2 = data[ i2 ] - t3;
					data[ i7 ] = -data[ i2 ] - t3;
					data[ i4 ] = t2;
					t1 = data[ i1 ] + t5;
					data[ i6 ] = data[ i1 ] - t5;
					data[ i1 ] = t1;
					t1 = data[ i5 ] + t4;
					data[ i5 ] -= t4;
					data[ i2 ] = t1;
				}
				id <<= 1;
				i = id - n2;
				id <<= 1;
			} while ( i < n );
		}
	}

	//division with array length
	const auto	invN = 1.0f / n;
	for ( i = 0; i < n; i++ )
		data[ i ] *= invN;
}
//------------------------------------------------------------------------------

void window_BlackmanHarris ( std::span<float> data )
{
	const auto	n = int ( data.size () );

	// Pre-calculate blackman-harris window
	for ( auto i = 0; i < n; ++i )
	{
		const auto	phase = float ( std::numbers::pi ) * 2.0f * ( float ( i ) / float ( n - 1 ) );
		data[ i ] = 0.35875f - 0.48829f * std::cos ( phase ) + 0.14128f * std::cos ( phase * 2.0f ) - 0.01168f * std::cos ( phase * 3.0f );
	}
}
//------------------------------------------------------------------------------

void window_Hann ( std::span<float> data )
{
	const auto	n = int ( data.size () );

	// Pre-calculate blackman-harris window
	for ( auto i = 0; i < n; ++i )
		data[ i ] = 0.5f * ( 1.0f - std::cos ( 2.0f * float ( std::numbers::pi ) * float ( i ) / float ( n - 1 ) ) );
}
//------------------------------------------------------------------------------

void applyWindow ( std::span<float> dst, std::span<float> src, std::span<float> window )
{
	const auto	n = int ( window.size () );

	for ( auto i = 0; i < n; ++i )
		dst[ i ] = src[ i ] * window[ i ];
}
//------------------------------------------------------------------------------

void applyWindow ( std::span<float> dst, std::span<float> window )
{
	const auto	n = int ( window.size () );

	for ( auto i = 0; i < n; ++i )
		dst[ i ] *= window[ i ];
}
//------------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
