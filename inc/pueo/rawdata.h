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
  PUEO_SS =0x501a,
  PUEO_SENSORS_TELEM =0xb1de,
  PUEO_SENSORS_DISK =0xcb1d,
  PUEO_CMD_ECHO = 0xecc0,
  PUEO_LOGS = 0x7a11,
  PUEO_SLOW = 0x510e,
  PUEO_TIMEMARK = 0xc10c
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


//more efficiently pack time into 64 bits
typedef struct pueo_time
{
  uint64_t utc_secs : 34;
  uint32_t utc_nsecs : 30;
} pueo_time_t;

_Static_assert(sizeof(pueo_time_t) == 8, "doh");



/** An individual waveform, This does not include run/event information and should only be used as part of another struct. Not serializable.*/
typedef struct pueo_waveform
{
  uint8_t channel_id; // wow, we are close to this limit aren't we?
  uint8_t flags;
  uint16_t length; // buffer length
  int16_t data[PUEO_MAX_BUFFER_LENGTH];
} pueo_waveform_t;



typedef struct pueo_full_waveforms
{
  uint32_t run;
  uint32_t event;
  uint32_t event_second;
  uint32_t event_time;
  uint32_t last_pps;
  uint32_t llast_pps;
  uint32_t trigger_meta[4];
  pueo_time_t readout_time;
  pueo_waveform_t wfs[PUEO_NCHAN];
} pueo_full_waveforms_t;

#define PUEO_FULL_WAVEFORMS_VER 1

typedef struct pueo_single_waveform
{
  uint32_t run;
  uint32_t event;
  uint32_t event_second;
  uint32_t event_time;
  uint32_t last_pps;
  uint32_t llast_pps;
  uint32_t trigger_meta[4];
  pueo_time_t readout_time;
  pueo_waveform_t wf;
} pueo_single_waveform_t;

#define PUEO_SINGLE_WAVEFORM_VER 1


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
  pueo_time_t readout_time;
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

#define PUEO_SENSORS_DISK_VER 1

typedef struct pueo_sensors_telem
{
  uint32_t timeref_secs; //UTC second reference time
  uint16_t sensor_id_magic;
  uint16_t num_packets; // Number of sensor packets here
  pueo_sensor_telem_t sensors[MAX_SENSORS_PER_PACKET_TELEM];
} pueo_sensors_telem_t;

#define PUEO_SENSORS_TELEM_VER 1


enum e_pueo_nav_src
{
  PUEO_NAV_ABX2 = 'A',
  PUEO_NAV_BOREAS = 'B',
  PUEO_NAV_CPT7 = 'C',
  PUEO_NAV_TURF = 'T',
};

enum e_pueo_nav_svtype
{
  PUEO_NAV_SV_GPSL1,
  PUEO_NAV_SV_GPSL2,
  PUEO_NAV_SV_GPSL5,
  PUEO_NAV_SV_GLOL1,
  PUEO_NAV_SV_GLOL2,
  PUEO_NAV_SV_GALE1,
  PUEO_NAV_SV_GALE5,
  PUEO_NAV_SV_BEID1,
  PUEO_NAV_SV_BEID2,
  PUEO_NAV_SV_QZSSL1,
  PUEO_NAV_SV_QZSSL2
};



typedef struct pueo_nav_att
{

  pueo_time_t readout_time;
  pueo_time_t gps_time;

  float lat;
  float lon;
  float alt;

  float heading;
  float pitch;
  float roll;
  float heading_sigma, pitch_sigma, roll_sigma;

  float v[3];
  float acc[3];

  float vdop;
  float hdop;

  char source;
  uint8_t nsats;
  uint8_t flags;

} pueo_nav_att_t;

#define  PUEO_NAV_ATT_VER 0


typedef struct pueo_nav_sat
{
  pueo_time_t readout_time;
  pueo_time_t gps_time;
  uint8_t nsats;
  char source;

  struct sat
  {
    uint8_t type : 4;
    uint8_t qualityInd : 4;
    uint8_t svid;
    _Float16 el;
    _Float16 az;
    _Float16 cno;
    _Float16 prRes;
  } sats[255]; //many won't be populated
} pueo_nav_sat_t;


typedef struct pueo_cmd_echo
{
  uint32_t when;
  uint32_t len_m1 : 8;
  uint32_t count : 24;
  uint8_t data[256];
} pueo_cmd_echo_t;



#define PUEO_CMD_ECHO_VER 0


typedef struct pueo_slow
{
  uint16_t ncmds;
  uint16_t time_since_last_cmd;
  uint32_t last_cmd : 8;
  uint32_t sipd_uptime : 24;
  uint32_t cpu_time;
  uint32_t cpu_uptime : 24;
  uint32_t can_ping_world : 1;
  uint32_t starlink_on : 1;
  uint32_t los_on : 1;
  uint16_t current_run;
  uint16_t current_run_secs;
  uint32_t current_run_events;
  uint16_t L1_rates[12][2];
  uint8_t L2_rates[12][2];
  struct
  {
    uint16_t index : 3; //by label, so 1-6, 0 means not present
    uint16_t free : 13; // in units of


  } pals[2];
  uint8_t palsA_index : 3 ;
  uint8_t palsA_present : 1;

} pueo_slow_t;

#define PUEO_SLOW_VER 1


#define PUEO_SS_NUM_SENSORS 8
typedef struct pueo_ss
{
  struct
  {
    // ok, this makes access somewhat more expensive but makes serialization easier
    unsigned __int128 x1 : 24;
    unsigned __int128 x2 : 24;
    unsigned __int128 y1 : 24;
    unsigned __int128 y2 : 24;
    unsigned __int128 tempADS1220 : 16;
    unsigned __int128 tempSS : 16;
  } ss[PUEO_SS_NUM_SENSORS];

  pueo_time_t readout_time;
  uint32_t sequence_number; //reset when pueo-ssd restarts
  uint32_t flags; //maybe we'll think of something but this is really just padding...
} pueo_ss_t;

#define PUEO_SS_TEMPERATURE_CONVERT(X) ( X * 500./65535 - 273.15)

//verify that the packing is correct
_Static_assert(sizeof(pueo_ss_t) == 16 * PUEO_SS_NUM_SENSORS + 16,"The sun exploded");

#define PUEO_SS_VER 0

typedef struct pueo_timemark
{
  pueo_time_t rising;
  pueo_time_t falling;
  uint16_t rise_count;
  uint8_t channel;
  uint8_t flags;
} pueo_timemark_t;


#define PUEO_TIMEMARK_VER 0



#ifdef __cplusplus
}
#endif

#endif
