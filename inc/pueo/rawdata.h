#ifndef _PUEO_RAW_DATA_H
#define _PUEO_RAW_DATA_H

/** 
 * \file pueo/rawdata.h 
 *
 * \brief RAW DATA FORMATS for PUEO 
 *
 * These are the in-memory raw formats. Please pay attention to struct padding. 
 *
 * All formats that can be written to disk should be added to the pueo_data_type_t enum, the PUEO_IO_DISPATCH_TABLE in pueoio.h and should have an appropriately named _VER macro.
 *
 *
 * If you want to update a struct and we already have data written to disk that we still want to read out in the future, you must consider the versioning.
 * Each struct gets a version written with it. When updating the version number, put the old struct inside versions.h as pueo_X_vN_t; 
 * You may also need to update pueo_read_packet_X in rawio_packets.c if it cannot be or is not currently handled automatigally  (see rawio_packets.c for details on what that means). 
 * Otherwise, you'll get failures on read. 
 * 
 * Note also that the version should be incremented by 1, not by larger values, and should the version number increase by too much, you may need to updated the magic macros in pueo_versions.c 
 *
 * pueo/pueo.h for constants  etc. 
 * See pueo/rawio.h for reading/writing this data. 
 *
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
 */ 

#include <stdint.h> 
#include <pueo/pueo.h> 

#ifdef __cplusplus
extern "C"
{
#endif


/*This is the 4-byte magic for each type of serializable data*/ 
typedef enum 
{
  PUEO_PACKET_INVALID = 0,
  PUEO_PACKET_HEAD = 0xead,  /// pueo_packet_head_t 
  PUEO_FULL_WAVEFORMS = 0xea717a11 , //pueo_full_waveforms_t
  PUEO_SINGLE_WAVEFORM = 0xea7ab17 , //pueo_single_waveform_t
  PUEO_ENCODED_WAVEFORM = 0xc0fefe, // pueo_encoded_waveform_t

} pueo_datatype_t;  

/** 
 * pueo_packet_head_t 
 *
 * This 16-byte header prefaces all PUEO data when written to disk. 
 * 
 *
 */
typedef struct pueo_packet_head
{
  pueo_datatype_t type; /// this tells us what type of packet we have
  uint32_t packet_number; /// The packet number in this stream (mostly useful when splitting up data into multiple packets)
  uint16_t num_bytes; /// Number of bytes in this packet paylaod
  uint8_t  version;  /// The version of this packet type
  uint8_t  reserved;  /// should equal 0xfe 
  uint32_t cksum ; /// A checksum! 
} pueo_packet_head_t; 

#define PUEO_PACKET_HEAD_VER 0 


/** An individual waveform, This does not include run/event information and should only be used as part of another struct. Not serializable.*/ 
typedef struct pueo_waveform
{
  uint8_t channel_id; // wow, we are close to this limit aren't we? 
  uint8_t flags; 
  uint16_t length; // buffer length
  uint16_t data[PUEO_MAX_BUFFER_LENGTH]; 
} pueo_waveform_t; 



typedef struct pueo_full_waveforms 
{
  uint32_t run; 
  uint32_t event; 
  pueo_waveform_t wfs[PUEO_NCHAN]; 
} pueo_full_waveforms_t; 

#define PUEO_FULL_WAVEFORMS_VER 0 

typedef struct pueo_single_waveform 
{
  uint32_t run; 
  uint32_t event; 
  pueo_waveform_t wf; 
} pueo_single_waveform_t; 

#define PUEO_SINGLE_WAVEFORM_VER 0


/* see pueo/encode.h to convert*/ 
typedef struct pueo_encoded_waveform 
{
  uint32_t run; 
  uint32_t event; 
  uint8_t channel_id;
  uint8_t flags; 
  uint16_t nsamples; 
  uint16_t encoded_flags; 
  uint16_t encoded_nbytes; 
  uint8_t encoded[2 * PUEO_MAX_BUFFER_LENGTH]; 
} pueo_encoded_waveform_t; 

#define PUEO_ENCODED_WAVEFORM_VER 0 





#ifdef __cplusplus
}
#endif 

#endif
