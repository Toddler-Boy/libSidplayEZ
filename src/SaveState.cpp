#include "SaveState.h"

#include <algorithm>
#include <cstring>
#include <type_traits>

#include "player.h"
#include "sidemu.h"

namespace libsidplayfp
{

namespace
{
	constexpr auto	ramSize = 0x10000u;
	constexpr auto	colorRamSize = 0x400u;

	// Differing bytes closer than this share one run
	constexpr auto	runMergeGap = 4u;
	constexpr auto	maxRunLength = 0x8000u;
}

//-----------------------------------------------------------------------------

class SaveState::Writer final
{
public:
	static constexpr bool	isReader = false;

	explicit Writer ( std::vector<uint8_t>& _out ) : out ( _out ) {}

	template<class... T> void operator() ( const T&... v )	{	( one ( v ), ... );	}

	void bytes ( const void* p, const size_t n )
	{
		const auto*	b = static_cast<const uint8_t*> ( p );
		out.insert ( out.end (), b, b + n );
	}

	void fail ()	{	good = false;	}

	[[ nodiscard ]] bool ok () const	{	return good;	}

private:
	template<class T> void one ( const T& v )
	{
		static_assert ( std::is_trivially_copyable_v<T> );
		bytes ( &v, sizeof ( T ) );
	}

	std::vector<uint8_t>&	out;
	bool	good = true;
};
//-----------------------------------------------------------------------------

class SaveState::Reader final
{
public:
	static constexpr bool	isReader = true;

	explicit Reader ( std::span<const uint8_t> _in ) : in ( _in ) {}

	template<class... T> void operator() ( T&... v )	{	( one ( v ), ... );	}

	void bytes ( void* p, const size_t n )
	{
		if ( pos + n > in.size () )
		{
			good = false;
			return;
		}

		std::memcpy ( p, in.data () + pos, n );
		pos += n;
	}

	void fail ()	{	good = false;	}

	[[ nodiscard ]] bool ok () const	{	return good;	}
	[[ nodiscard ]] bool atEnd () const	{	return good && pos == in.size ();	}

private:
	template<class T> void one ( T& v )
	{
		static_assert ( std::is_trivially_copyable_v<T> );
		bytes ( &v, sizeof ( T ) );
	}

	std::span<const uint8_t>	in;
	size_t	pos = 0;
	bool	good = true;
};
//-----------------------------------------------------------------------------

void SaveState::captureReference ( Player& player )
{
	auto&	m = player.m_c64;

	player.m_stateReference.resize ( ramSize + colorRamSize );
	std::copy_n ( m.mmu.ramBank.ram, ramSize, player.m_stateReference.data () );
	std::copy_n ( m.colorRAMBank.ram, colorRamSize, player.m_stateReference.data () + ramSize );
}
//-----------------------------------------------------------------------------

bool SaveState::save ( Player& player, std::vector<uint8_t>& out )
{
	out.clear ();

	Writer	w ( out );
	return io ( w, player ) && w.ok ();
}
//-----------------------------------------------------------------------------

bool SaveState::restore ( Player& player, std::span<const uint8_t> in )
{
	Reader	r ( in );
	return io ( r, player ) && r.atEnd ();
}
//-----------------------------------------------------------------------------

template<class A>
bool SaveState::io ( A& a, Player& player )
{
	auto	version = formatVersion;
	a ( version );
	if ( version != formatVersion )
		return false;

	auto	numChips = uint8_t ( player.m_sidEmu.size () );
	a ( numChips );
	if ( numChips != player.m_sidEmu.size () || numChips == 0 )
		return false;

	if ( player.m_stateReference.size () != ramSize + colorRamSize )
		return false;

	auto&	m = player.m_c64;

	io ( a, m );

	if ( ! ioQueue ( a, m ) )
		return false;

	for ( auto* chip : player.m_sidEmu )
		if ( ! ioChip ( a, *chip ) )
			return false;

	ioRam ( a, m.mmu.ramBank.ram, player.m_stateReference.data (), ramSize );
	ioRam ( a, m.colorRAMBank.ram, player.m_stateReference.data () + ramSize, colorRamSize );

	return a.ok ();
}
//-----------------------------------------------------------------------------

// The fixed table every queue entry refers to by index
void SaveState::collectEvents ( c64& m, std::vector<Event*>& events )
{
	auto ciaEvents = [ &events ] ( MOS652X& cia )
	{
		events.push_back ( &static_cast<Event&> ( static_cast<Timer&> ( cia.timerA ) ) );
		events.push_back ( &cia.timerA.m_cycleSkippingEvent );
		events.push_back ( &static_cast<Event&> ( static_cast<Timer&> ( cia.timerB ) ) );
		events.push_back ( &cia.timerB.m_cycleSkippingEvent );

		auto&	irq = *cia.interruptSource;
		events.push_back ( &irq.interruptEvent );
		events.push_back ( &irq.updateIdrEvent );
		events.push_back ( &irq.setIrqEvent );
		events.push_back ( &irq.clearIrqEvent );

		events.push_back ( &static_cast<Event&> ( cia.tod ) );
		events.push_back ( &static_cast<Event&> ( cia.serialPort ) );
		events.push_back ( &cia.serialPort.flipCntEvent );
		events.push_back ( &cia.serialPort.flipFakeEvent );
		events.push_back ( &cia.serialPort.startSdrEvent );
		events.push_back ( &cia.bTickEvent );
	};

	events.push_back ( &m.cpu.m_nosteal );
	events.push_back ( &m.cpu.m_steal );
	events.push_back ( &m.cpu.clearInt );

	ciaEvents ( m.cia1 );
	ciaEvents ( m.cia2 );

	events.push_back ( &static_cast<Event&> ( static_cast<MOS656X&> ( m.vic ) ) );
	events.push_back ( &m.vic.badLineStateChangeEvent );
	events.push_back ( &m.vic.rasterYIRQEdgeDetectorEvent );
	events.push_back ( &m.vic.lightpenTriggerEvent );
}
//-----------------------------------------------------------------------------

// The queue is stored in list order, so same-time events fire as they would have
template<class A>
bool SaveState::ioQueue ( A& a, c64& m )
{
	std::vector<Event*>	events;
	collectEvents ( m, events );

	auto&	s = m.eventScheduler;

	if constexpr ( A::isReader )
	{
		uint8_t	count = 0;
		a ( count );
		if ( ! a.ok () )
			return false;

		s.firstEvent = nullptr;
		auto*	tail = &s.firstEvent;

		for ( auto i = 0; i < count; ++i )
		{
			uint8_t			id = 0;
			event_clock_t	time = 0;
			a ( id, time );
			if ( ! a.ok () || id >= events.size () )
				return false;

			auto*	e = events[ id ];
			e->triggerTime = time;
			e->next = nullptr;
			*tail = e;
			tail = &e->next;
		}
	}
	else
	{
		std::vector<std::pair<uint8_t, event_clock_t>>	queued;

		for ( auto* e = s.firstEvent; e; e = e->next )
		{
			const auto	it = std::ranges::find ( events, e );
			if ( it == events.end () )
				return false;

			queued.emplace_back ( uint8_t ( it - events.begin () ), e->triggerTime );
		}

		auto	count = uint8_t ( queued.size () );
		a ( count );
		for ( auto& [ id, time ] : queued )
			a ( id, time );
	}

	return true;
}
//-----------------------------------------------------------------------------

template<class A>
bool SaveState::ioChip ( A& a, sidemu& chip )
{
	using namespace reSIDfp;

	auto dispatch = [ & ] ( auto* spec, const uint8_t expected ) -> bool
	{
		auto	tag = expected;
		a ( tag );
		if ( tag != expected )
			return false;

		io ( a, *spec );
		return true;
	};

	if ( auto* e = dynamic_cast<sidemuSpec<Filter6581<true>>*> ( &chip ) )	return dispatch ( e, 0 );
	if ( auto* e = dynamic_cast<sidemuSpec<Filter6581<false>>*> ( &chip ) )	return dispatch ( e, 1 );
	if ( auto* e = dynamic_cast<sidemuSpec<Filter8580<true>>*> ( &chip ) )	return dispatch ( e, 2 );
	if ( auto* e = dynamic_cast<sidemuSpec<Filter8580<false>>*> ( &chip ) )	return dispatch ( e, 3 );

	return false;
}
//-----------------------------------------------------------------------------

// Runs of bytes that differ from the reference image: offset, length, bytes
template<class A>
void SaveState::ioRam ( A& a, uint8_t* ram, const uint8_t* reference, const uint32_t size )
{
	if constexpr ( A::isReader )
	{
		uint16_t	count = 0;
		a ( count );

		for ( auto i = 0; i < count && a.ok (); ++i )
		{
			uint16_t	offset = 0;
			uint16_t	length = 0;
			a ( offset, length );
			if ( ! a.ok () || uint32_t ( offset ) + length > size )
			{
				a.fail ();
				return;
			}

			a.bytes ( ram + offset, length );
		}
	}
	else
	{
		std::vector<std::pair<uint16_t, uint16_t>>	runs;

		for ( auto i = 0u; i < size; )
		{
			if ( ram[ i ] == reference[ i ] )
			{
				++i;
				continue;
			}

			auto	last = i;
			for ( auto j = i + 1; j < size && j - last <= runMergeGap; ++j )
				if ( ram[ j ] != reference[ j ] )
					last = j;

			const auto	end = last + 1;
			for ( auto start = i; start < end; start += maxRunLength )
				runs.emplace_back ( uint16_t ( start ), uint16_t ( std::min ( end - start, maxRunLength ) ) );

			i = end;
		}

		auto	count = uint16_t ( runs.size () );
		a ( count );

		for ( auto& [ offset, length ] : runs )
		{
			a ( offset, length );
			a.bytes ( ram + offset, length );
		}
	}
}
//-----------------------------------------------------------------------------

template<class A>
void SaveState::io ( A& a, c64& m )
{
	io ( a, m.eventScheduler );
	a ( m.irqCount, m.oldBAState, m.irqTime, m.irqStart );
	io ( a, m.cpu );
	io ( a, m.mmu );
	io ( a, m.cia1 );
	a ( m.cia1.last_ta );
	io ( a, m.cia2 );
	io ( a, m.vic );
}
//-----------------------------------------------------------------------------

template<class A>
void SaveState::io ( A& a, EventScheduler& s )
{
	a ( s.currentTime );
}
//-----------------------------------------------------------------------------

template<class A>
void SaveState::io ( A& a, MOS6510& cpu )
{
	a ( cpu.cycleCount, cpu.interruptCycle,
		cpu.irqAssertedOnPin, cpu.nmiFlag, cpu.rstFlag, cpu.rdy, cpu.adl_carry, cpu.d1x1, cpu.rdyOnThrowAwayRead, cpu.jammed,
		cpu.Register_ProgramCounter, cpu.Cycle_EffectiveAddress, cpu.Cycle_Pointer, cpu.Cycle_Data,
		cpu.Register_StackPointer, cpu.Register_Accumulator, cpu.Register_X, cpu.Register_Y );

	auto	sr = cpu.flags.get ();
	a ( sr );
	if constexpr ( A::isReader )
		cpu.flags.set ( sr );
}
//-----------------------------------------------------------------------------

template<class A>
void SaveState::io ( A& a, MMU& mmu )
{
	a ( mmu.loram, mmu.hiram, mmu.charen, mmu.seed );

	auto&	z = mmu.zeroRAMBank;
	a ( z.dataBit6.dataSetClk, z.dataBit6.isFallingOff, z.dataBit6.dataSet );
	a ( z.dataBit7.dataSetClk, z.dataBit7.isFallingOff, z.dataBit7.dataSet );
	a ( z.dir, z.data, z.dataRead, z.procPortPins );

	if constexpr ( A::isReader )
		mmu.updateMappingPHI2 ();
}
//-----------------------------------------------------------------------------

template<class A>
void SaveState::io ( A& a, MOS652X& cia )
{
	a ( cia.regs );
	io ( a, cia.timerA );
	io ( a, cia.timerB );
	io ( a, *cia.interruptSource );
	io ( a, cia.tod );
	io ( a, cia.serialPort );
}
//-----------------------------------------------------------------------------

template<class A>
void SaveState::io ( A& a, Timer& t )
{
	a ( t.ciaEventPauseTime, t.pbToggle, t.timer, t.latch, t.lastControlValue, t.state );
}
//-----------------------------------------------------------------------------

template<class A>
void SaveState::io ( A& a, Tod& t )
{
	a ( t.cycles, t.todtickcounter, t.isLatched, t.isStopped, t.clock, t.latch, t.alarm );
}
//-----------------------------------------------------------------------------

template<class A>
void SaveState::io ( A& a, InterruptSource& i )
{
	a ( i.last_clear, i.last_set, i.icr, i.idr, i.idrTemp, i.scheduled, i.asserted );
}
//-----------------------------------------------------------------------------

template<class A>
void SaveState::io ( A& a, SerialPort& p )
{
	a ( p.lastSync, p.count, p.cnt, p.cntHistory, p.loaded, p.pending, p.forceFinish );
}
//-----------------------------------------------------------------------------

template<class A>
void SaveState::io ( A& a, MOS656X& v )
{
	a ( v.rasterClk, v.lineCycle, v.rasterY, v.yscroll,
		v.areBadLinesEnabled, v.isBadLine, v.rasterYIRQCondition, v.vblanking, v.lpAsserted,
		v.irqFlags, v.irqMask, v.regs );
	a ( v.lp.lpx, v.lp.lpy, v.lp.isTriggered );
	a ( v.sprites.exp_flop, v.sprites.dma, v.sprites.mc_base, v.sprites.mc );
}
//-----------------------------------------------------------------------------

template<class A, typename FLT>
void SaveState::io ( A& a, sidemuSpec<FLT>& chip )
{
	a ( chip.m_accessClk, chip.m_bufferpos );

	if ( chip.m_bufferpos < 0 || chip.m_bufferpos > int ( sidemu::OUTPUTBUFFERSIZE ) )
	{
		a.fail ();
		return;
	}

	// Samples clocked but not yet mixed
	a.bytes ( chip.m_buffer, size_t ( chip.m_bufferpos ) * sizeof ( int16_t ) );
	a.bytes ( chip.m_bufferDigi, size_t ( chip.m_bufferpos ) );

	io ( a, chip.m_sid );
}
//-----------------------------------------------------------------------------

template<class A, typename FLT>
void SaveState::io ( A& a, reSIDfp::SID<FLT>& s )
{
	a ( s.lastpoke );
	a ( s.startupDeclickActive, s.startupDeclickPending, s.startupDeclickNoteSeen,
		s.startupDeclickMinCycles, s.startupDeclickCapCycles, s.startupDeclickWriteGate, s.startupDeclickRapidRun );
	a ( s.busValueTtl, s.nextVoiceSync, s.busValue );

	for ( auto& v : s.voice )
		io ( a, v );

	io ( a, s.filter, s.lastpoke );
	io ( a, s.externalFilter );
	io ( a, s.resampler.s1 );
	io ( a, s.resampler.s2 );
	io ( a, s.digi );
}
//-----------------------------------------------------------------------------

template<class A, bool F>
void SaveState::io ( A& a, reSIDfp::Voice<F>& v )
{
	io ( a, v.waveformGenerator );
	io ( a, v.envelopeGenerator );
	a ( v.envLevel );
}
//-----------------------------------------------------------------------------

template<class A, bool F>
void SaveState::io ( A& a, reSIDfp::WaveformGenerator<F>& w )
{
	a ( w.pw, w.shift_register, w.shift_latch, w.shift_pipeline,
		w.ring_msb_mask, w.no_noise, w.noise_output, w.no_noise_or_noise_output, w.no_pulse, w.pulse_output,
		w.waveform, w.waveform_output, w.accumulator, w.freq, w.tri_saw_pipeline, w.osc3,
		w.shift_register_reset, w.floating_output_ttl,
		w.test, w.sync, w.test_or_reset, w.msb_rising );

	// The table pointers follow the waveform the way writeCONTROL_REG derives them
	if constexpr ( A::isReader )
	{
		w.wave = nullptr;
		w.pulldown = nullptr;

		if ( w.waveform )
		{
			auto	modWave = w.model_wave->data ();
			auto	modPulldown = w.model_pulldown->data ();

			w.wave = &modWave[ ( w.waveform & 0x3 ) << 12 ];

			switch ( w.waveform & 0x7 )
			{
				case 3:		w.pulldown = &modPulldown[ 0 << 12 ];										break;
				case 4:		w.pulldown = ( w.waveform & 0x8 ) ? &modPulldown[ 4 << 12 ] : nullptr;	break;
				case 5:		w.pulldown = &modPulldown[ 1 << 12 ];										break;
				case 6:		w.pulldown = &modPulldown[ 2 << 12 ];										break;
				case 7:		w.pulldown = &modPulldown[ 3 << 12 ];										break;
				default:																				break;
			}
		}
	}
}
//-----------------------------------------------------------------------------

template<class A>
void SaveState::io ( A& a, reSIDfp::EnvelopeGenerator& e )
{
	a ( e.lfsr, e.rate, e.exponential_counter, e.exponential_counter_period, e.new_exponential_counter_period,
		e.state_pipeline, e.envelope_pipeline, e.exponential_pipeline,
		e.state, e.next_state, e.counter_enabled, e.gate, e.resetLfsr,
		e.envelope_counter, e.attack, e.decay, e.sustain, e.release, e.env3 );
}
//-----------------------------------------------------------------------------

template<class A, bool U>
void SaveState::io ( A& a, reSIDfp::Filter<U>& f, const uint8_t* lastpoke )
{
	a ( f.Vlp, f.Vbp, f.Vhp, f.fc, f.voice3LeakIdx, f.filterModeRouting );

	// The table pointers follow the registers the way the write functions derive them
	if constexpr ( A::isReader )
	{
		f.currentVolume = f.volume + ( ( lastpoke[ 0x18 ] & 0x0F ) << 16 );

		if constexpr ( U )
			f.currentResonance = f.resonance + ( ( lastpoke[ 0x17 ] >> 4 ) << 16 );

		f.updateMixing ();
		f.updatedCenterFrequency ();
	}
}
//-----------------------------------------------------------------------------

template<class A, bool U>
void SaveState::io ( A& a, reSIDfp::Filter6581<U>& f, const uint8_t* lastpoke )
{
	io ( a, static_cast<reSIDfp::Filter<U>&> ( f ), lastpoke );
	a ( f.hpIntegrator.vx, f.hpIntegrator.vc, f.bpIntegrator.vx, f.bpIntegrator.vc );
}
//-----------------------------------------------------------------------------

template<class A, bool U>
void SaveState::io ( A& a, reSIDfp::Filter8580<U>& f, const uint8_t* lastpoke )
{
	io ( a, static_cast<reSIDfp::Filter<U>&> ( f ), lastpoke );
	a ( f.hpIntegrator.vx, f.hpIntegrator.vc, f.bpIntegrator.vx, f.bpIntegrator.vc );
}
//-----------------------------------------------------------------------------

template<class A>
void SaveState::io ( A& a, reSIDfp::ExternalFilter& f )
{
	a ( f.Vlp, f.Vhp );
}
//-----------------------------------------------------------------------------

template<class A>
void SaveState::io ( A& a, reSIDfp::SincResampler& r )
{
	a ( r.sampleIndex, r.sampleOffset, r.outputValue );

	// The FIR only ever reads the firN + 1 samples before the write cursor. They
	// are stored as 16-bit audio; a pass-1 overshoot beyond that fails the save
	constexpr auto	ringMask = reSIDfp::SincResampler::RINGSIZE - 1;

	for ( auto k = 0; k <= r.firN; ++k )
	{
		const auto	idx = ( r.sampleIndex - r.firN - 1 + k ) & ringMask;

		auto	v = int16_t ( r.sample[ idx ] );

		if constexpr ( ! A::isReader )
			if ( v != r.sample[ idx ] )
				a.fail ();

		a ( v );

		if constexpr ( A::isReader )
			r.sample[ idx ] = r.sample[ idx + reSIDfp::SincResampler::RINGSIZE ] = v;
	}
}
//-----------------------------------------------------------------------------

// The scan-mode statistics stay out: playback never restores mid-scan
template<class A>
void SaveState::io ( A& a, reSIDfp::DigiCapture& d )
{
	a ( d.lp1, d.lp2, d.dc, d.carminaLp );
}
//-----------------------------------------------------------------------------

}
