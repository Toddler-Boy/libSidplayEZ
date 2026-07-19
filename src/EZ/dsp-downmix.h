#pragma once

#include <cmath>
#include <numbers>

namespace libsidplayEZ::dsp
{

inline void downMix ( float* __restrict__ srcDstL, float* __restrict__ srcDstR, const int numSamples, const float width )
{
	// Snap threshold: residual side level at width 1e-3 is ~-60 dB, inaudible
	constexpr auto	epsilon = 1e-3f;

	// Pure stereo, no down mixing required
	if ( width >= ( 1.0f - epsilon ) )
		return;

	constexpr auto	invSqrt2 = std::numbers::sqrt2_v<float> * 0.5f;

	// Pure mono
	if ( width < epsilon )
	{
		for ( auto i = 0; i < numSamples; ++i )
		{
			const auto	mid = ( srcDstL[ i ] + srcDstR[ i ] ) * invSqrt2;

			srcDstL[ i ] = mid;
			srcDstR[ i ] = mid;
		}
		return;
	}

	const auto	midGain = std::cos ( 0.25f * std::numbers::pi_v<float> * width ) * invSqrt2;
	const auto	sideGain = std::sin ( 0.25f * std::numbers::pi_v<float> * width ) * invSqrt2;

	// Down mix
	for ( auto i = 0; i < numSamples; ++i )
	{
		const auto	left = srcDstL[ i ];
		const auto	right = srcDstR[ i ];

		const auto	mid = ( left + right ) * midGain;
		const auto	side = ( left - right ) * sideGain;

		srcDstL[ i ] = mid + side;
		srcDstR[ i ] = mid - side;
	}
}

}
