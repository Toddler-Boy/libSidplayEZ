#pragma once

#include <cstdint>

namespace reSIDfp
{

// How the digi buffer derives its signed display-ready samples: the reading
// modes convert their register's shadow byte (volume nibble, whole byte
// through the model's Mahoney level table, centered freq-hi), the voice modes
// compute from live voice state, output copies the final mix (techniques
// resynthesizing audio on all voices at once). The raw* modes are the
// measurement variants: the plain centered byte of a computed mode's write
// register
enum class DigiMode : uint8_t { nibble, mahoney, freq1, freq2, freq3, pwLo1, pwHi1, pwFull1, filt1, voice3Out, voice1Pwm, covox, carmina, escos, output, rawCtrl3, rawPw1 };

}
