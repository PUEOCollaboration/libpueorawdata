#ifndef _PUEO_RAW_DATA_H
#define _PUEO_RAW_DATA_H

/**
 * \file pueo/rawdata.h
 *
 * \brief RAW DATA FORMATS for PUEO
 *
 * These are the in-memory raw formats. Please pay attention to struct padding.
 *
 * All formats that can be written to disk should be added to the pueo_data_type_t enum, the PUEO_IO_DISPATCH_TABLE in rawio.h and should have an appropriately named _VER macro.
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
#include <pueo/sensor_ids.h>
#define __STDC_WANT_IEC_60559_TYPES_EXT__
#include <float.h>

#ifndef FLT16_MAX
#error Please use a compiler that supports FLOAT16 (e.g. gcc > 12)
#endif

#ifdef __cplusplus
extern "C"
{
#endif


/*This is the 2-byte magic for each type of serializable data*/

typedef enum e_pueo_datatype
{
  PUEO_PACKET_INVALID = 0,
  PUEO_PACKET_HEAD = 0x0ead , // if we write out headres, for some reason
  PUEO_FULL_WAVEFORMS = 0xda7a , //pueo_full_waveforms_t
  PUEO_SINGLE_WAVEFORM = 0xea7a, //pueo_single_waveform_t
  PUEO_ENCODED_WAVEFORM = 0xabcd, // pueo_encoded_waveform_t
  PUEO_NAV_ATT =0xa771,
  PUEO_SENSORS_TELEM =0xb1de,
  PUEO_SENSORS_DISK =0xcb1d,
  PUEO_CMD_ECHO = 0xecc0,
  PUEO_LOGS = 0x7a11
} pueo_datatype_t;

/**
 * pueo_packet_head_t
 *
 * This 8-byte header prefaces all PUEO data when written to disk or telemetry
 *
 *
 */
typedef struct pueo_packet_head
{
  uint16_t type; /// this tells us what type of packet we have
  uint16_t cksum ; /// A checksum!
  uint32_t f1 : 8; //  0xf1
  uint32_t version : 4;  /// The version of this packet type
  uint32_t num_bytes : 20; /// Number of bytes in this packet paylaod
} pueo_packet_head_t;



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


/* see pueo/encode.h to convert, once implemented*/
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


#define MAX_SENSORS_PER_PACKET_DISK 256
#define MAX_SENSORS_PER_PACKET_TELEM 256


// THESE ARE NOT WRITTEN OUT DIRECTLY
typedef struct pueo_sensor_telem
{
  uint16_t sensor_id  : 10;
  int16_t relsecs : 6;
  union
  {
    _Float16 fval;
    int16_t ival;
    uint16_t uval;
  } val;
} pueo_sensor_telem_t;



// THESE ARE NOT WRITTEN OUT DIRECTLY
typedef struct pueo_sensor_disk
{
  uint16_t sensor_id;
  uint16_t time_ms;  //millisecond part of time
  uint32_t time_secs; //second part of time
  union
  {
    float fval;
    int32_t ival;
    uint32_t uval;
  } val;
} pueo_sensor_disk_t;



typedef struct pueo_sensors_disk
{
  uint16_t num_packets; // Number of sensor packets here
  uint16_t sensor_id_magic;
  pueo_sensor_disk_t sensors[MAX_SENSORS_PER_PACKET_DISK];
} pueo_sensors_disk_t;

#define PUEO_SENSORS_DISK_VER 0

typedef struct pueo_sensors_telem
{
  uint32_t timeref_secs; //UTC second reference time
  uint16_t sensor_id_magic;
  uint16_t num_packets; // Number of sensor packets here
  pueo_sensor_telem_t sensors[MAX_SENSORS_PER_PACKET_TELEM];
} pueo_sensors_telem_t;

#define PUEO_SENSORS_TELEM_VER 0


typedef struct pueo_time
{
  uint64_t utc_secs : 34;
  uint32_t utc_nsecs : 30;
} pueo_time_t;



typedef struct pueo_nav_att
{
  enum
  {
    PUEO_NAV_CPT7,
    PUEO_NAV_BOREAS,
    PUEO_NAV_ABX2
  } source;

  pueo_time_t readout_time;
  pueo_time_t gps_time;

  float lat;
  float lon;
  float alt;

  float heading;
  float pitch;
  float roll;

  float v[3];
  float acc[3];

  float vdop;
  float hdop;

  uint8_t nsats;
  uint8_t flags;

} pueo_nav_att_t;

#define  PUEO_NAV_ATT_VER 0


typedef struct pueo_cmd_echo
{
  uint32_t when;
  uint32_t len_m1 : 8;
  uint32_t count : 24;
  uint8_t data[256];
} pueo_cmd_echo_t;


#define PUEO_CMD_ECHO_VER 0

#ifdef __cplusplus
}
#endif

#endif
