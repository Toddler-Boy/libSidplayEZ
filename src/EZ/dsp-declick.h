#pragma once

#include <span>

//-----------------------------------------------------------------------------

namespace libsidplayEZ::dsp
{
	void declick ( std::span<float> _source, const int _sampleRate );
}
//-----------------------------------------------------------------------------
