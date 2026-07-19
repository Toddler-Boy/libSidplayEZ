#pragma once

#if defined(_MSC_VER) && !defined(__clang__) && !defined(__INTEL_COMPILER)
	#define sidinline __forceinline
#else
	#define sidinline inline __attribute__((always_inline))
#endif
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
	#define SID_X86 1
	#if defined(_MSC_VER)
		#include <intrin.h>		// __cpuid / __cpuidex / _xgetbv (MSVC and clang-cl)
	#else
		#include <cpuid.h>
	#endif
#elif defined(__aarch64__) && defined(__ARM_NEON)
	#define SID_NEON 1
#endif

namespace libsidplayEZ
{
	// Highest SIMD level usable at runtime (CPU support AND OS state saving).
	// Ordered, so kernels can test with e.g. `simdLevel () >= SIMD_AVX2`.
	enum SimdLevel : int
	{
		SIMD_SSE42,		// x86 build baseline (-msse4.2)
		SIMD_AVX,
		SIMD_AVX2,		// only reported together with FMA
		SIMD_NEON,		// aarch64 baseline
	};

#if SID_X86
	inline SimdLevel detectSimdLevel ()
	{
		int	r[ 4 ];

	#if defined(_MSC_VER)
		__cpuid ( r, 0 );
		const auto	maxLeaf = r[ 0 ];
		__cpuid ( r, 1 );
	#else
		__get_cpuid ( 0, (unsigned*)&r[ 0 ], (unsigned*)&r[ 1 ], (unsigned*)&r[ 2 ], (unsigned*)&r[ 3 ] );
		const auto	maxLeaf = r[ 0 ];
		__get_cpuid ( 1, (unsigned*)&r[ 0 ], (unsigned*)&r[ 1 ], (unsigned*)&r[ 2 ], (unsigned*)&r[ 3 ] );
	#endif

		const auto	ecx1 = r[ 2 ];
		const auto	fma = ( ecx1 & ( 1 << 12 ) ) != 0;
		const auto	avx = ( ecx1 & ( 1 << 28 ) ) != 0;
		const auto	osxsave = ( ecx1 & ( 1 << 27 ) ) != 0;

		// AVX+ also needs the OS to save the wider registers on context switch
		unsigned long long	xcr0 = 0;
		if ( osxsave )
		{
		#if defined(_MSC_VER)
			xcr0 = _xgetbv ( 0 );
		#else
			// the builtin needs -mxsave at compile time, so use the raw instruction
			unsigned	lo, hi;
			__asm__ __volatile__ ( "xgetbv" : "=a"( lo ), "=d"( hi ) : "c"( 0 ) );
			xcr0 = ( (unsigned long long)hi << 32 ) | lo;
		#endif
		}
		const auto	ymmOS = ( xcr0 & 0x06 ) == 0x06;			// XMM + YMM state

		auto	avx2 = false;
		if ( maxLeaf >= 7 )
		{
		#if defined(_MSC_VER)
			__cpuidex ( r, 7, 0 );
		#else
			__get_cpuid_count ( 7, 0, (unsigned*)&r[ 0 ], (unsigned*)&r[ 1 ], (unsigned*)&r[ 2 ], (unsigned*)&r[ 3 ] );
		#endif
			avx2 = ( r[ 1 ] & ( 1 << 5 ) ) != 0;
		}

		if ( avx2 && fma && ymmOS )		return SIMD_AVX2;
		if ( avx && ymmOS )				return SIMD_AVX;
		return SIMD_SSE42;
	}
#else
	inline SimdLevel detectSimdLevel () { return SIMD_NEON; }
#endif

	// Cached once, cheap to call from every constructor
	inline SimdLevel simdLevel ()
	{
		static const auto	level = detectSimdLevel ();
		return level;
	}
}
