#ifndef _PUEO_H
#define _PUEO_H

/** \file pueo/pueo.h
 *
 * \brief Global PUEO defines (that affect storage structs)
 *
 * This is influenced by the compile-time variable PUEO_VERSION, which will
 * default to the latest version if not defined.
 *
 *
 * This file is designed to be read by both flight and ground software, so if things are
 * only necessary for one or the other, they should go elsewhere.
 *
 * This file is part of libpueorawdata, developed by the PUEO collaboration.
 * \copyright Copyright (C) 2021 PUEO Collaboration
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 **/


#ifdef __cplusplus
extern "C"
{
#endif


/**
 *  PUEO-wide constants
 *
 */


/** Allow overriding of pueo version */
#ifndef PUEO_VERSION
#define PUEO_VERSION 0
#endif




#if PUEO_VERSION == 0

#define PUEO_V0

/** Number of quad-ridge horn antennas */
#define PUEO_NQRH_ANT  96
#define PUEO_NLF_ANT 8
#define PUEO_NANT (PUEO_NQRH_ANT + PUEO_NLF_ANT)

//2 are virtual, will throw them out later
#define PUEO_NSURF 28
#define PUEO_NCHAN_PER_SURF 8
// how many surfs are actually populated
#define PUEO_NREALSURF 26

#define PUEO_NMISURF 24

//convert from real surf index to daq surf index
#define PUEO_IREALSURF_TO_ISURF(i)   ( i < 20 ? i : i+1 )

//convert from MI surf to daq surf index
#define PUEO_IMISURF_TO_ISURF(i)   ( i < 6  ? i :\
                                     i < 12 ? i+1:\
                                     i < 18 ? i+2:\
                                     i+3)
#define PUEO_SURF_LINK(i)   ( i / 7)
#define PUEO_SURF_SLOT(i)   ( i % 7)

#define PUEO_IREALSURF_LINK(i)   ( PUEO_SURF_LINK(PUEO_IREALSURF_TO_ISURF(i)))
#define PUEO_IREALSURF_SLOT(i)   ( PUEO_SURF_SLOT(PUEO_IREALSURF_TO_ISURF(i)))

#define PUEO_IMISURF_LINK(i)   ( PUEO_SURF_LINK(PUEO_IMISURF_TO_ISURF(i)))
#define PUEO_IMISURF_SLOT(i)   ( PUEO_SURF_SLOT(PUEO_IMISURF_TO_ISURF(i)))

#define PUEO_NCHAN (PUEO_NSURF * PUEO_NCHAN_PER_SURF)

#define PUEO_NBEAMS 48

#define PUEO_MAIN_SAMPLE_RATE 3.0
#define PUEO_MAX_BUFFER_LENGTH 1024
#define PUEO_LF_SAMPLE_RATE 3.0

#else
#error unknown PUEO_VERSION
#endif

//pueo version 0

#ifdef __cplusplus
}
#endif

#endif
