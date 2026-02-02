#pragma once
/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2023 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2004,2010 Dag Lem
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <cmath>
#include <memory>
#include <vector>

#include "../../EZ/config.h"

#include "Spline.h"

namespace reSIDfp
{

/**
 * Find output voltage in inverting gain and inverting summer SID op-amp
 * circuits, using a combination of Newton-Raphson and bisection.
 *
 *               +---R2--+
 *               |       |
 *     vi ---R1--o--[A>--o-- vo
 *               vx
 *
 * From Kirchoff's current law it follows that
 *
 *     IR1f + IR2r = 0
 *
 * Substituting the triode mode transistor model K*W/L*(Vgst^2 - Vgdt^2)
 * for the currents, we get:
 *
 *     n*((Vddt - vx)^2 - (Vddt - vi)^2) + (Vddt - vx)^2 - (Vddt - vo)^2 = 0
 * 
 * where n is the ratio between R1 and R2.
 *
 * Our root function f can thus be written as:
 *
 *     f = (n + 1)*(Vddt - vx)^2 - n*(Vddt - vi)^2 - (Vddt - vo)^2 = 0
 *
 * Using substitution constants
 *
 *     a = n + 1
 *     b = Vddt
 *     c = n*(Vddt - vi)^2
 *
 * the equations for the root function and its derivative can be written as:
 *
 *     f = a*(b - vx)^2 - c - (b - vo)^2
 *     df = 2*((b - vo)*dvo - a*(b - vx))
 */
class OpAmp final
{
private:
    /// Current root position (cached as guess to speed up next iteration)
    double  x = 0.0;

    const double Vddt;
    const double vmin;
    const double vmax;

    std::unique_ptr<Spline> const opamp;

public:
    /**
     * Opamp input -> output voltage conversion
     *
     * @param opamp opamp mapping table as pairs of points (in -> out)
     * @param Vddt transistor dt parameter (in volts)
     * @param vmin
     * @param vmax
     */
	OpAmp ( const std::vector<Spline::Point>& opamp, double Vddt, double vmin, double vmax )
		: Vddt ( Vddt )
		, vmin ( vmin )
		, vmax ( vmax )
		, opamp ( new Spline ( opamp ) )
    {
    }

    /**
     * Reset root position
     */
	void reset () noexcept
    {
        x = vmin;
    }

    /**
     * Solve the opamp equation for input vi in loading context n
     *
     * @param n the ratio of input/output loading
     * @param vi input voltage
     * @return vo output voltage
     */
	sidinline double solve ( double n, double vi ) noexcept
	{
		constexpr auto	EPSILON = 1e-8;

		// Start off with an estimate of x and a root bracket [ak, bk].
			// f is decreasing, so that f(ak) > 0 and f(bk) < 0.
		auto    ak = vmin;
		auto    bk = vmax;

		const auto  a = n + 1.0;
		const auto  b = Vddt;
		const auto  b_vi = ( b > vi ) ? ( b - vi ) : 0.0;
		const auto  c = n * ( b_vi * b_vi );

		for ( ;;)
		{
			const auto  xk = x;

			// Calculate f and df
			const auto	out = opamp->evaluate ( x );
			const auto  vo = out.x;
			const auto  dvo = out.y;

			const auto  b_vx = ( b > x ) ? b - x : 0.;
			const auto  b_vo = ( b > vo ) ? b - vo : 0.;

			// f = a*(b - vx)^2 - c - (b - vo)^2
			const auto	f = a * ( b_vx * b_vx ) - c - ( b_vo * b_vo );

			// df = 2*((b - vo)*dvo - a*(b - vx))
			const auto	df = 2.0 * ( b_vo * dvo - a * b_vx );

			// Newton-Raphson step: xk1 = xk - f(xk)/f'(xk)
			x -= f / df;

			if ( std::fabs ( x - xk ) < EPSILON ) [[ unlikely ]]
				return opamp->evaluate ( x ).x;

			// Narrow down root bracket
 			if ( f < 0.0 )
 				bk = xk;
 			else
 				ak = xk;

			// Bisection step (ala Dekker's method)
			if ( x <= ak || x >= bk ) [[ unlikely ]]
				x = ( ak + bk ) * 0.5;
		}
	}
};

} // namespace reSIDfp
