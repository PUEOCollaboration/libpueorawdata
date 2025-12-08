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
#ifndef __STDC_WANT_IEC_60559_TYPES_EXT__
#define __STDC_WANT_IEC_60559_TYPES_EXT__
#endif
#include <float.h>
#ifdef __cplusplus
extern "C"
{
#endif


#ifndef FLT16_MAX
#warning Using a compiler with FLOAT16 support, attempting to use some structs will result in errors
typedef struct fake_Float16_will_probably_cause_errors
{
  uint16_t sign :1;
  uint16_t exponent :5;
  uint16_t fraction :10;
} FakeFloat16;
#define float16 FakeFloat16
#else
#define float16 _Float16
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
  PUEO_NAV_SAT =0x5a75,
  PUEO_NAV_POS =0x1a75,
  PUEO_DAQ_HSK =0xdacc,
  PUEO_DAQ_HSK_SUMMARY =0xdac5,
  PUEO_SS =0x501a,
  PUEO_SENSORS_TELEM =0xb1de,
  PUEO_SENSORS_DISK =0xcb1d,
  PUEO_CMD_ECHO = 0xecc0,
  PUEO_LOGS = 0x7a11,
  PUEO_FILE_DOWNLOAD = 0xd1d1,
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

#ifndef __cplusplus
_Static_assert(sizeof(pueo_time_t) == 8, "doh");
#endif



/** An individual waveform, This does not include run/event information and should only be used as part of another struct. Not serializable.*/
typedef struct pueo_waveform
{
  uint8_t channel_id; // wow, we are close to this limit aren't we?
  uint8_t surf_word; //   L1 word for this surf
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
  uint32_t deadtime_counter;
  uint32_t deadtime_counter_last_pps;
  uint32_t deadtime_counter_llast_pps;
  uint32_t L2_mask : 24;
  uint32_t soft_trigger : 1;
  uint32_t pps_trigger : 1;
  uint32_t ext_trigger : 1;
  uint32_t reserved : 5;
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
  uint32_t deadtime_counter;
  uint32_t deadtime_counter_last_pps;
  uint32_t deadtime_counter_llast_pps;
  uint32_t L2_mask : 24;
  uint32_t soft_trigger : 1;
  uint32_t pps_trigger : 1;
  uint32_t ext_trigger : 1;
  uint32_t reserved : 5;

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
    float16 fval;
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

  float vdop;
  float hdop;

  char source;
  uint8_t nsats;
  uint8_t flags;

  uint16_t antenna_currents[3];
  int16_t temperature;
} pueo_nav_att_t;

#define  PUEO_NAV_ATT_VER 0


typedef struct pueo_nav_sat
{
  pueo_time_t readout_time;
  pueo_time_t gps_time;
  uint8_t nsats_used;
  uint8_t nsats_visible;
  char source;

  struct sat
  {
    uint8_t used : 1;
    uint8_t type : 3;
    uint8_t qualityInd : 4;
    uint8_t svid;
    float16 el;
    float16 az;
    float16 cno;
    float16 prRes;
  } sats[255]; //many won't be populated
} pueo_nav_sat_t;

#define  PUEO_NAV_SAT_VER 0

typedef struct pueo_nav_pos
{

  pueo_time_t readout_time;
  pueo_time_t gps_time;

  float lat;
  float lon;
  float alt;

  float x[3];
  float v[3];

  float vdop;
  float hdop;

  char source;
  uint8_t nsats;
  uint8_t flags;

} pueo_nav_pos_t;

#define  PUEO_NAV_POS_VER 0

typedef struct pueo_cmd_echo
{
  uint32_t when;
  uint32_t len_m1 : 8;
  uint32_t count : 24;
  uint8_t data[256];
} pueo_cmd_echo_t;



#define PUEO_CMD_ECHO_VER 0

#define PUEO_NUM_L2 12
typedef struct pueo_slow
{
  // 0
  uint16_t ncmds;  //number of commands received by THIS instance of sipd
  uint16_t time_since_last_cmd;  // saturating time since last command
  //4
  uint32_t last_cmd : 8;  // what was th elast command?
  uint32_t sipd_uptime : 24;  //uptime of sipd
  //8
  uint32_t cpu_time;  // the time the CPU believes it is
  //12
  uint32_t cpu_uptime : 24;  // the uptime of the CPU
  uint32_t can_ping_world : 1;  // Are we able to ping the world?
  uint32_t starlink_on : 1;     // Is Starlink on?
  uint32_t los_on : 1;          // is LOS on?
  uint32_t gpu_present : 1;     // Is the GPU present
  uint32_t nic_present : 1;    // Is the NIC present
  uint32_t turf_seen : 1;      // Do we see the TURF?
  uint32_t hsk_seen : 1;
  uint32_t ss_seen : 1;
  //16
  uint16_t current_run;        // What is the current run?
  uint16_t current_run_secs;   // How long has the run been going for
  //20
  uint32_t current_run_events : 24; // How many event shave been written
  uint32_t acqd_running : 1;
  uint32_t prioritizerd_running : 1;

  //24
  uint32_t current_run_rf_events : 24; // How many were RF events?

  //28
  uint32_t hsk_uptime : 24;

  //32
  uint16_t L2_rates[PUEO_NUM_L2][2];

  //80


  struct
  {
    uint16_t index : 3; //by label, so 1-6, 0 means not present
    uint16_t free : 13; // in units of 2.5 GiB
  } pals[2];
  //84

  struct
  {
    uint64_t ssd0_free : 12;  //4095 means not there
    uint64_t ssd1_free : 12;  //4095 means not there
    uint64_t ssd2_free : 12;  //4095 means not there
    uint64_t ssd3_free : 12;  //4095 means not there
    uint64_t ssd4_free : 12;  //4095 means not there
  } ssd;
  //92

  struct
  {
    uint32_t heading_abx : 9; // 512 means not present
    uint32_t heading_boreas : 9; // 512 means not present
    uint32_t heading_cpt7 : 9; // 512 means not present
    uint32_t turf_fix_type : 2;
  } nav;
  //96

  int8_t NIC_temperature;
  int8_t SFC_temperature; 

  uint32_t pwr_from_sun    : 11;
  uint32_t pwr_usage       : 11;
  uint32_t battery_state   : 3;
  uint32_t AMPA_current    : 4;
  uint32_t VPol_Crate_on   : 1;
  uint32_t HPol_Crate_on   : 1;
  uint32_t SFC_Voltage_on  : 1; // this was is redudant because how the hell do you get this packet if the SFC is off??

  uint8_t  VPol_Current; // range 0,20 in real units (in data taking mode it is railed at 20 at the sensor lvl by design flaw)
  uint8_t  HPol_Current; // range 0,20 in real units (in data taking mode it is railed at 20 at sensor lvl by design flaw)
  uint8_t  SFC_Current; // 0,20 in real units (usually sits at ~4.5A , but includes current of GPU which goes to ~8A if prioritizer is running)
  
} pueo_slow_t;

#define PUEO_SLOW_VER 2


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

#define PUEO_SS_TEMPERATURE_CONVERT_ADS1220(X) ( (X) * 0.03125)
#define PUEO_SS_TEMPERATURE_CONVERT_SS(X) ( (X)  / 1024. * 5 * 100  - 273.15)

//verify that the packing is correct
#ifndef __cplusplus
_Static_assert(sizeof(pueo_ss_t) == 16 * PUEO_SS_NUM_SENSORS + 16,"The sun exploded");
#endif

#define PUEO_SS_VER 0

typedef struct pueo_timemark
{
  pueo_time_t readout_time;
  pueo_time_t rising;
  pueo_time_t falling;
  uint16_t rise_count;
  uint8_t channel;
  uint8_t flags;
} pueo_timemark_t;


#define PUEO_TIMEMARK_VER 0



typedef struct pueo_logs
{
  uint32_t utc_retrieved : 31;
  uint32_t is_until  :1;
  uint16_t rel_time_since_or_until;
  uint8_t daemon_len;
  uint8_t grep_len;
  uint16_t msg_len;
  char buf[2048];
} pueo_logs_t;

#define PUEO_LOGS_VER 0


#define PUEO_PRIO_TRIG_TYPE_FORCE 0
#define PUEO_PRIO_TRIG_TYPE_LF 1
#define PUEO_PRIO_TRIG_TYPE_MI 2
#define PUEO_PRIO_TRIG_TYPE_LFANDMI 3

#define PUEO_PRIO_CAL_TYPE_NONE 0
#define PUEO_PRIO_CAL_TYPE_HICAL 1
#define PUEO_PRIO_CAL_TYPE_GROUNDCAL 2
#define PUEO_PRIO_CAL_TYPE_RESERVED 3

#define PUEO_PRIO_SIGNAL_LEVEL_THERMAL 0
#define PUEO_PRIO_SIGNAL_LEVEL_SIGNAL 1
#define PUEO_PRIO_SIGNAL_LEVEL_BEST_SIGNAL 2
#define PUEO_PRIO_SIGNAL_LEVEL_BESTEST_SIGNAL 3

typedef struct pueo_priority
{
  uint16_t trig_type : 2;
  uint16_t topring_blast_flag : 1; //set by prioritizer
  uint16_t botring_blast_flag : 1; //set by prioritizer
  uint16_t fullpayload_blast_flag : 1; //set by prioritizer
  uint16_t frontback_blast_flag : 1; //set by prioritizer
  uint16_t anthro_base1_flag : 1;
  uint16_t anthro_base2_flag : 1;
  uint16_t anthro_base3_flag : 1;
  uint16_t anthro_base4_flag : 1;
  uint16_t anthro_base5_flag : 1;
  uint16_t anthro_base6_flag : 1;
  uint16_t cal_type : 2;
  uint16_t signal_level : 2; //set by prioritizer
} pueo_priority_t;

typedef struct pueo_daq_hsk_summary
{
  struct
  {
    struct
    {
      uint32_t thresh_avg : 15;
      uint32_t scaler_avg : 12;
      uint32_t scaler_rms_div_16 : 5;
    } beams[PUEO_NBEAMS];
  } surf[PUEO_NREALSURF];

  uint8_t Hscalers_avg[12];  //saturate at 255
  uint8_t Vscalers_avg[12];

  uint64_t MIE_total_H_avg : 9;
  uint64_t MIE_total_V_avg : 9;
  uint64_t aux_total_avg : 6;
  uint64_t pps_rate : 2;
  uint64_t global_total_avg : 10;
  uint64_t global_total_min : 10;
  uint64_t global_total_max : 10;
  uint64_t global_total_rms : 8;

  uint32_t start_second;
  uint32_t end_second;
} pueo_daq_hsk_summary_t;

#define PUEO_DAQ_HSK_SUMMARY_VER 0


typedef struct pueo_daq_hsk
{
  struct
  {
    struct
    {
      uint64_t threshold : 18;
      uint64_t pseudothreshold : 18;
      uint64_t scaler : 12;
      uint64_t pseudoscaler : 12;
      uint64_t in_mask : 1;
      uint64_t scaler_bank : 1;
    } beams[PUEO_NBEAMS];

    uint32_t agc_scale[PUEO_NCHAN_PER_SURF];
    uint16_t agc_offset[PUEO_NCHAN_PER_SURF];

    pueo_time_t readout_time_start;
    uint16_t ms_elapsed;
    uint8_t surf_link;
    uint8_t surf_slot;
  } surfs[PUEO_NSURF];  // note, that some are probably garbage

  pueo_time_t l2_readout_time;
  uint32_t Hscalers[12];
  uint32_t Vscalers[12];

  pueo_time_t scaler_readout_time;;
  uint32_t turfio_L1_rate[4][7];
  uint32_t soft_rate;
  uint32_t pps_rate;
  uint32_t ext_rate;

  uint16_t MIE_total_H;
  uint16_t MIE_total_V;
  uint16_t LF_total_H;
  uint16_t LF_total_V;
  uint16_t aux_total;
  uint16_t global_total;

} pueo_daq_hsk_t;


#define PUEO_DAQ_HSK_VER 1


//plenty of headroom here!
#define PUEO_MAX_FILE_DOWNLOAD_LENGTH 65000

typedef struct pueo_file_download
{
  uint32_t offset : 31;
  uint32_t read_ok : 1; // did we read it fine? (to disambiguate between empty and non-existent)
  uint32_t mtime : 31;
  uint32_t binary :1;
  char fname[30]; //the front may be truncated
  uint16_t len; //check with pahole if this aligned ok
  uint8_t bytes[PUEO_MAX_FILE_DOWNLOAD_LENGTH];
} pueo_file_download_t;


#define PUEO_FILE_DOWNLOAD_VER 0

#ifdef __cplusplus
}
#endif

#endif
