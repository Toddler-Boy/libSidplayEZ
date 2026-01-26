#pragma once
/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright (C) 2013-2014 Leandro Nini
 * Copyright (C) 2001 Dag Lem
 * Copyright (C) 1989-1997 André Fachat (a.fachat@physik.tu-chemnitz.de)
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

 /*
	 Modified by Dag Lem <resid@nimrod.no>
	 Relocate and extract text segment from memory buffer instead of file.
	 For use with VICE VSID.

	 Ported to c++ and cut down for use in libsidplayfp by Leandro Nini
 */

#include <cstdint>

 /**
  * reloc65 -- A part of xa65 - 65xx/65816 cross-assembler and utility suite
  * o65 file relocator. Trimmed down for our needs.
  */
class reloc65
{
private:
	const int	m_tbase;
	int			m_tdiff;

private:
	[[ nodiscard ]] int reldiff ( uint8_t s );

	/**
	 * Relocate segment.
	 *
	 * @param buf segment
	 * @param len segment size
	 * @param rtab relocation table
	 * @return a pointer to the next section
	 */
	[[ nodiscard ]] uint8_t* reloc_seg ( uint8_t* buf, int len, uint8_t* rtab );

	/**
	 * Relocate exported globals list.
	 *
	 * @param buf exported globals list
	 * @return a pointer to the next section
	 */
	uint8_t* reloc_globals ( uint8_t* buf );

public:
	/**
	 * @param addr address of the segment to relocate
	 */
	reloc65 ( int addr );

	/**
	 * Do the relocation.
	 *
	 * @param buf beffer containing o65 data
	 * @param fsize size of the data
	 */
	[[ nodiscard ]] bool reloc ( uint8_t** buf, int* fsize );
};
