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
#define PUEO_NQRH_ANT  108 
#define PUEO_NLF_ANT 8 
#define PUEO_NANT (PUEO_NQRH_ANT + PUEO_NLF_ANT) 


#define PUEO_NSURF 28 
#define PUEO_NCHAN_PER_SURF 8
#define PUEO_NCHAN (PUEO_NSURF * PUEO_NCHAN_PER_SURF) 


#define PUEO_MAIN_SAMPLE_RATE 3.0
#define PUEO_MAX_BUFFER_LENGTH 2048 
#define PUEO_LF_SAMPLE_RATE 0.5 

#else 
#error unknown PUEO_VERSION
#endif

//pueo version 0 

#ifdef __cplusplus
}
#endif 

#endif 
