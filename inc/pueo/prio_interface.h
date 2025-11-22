#ifndef _PUEO_PRIO_INTERFACE_H
#define _PUEO_PRIO_INTERFACE_H

/** \file pueo/prio_interface.h
 *
 * \brief Prioritizer interface
 *
 * The prioritizer is implemented as a shared library that must implement
 * every method in the prioritizer.
 *
 * Every function in the X macro PUEO_PRIO_FUNCTIONS must be implemented. You
 * can autogenerate the declarations as
 *
 *  PUEO_PRIO_FUNCTIONS(PUEO_PRIO_DECLARE)
 *
 * This file is part of libpueorawdata, developed by the PUEO collaboration.
 * \copyright Copyright (C) 2021-2025 PUEO Collaboration
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <https://www.gnu.org/licenses/>.

 *
 */


#ifdef __cplusplus
extern "C"
{
#define PUEO_EXTERN extern "C"
#else
#define PUEO_EXTERN
#endif

#include <pueo/rawdata.h>
#include <stdint.h>



typedef struct pueo_prio_cfg_opt
{
  double noiseThreshold;
  double tdrsSignalThreshold;
  double extraSignalThreshold;
  double rmsThreshold;
  double topBlastThreshold;
  double bottomBlastThreshold;
  double fullBlastThreshold;
  double frontBackThreshold;
  double aboveHorizontal;

} pueo_prio_cfg_opt_t;

typedef struct pueo_prio_result
{
  float blast_params[3]; /// 0: number of antennas that beat rms threshold, 1: top pkpk/ bot pkpk ratio, 2: opp pkpk * opp pkpk / front pkpk ratio 
  float phi; /// runs from -55 deg to +430 deg
  float theta; /// runs from 20 deg above horizontal to -40 deg below horizontal
  float imp; //scRiseTime
  float map_peak; //skymap peak
  float peak_coherent; //Coherent Sum PKPK
  float pk2pk[192];
  float rms[192];
  pueo_priority_t priority;
} pueo_prio_result_t;

typedef struct pueo_prior_Event
{
  int trigL2;
  pueo_full_waveforms_t allwfms; //all waveforms as handed by CPU
} pueo_prior_Event_t;

#define PUEO_PRIO_MAX_BATCH 125


#define PUEO_PRIO_FUNCTIONS(FN)\
  FN(void, pueo_prio_init, (void))\
  FN(pueo_prio_cfg_opt_t, pueo_prio_configure, (void))\
  FN(pueo_prio_result_t*, pueo_prio_compute, (pueo_prior_Event_t * events, int N, pueo_prio_cfg_opt_t config))

#define PUEO_PRIO_DECLARE(ret, name, args)\
PUEO_EXTERN ret name args;


#define PUEO_PRIO_METHOD(ret, name, args) \
  ret (* name) args;

typedef struct pueo_prio_impl
{
  PUEO_PRIO_FUNCTIONS(PUEO_PRIO_METHOD)
} pueo_prio_impl_t;


int pueo_prio_create(const char * sopath, pueo_prio_impl_t * impl);


#ifdef __cplusplus
}
#endif



#endif
