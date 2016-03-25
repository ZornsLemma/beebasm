/*************************************************************************************************/
/**
	random.cpp

        Simple Lehmer random number generator; used instead of the native C library's generator 
        so RANDOMIZE+RND() gives consistent results on all platforms. The constants used are those
        for C++11's minstd_rand0. See https://en.wikipedia.org/wiki/Lehmer_random_number_generator

	Copyright (C) Rich Talbot-Watkins 2007 - 2012
        Copyright (C) Steven Flintham 2016

	This file is part of BeebAsm.

	BeebAsm is free software: you can redistribute it and/or modify it under the terms of the GNU
	General Public License as published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	BeebAsm is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
	even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along with BeebAsm, as
	COPYING.txt.  If not, see <http://www.gnu.org/licenses/>.
*/
/*************************************************************************************************/

#include "random.h"

static unsigned long state = 19670512;

static unsigned long modulus = 2147483647;

void beebasm_srand(unsigned long seed)
{
        state = seed % modulus;
        if ( state == 0 )
        {
                state = 1;
        }
}

unsigned long beebasm_rand()
{
        state = ( 48271 * state ) % modulus;
        return state;
}
