#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace reSIDfp
{
	class DigiCapture;
	class EnvelopeGenerator;
	class ExternalFilter;
	class SincResampler;
	template<bool> class Filter;
	template<bool> class Filter6581;
	template<bool> class Filter8580;
	template<bool> class Voice;
	template<bool> class WaveformGenerator;
	template<typename> class SID;
}

namespace libsidplayfp
{

class c64;
class Event;
class EventScheduler;
class InterruptSource;
class MMU;
class MOS6510;
class MOS652X;
class MOS656X;
class Player;
class SerialPort;
class sidemu;
class Timer;
class Tod;
template<typename> class sidemuSpec;

// Byte-exact snapshot of a Player between two play () calls: machine, event queue,
// every SID chip, and the RAM bytes that differ from the image initialise () produced
class SaveState final
{
public:
	static constexpr uint8_t	formatVersion = 1;

	// Keeps the RAM image initialise () produced, the baseline save () diffs against
	static void captureReference ( Player& player );

	// False when the machine holds an event the format does not know
	[[ nodiscard ]] static bool save ( Player& player, std::vector<uint8_t>& out );

	// The player must have loaded and initialised the same tune with the same
	// config. False on a version, chip or size mismatch, the player is then unusable
	[[ nodiscard ]] static bool restore ( Player& player, std::span<const uint8_t> in );

private:
	class Writer;
	class Reader;

	static void collectEvents ( c64& machine, std::vector<Event*>& events );

	template<class A> static bool io ( A& a, Player& player );
	template<class A> static bool ioQueue ( A& a, c64& machine );
	template<class A> static bool ioChip ( A& a, sidemu& chip );
	template<class A> static void ioRam ( A& a, uint8_t* ram, const uint8_t* reference, uint32_t size );

	template<class A> static void io ( A& a, c64& machine );
	template<class A> static void io ( A& a, EventScheduler& scheduler );
	template<class A> static void io ( A& a, MOS6510& cpu );
	template<class A> static void io ( A& a, MMU& mmu );
	template<class A> static void io ( A& a, MOS652X& cia );
	template<class A> static void io ( A& a, Timer& timer );
	template<class A> static void io ( A& a, Tod& tod );
	template<class A> static void io ( A& a, InterruptSource& source );
	template<class A> static void io ( A& a, SerialPort& port );
	template<class A> static void io ( A& a, MOS656X& vic );

	template<class A, typename FLT> static void io ( A& a, sidemuSpec<FLT>& chip );
	template<class A, typename FLT> static void io ( A& a, reSIDfp::SID<FLT>& sid );
	template<class A, bool F> static void io ( A& a, reSIDfp::Voice<F>& voice );
	template<class A, bool F> static void io ( A& a, reSIDfp::WaveformGenerator<F>& wave );
	template<class A> static void io ( A& a, reSIDfp::EnvelopeGenerator& env );
	template<class A, bool U> static void io ( A& a, reSIDfp::Filter<U>& filter, const uint8_t* lastpoke );
	template<class A, bool U> static void io ( A& a, reSIDfp::Filter6581<U>& filter, const uint8_t* lastpoke );
	template<class A, bool U> static void io ( A& a, reSIDfp::Filter8580<U>& filter, const uint8_t* lastpoke );
	template<class A> static void io ( A& a, reSIDfp::ExternalFilter& filter );
	template<class A> static void io ( A& a, reSIDfp::SincResampler& resampler );
	template<class A> static void io ( A& a, reSIDfp::DigiCapture& digi );
};

}
