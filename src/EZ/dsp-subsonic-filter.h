#pragma once

#include <cmath>
#include <numbers>

namespace libsidplayEZ::dsp
{

// 2nd-order Butterworth high-pass, in-place block processing.
// float I/O, double coefficients/state (mandatory at fc/fs this small).
class SubsonicFilter
{
public:
	SubsonicFilter ()
	{
		setup ( 44100.0 );
	}

	void setup ( double fs, double fc = 16.0, double q = 0.70710678118654752 )
	{
		const auto  w0 = 2.0 * std::numbers::pi * fc / fs;
		const auto  cw = std::cos ( w0 );
		const auto  alpha = std::sin ( w0 ) / ( 2.0 * q );
		const auto  a0 = 1.0 + alpha;

		b0 = ( 1.0 + cw ) * 0.5 / a0;
		b1 = -( 1.0 + cw ) / a0;
		b2 = b0;
		a1 = -2.0 * cw / a0;
		a2 = ( 1.0 - alpha ) / a0;

		reset ();
	}

	void process ( float* data, int numSamples )
	{
		// local copies help the optimizer
		auto    s1 = z1;
		auto    s2 = z2;
		for ( auto i = 0; i < numSamples; ++i )
		{
			const auto  in = data[ i ];
			const auto  out = b0 * in + s1;
			s1 = b1 * in - a1 * out + s2;
			s2 = b2 * in - a2 * out;
			data[ i ] = static_cast<float> ( out );
		}
		z1 = s1;
		z2 = s2;
	}

	void reset ()
	{
		z1 = 0.0;
		z2 = 0.0;
	}

private:
	double b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
	double z1 = 0, z2 = 0;
};

}