#include "declick.h"

#include "realFFT.h"

#include <algorithm>
#include <cmath>
#include <bit>
#include <numbers>
#include <vector>

//-----------------------------------------------------------------------------

void libsidplayEZ::declick ( std::span<float> _source, const int _sampleRate )
{
	//
	// The basic algorithm is to convert up to 1 second of audio into FFT data and then skip forward
	// until at least three consecutive FFT blocks are above a certain threshold.
	// This is considered the start of the song.
	//
	std::vector<float> densities;

	auto	maxDensity = 0.0f;
	auto	hopSize = 0;

	//
	// Analyze
	//
	{
		// set up some basic parameters
		const auto		frames = std::min ( int ( _source.size () ), _sampleRate );
		constexpr auto	baseWindowSize = 256;
		constexpr auto	baseHop = baseWindowSize / 2;
		const auto		windowSize = int ( std::bit_ceil ( (unsigned int)( baseWindowSize * 44100 / _sampleRate ) ) );
		const auto		hzPerBin = float ( _sampleRate ) / float ( windowSize );
		const auto		binCount = int ( 11000.0f / hzPerBin );

		hopSize = int ( std::bit_ceil ( (unsigned int)( baseHop * 44100 / _sampleRate ) ) );

		// max gain
		auto	maxGain = 0.0f;
		for ( auto pos = 0; pos < frames; pos++ )
			maxGain = std::max ( maxGain, std::abs ( _source[ pos ] ) );

		// create window
		std::vector<float>	hann ( windowSize );
		realFFT::window_Hann ( hann );

		// calculate densities
		std::vector<float>	spectrum ( windowSize * 2 );

		for ( auto pos = 0; pos < frames; pos += hopSize )
		{
			// copy snippet centered around pos
			std::fill_n ( spectrum.data (), windowSize, 0.0f );

			const auto	copySize = std::min ( windowSize, frames - pos );
			for ( auto i = 0; i < copySize; i++ )
			{
				const auto	sourcePos = pos + i - ( windowSize / 2 );
				if ( sourcePos < 0 || sourcePos >= frames )
					continue;

				auto shape = [ maxGain ] ( float _x ) -> float
				{
					// squash everything above 50% of max gain
					constexpr auto	ratio = 1.0f / 4.0f;
					const auto		threshold = 0.5f * maxGain;
					const auto		absX = std::abs ( _x );
					const auto		absShaped = absX > threshold ? ( absX - threshold ) * ratio + threshold : absX;

					return std::copysign ( absShaped, _x );
				};

				spectrum[ i ] += shape ( _source[ sourcePos ] );
			}

			// apply window
			realFFT::applyWindow ( spectrum, hann );

			// convert to spectrum
			realFFT::split ( { spectrum.data (), spectrum.size () / 2 } );

			// measure density
			auto	density = 0.0f;
			for ( auto bin = 1; bin < binCount; bin++ )
			{
				const auto	real = spectrum[ bin ];
				const auto	imag = spectrum[ windowSize - bin ];

				if ( std::abs ( real ) < 0.00001f && std::abs ( imag ) < 0.00001f )
					continue;

				constexpr auto	dbRange = 70.0f;

				const auto	gain = std::sqrt ( real * real + imag * imag );
				const auto	value = std::clamp ( dbRange + realFFT::linearToDecibel ( gain ), 0.0f, dbRange );

				density += value;
			}

			densities.emplace_back ( density );
			maxDensity = std::max ( maxDensity, density );
		}
	}

	//
	// Declick
	//
	{
		auto	songStart = 0;

		// Find start of song
		{
			const auto	threshold = 0.04f * maxDensity;

			// look for 3 consecutive high density blocks that mark the start of the song - 2 or 1 are considered clicks
			for ( auto hop = 0; hop < int ( densities.size () - 2 ); hop++ )
			{
				if ( densities[ hop ] > threshold && densities[ hop + 1 ] > threshold && densities[ hop + 2 ] > threshold )
				{
					songStart = hop * hopSize;
					break;
				}
			}
		}

		if ( ! songStart )
			return;

 		// fade in (5ms)
 		const auto	fadeInStart = std::max ( 0, songStart - int ( _sampleRate * 0.005f ) );
 
 		if ( fadeInStart )
 			std::fill_n ( _source.begin (), fadeInStart, 0.0f );

 		const auto	fadeLength = songStart - fadeInStart;
 		for ( auto pos = 0; pos < fadeLength; pos++ )
 			_source[ fadeInStart + pos ] *= float ( pos ) / float ( fadeLength );
	}
}
//-----------------------------------------------------------------------------
