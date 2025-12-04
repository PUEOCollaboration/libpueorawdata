#ifndef _PUEO_RAWIO_VERSIONS_H
#define _PUEO_RAWIO_VERSIONS_H
/** old versions of types will go here */

#include "pueo/rawio.h"
#include "pueo/rawdata.h"
#include "pueo/pueo.h"

// make a prototype for pueo_X_size (ver)

#define X_SIZE(TYPE, STRUCT) \
int pueo_##STRUCT##_size(int ver);



PUEO_IO_DISPATCH_TABLE(X_SIZE)


#define CAT(a, ...) CAT_IMPL(a, __VA_ARGS__)
#define CAT_IMPL(a,...) a ## __VA_ARGS__


// These two macros just define pueo_X_vN_t as pueo_X_t, where N is the newest version

#define MAKE_CURRENT_VER_HELPER(STRUCT, VER) \
typedef pueo_##STRUCT##_t CAT(CAT(pueo_##STRUCT##_v,VER),_t);

#define X_CURRENT_VER(TYPE, STRUCT) \
  MAKE_CURRENT_VER_HELPER(STRUCT,TYPE##_VER)

PUEO_IO_DISPATCH_TABLE(X_CURRENT_VER)


// Here, we explicitly define any old versions (i.e. by copying the previous definition along with its version */


typedef pueo_sensors_telem_t pueo_sensors_telem_v0_t;
typedef pueo_sensors_disk_t pueo_sensors_disk_v0_t;
typedef struct pueo_slow_v0
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
  uint16_t L1_rates[12];
  uint8_t L2_rates[12];
} pueo_slow_v0_t;


typedef struct pueo_full_waveforms_v0
{
  uint32_t run;
  uint32_t event;
  uint32_t event_second;
  uint32_t event_time;
  uint32_t last_pps;
  uint32_t llast_pps;
  uint32_t trigger_meta[4];
  pueo_waveform_t wfs[PUEO_NCHAN];
} pueo_full_waveforms_v0_t;


typedef struct pueo_single_waveform_v0
{
  uint32_t run;
  uint32_t event;
  uint32_t event_second;
  uint32_t event_time;
  uint32_t last_pps;
  uint32_t llast_pps;
  uint32_t trigger_meta[4];
  pueo_waveform_t wf;
} pueo_single_waveform_v0_t;


typedef struct pueo_daq_hsk_v0
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

} pueo_daq_hsk_v0_t;



#endif















