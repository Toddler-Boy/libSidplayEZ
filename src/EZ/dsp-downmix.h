#pragma once

#include <cmath>
#include <numbers>

namespace libsidplayEZ::dsp
{

void downMix ( float* __restrict__ srcDstL, float* __restrict__ srcDstR, const int numSamples, const float width )
{
	// Pure stereo, no down mixing required
	if ( width >= ( 1.0f - 1e-6f ) )
		return;

	constexpr auto	quarterSine = 0.70710678118f;

	// Pure mono
	if ( width < 1e-6f )
	{
		for ( auto i = 0; i < numSamples; ++i )
		{
			const auto	mid = ( srcDstL[ i ] + srcDstR[ i ] ) * quarterSine;

			srcDstL[ i ] = mid;
			srcDstR[ i ] = mid;
		}
		return;
	}

	const auto	midGain = std::cos ( 0.25f * std::numbers::pi_v<float> * width ) * quarterSine;
	const auto	sideGain = std::sin ( 0.25f * std::numbers::pi_v<float> * width ) * quarterSine;

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
