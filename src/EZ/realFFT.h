#pragma once

#include <span>
#include <cmath>

//-----------------------------------------------------------------------------

namespace libsidplayEZ::realFFT
{
	void split ( std::span<float> data );

	void window_BlackmanHarris ( std::span<float> data );
	void window_Hann ( std::span<float> data );

	void applyWindow ( std::span<float> dst, std::span<float> src, std::span<float> window );
	void applyWindow ( std::span<float> dst, std::span<float> window );

//	constexpr auto linearToDecibel = [] ( float gain ) -> float { return ( std::log2 ( gain ) / std::log2 ( 10.0f ) ) * 20.0f; };
	constexpr auto linearToDecibel = [] ( float gain ) -> float { return ( std::log10 ( gain ) ) * 20.0f; };
	constexpr auto decibleToLinear = [] ( float db ) -> float {	return std::pow ( 10.0f, db * 0.05f ); };
}
//-----------------------------------------------------------------------------
