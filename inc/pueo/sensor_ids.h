#ifndef _PUEO_SENSOR_IDS_H
#define _PUEO_SENSOR_IDS_H

/**
 * \file pueo/sensor_ids.h
 *
 * \brief Sensor indices for sensor files
 *
 * Max is 1024 since telemetry only has 10 bits here. If
 *
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


// for forward compatibility, we will add magic 16-bit val to all sensor packets.
// Should the sensor map change, this number will be bigger and we can define a compatibility shim
#define PUEO_SENSORS_CURRENT_MAGIC 0x2025

#define PUEO_SENSOR_ID_BITS 10

enum e_pueo_sensor_type_tag
{
  PUEO_SENSOR_FLOAT = 'F',
  PUEO_SENSOR_INT = 'I',
  PUEO_SENSOR_UINT = 'U',
};

enum  e_pueo_sensor_kind
{
  PUEO_SENSOR_VOLTS = 'V',
  PUEO_SENSOR_AMPS = 'A',
  PUEO_SENSOR_CELSIUS = 'C',
  PUEO_SENSOR_WATTS = 'W',
  PUEO_SENSOR_GAUSS = 'G',
  PUEO_SENSOR_BITMASK = 'X',
};



//WARNING WARNING WARNING
//Reordering elements will scramble everything. Once we start taking data 4realz, don't do it without changing the magic
//and building a compatibility shim. You can add stuff at the end though.
// This is an X macro
// Syntax is subsystem, name, typetag (U,I,F), kind (VACMB)
#define ALL_THE_PUEO_SENSORS(SENSOR)                     \
SENSOR(W1TEMPS,  ChargeContrHeatsink,       'F','C')     \
SENSOR(W1TEMPS,  PowerBoxBaseplate,         'F','C')     \
SENSOR(W1TEMPS,  24VVicor,                  'F','C')     \
SENSOR(W1TEMPS,  5VVicor,                   'F','C')     \
SENSOR(W1TEMPS,  4VVicor,                   'F','C')     \
SENSOR(W1TEMPS,  12V_AFlex,                 'F','C')     \
SENSOR(W1TEMPS,  12V_BFlex,                 'F','C')     \
SENSOR(W1TEMPS,  12V_CFlex,                 'F','C')     \
SENSOR(W1TEMPS,  StatRadPlateInt,           'F','C')     \
SENSOR(W1TEMPS,  SideofOutboardFPB,         'F','C')     \
SENSOR(W1TEMPS,  FrontofNavEncl,            'F','C')     \
SENSOR(W1TEMPS,  FrontofHDAQCrate,          'F','C')     \
SENSOR(W1TEMPS,  FrontofSciStack,           'F','C')     \
SENSOR(W1TEMPS,  DynRadPlateInt,            'F','C')     \
SENSOR(W1TEMPS,  TopofPowerFPB,             'F','C')     \
SENSOR(W1TEMPS,  FrontofCPUCrate,           'F','C')     \
SENSOR(W1TEMPS,  FrontofVDAQCrate,          'F','C')     \
SENSOR(W1TEMPS,  TopofPowerBox,             'F','C')     \
SENSOR(W1TEMPS,  PVActuator1,               'F','C')     \
SENSOR(W1TEMPS,  PVActuator2,               'F','C')     \
SENSOR(W1TEMPS,  LFActuator1,               'F','C')     \
SENSOR(W1TEMPS,  LFActuator2,               'F','C')     \
SENSOR(W1TEMPS,  StatRadPlateExt,           'F','C')     \
SENSOR(W1TEMPS,  DynRadPlateExt,            'F','C')     \
SENSOR(W1TEMPS,  MIELidExt,                 'F','C')     \
SENSOR(W1TEMPS,  BatteryBox1Face,           'F','C')     \
SENSOR(W1TEMPS,  BatteryBox2Face,           'F','C')     \
SENSOR(W1TEMPS,  SunSensorPhiSect7,         'F','C')     \
SENSOR(W1TEMPS,  SunSensorPhiSect19,        'F','C')     \
SENSOR(W1TEMPS,  LFOutwardofBoard4,         'F','C')     \
SENSOR(W1TEMPS,  LFInwardofBoard6,          'F','C')     \
SENSOR(W1TEMPS,  LFInwardofBoard3,          'F','C')     \
SENSOR(W1TEMPS,  LFPowerBoard,              'F','C')     \
SENSOR(W1TEMPS,  LFOutwardofBoard7,         'F','C')     \
SENSOR(W1TEMPS,  LFInwardofBoard8,          'F','C')     \
SENSOR(W1TEMPS,  LFOutwardofBoard1,         'F','C')     \
SENSOR(W1TEMPS,  AMPA_186,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_223,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_011,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_073,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_181,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_028,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_064,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_179,                  'F','C')     \
SENSOR(W1TEMPS,  UpperIMU,                  'F','C')     \
SENSOR(W1TEMPS,  StarlinkDishyMount,        'F','C')     \
SENSOR(W1TEMPS,  StarlinkPowerBox,          'F','C')     \
SENSOR(W1TEMPS,  ToqueCorner,               'F','C')     \
SENSOR(W1TEMPS,  AutoBelay,                 'F','C')     \
SENSOR(W1TEMPS,  AMPA_040,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_021,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_007,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_105,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_075,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_163,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_132,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_034,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_152,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_145,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_112,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_200,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_169,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_085,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_119,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_161,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_139,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_012,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_035,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_208,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_219,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_025,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_027,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_026,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_094,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_190,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_153,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_157,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_226,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_204,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_099,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_185,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_149,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_144,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_217,                  'F','C')     \
SENSOR(W1TEMPS,  AMPA_108,                  'F','C')     \
SENSOR(W1TEMPS,  PV1PhiSect,                'F','C')     \
SENSOR(W1TEMPS,  PV5PhiSect,                'F','C')     \
SENSOR(W1TEMPS,  PV4PhiSect,                'F','C')     \
SENSOR(W1TEMPS,  PV3PhiSect,                'F','C')     \
SENSOR(W1TEMPS,  PV7PhiSect,                'F','C')     \
SENSOR(W1TEMPS,  PV2PhiSect,                'F','C')     \
SENSOR(W1TEMPS,  PV6PhiSect,                'F','C')     \
SENSOR(W1TEMPS,  PV8PhiSect,                'F','C')     \
SENSOR(CPU,      Core10,                    'F','C')     \
SENSOR(CPU,      Core2,                     'F','C')     \
SENSOR(CPU,      Core9,                     'F','C')     \
SENSOR(CPU,      Core13,                    'F','C')     \
SENSOR(CPU,      Core12,                    'F','C')     \
SENSOR(CPU,      Core4,                     'F','C')     \
SENSOR(CPU,      Core5,                     'F','C')     \
SENSOR(CPU,      Core11,                    'F','C')     \
SENSOR(CPU,      Core1,                     'F','C')     \
SENSOR(CPU,      Core8,                     'F','C')     \
SENSOR(CPU,      Core3,                     'F','C')     \
SENSOR(CPU,      Core0,                     'F','C')     \
SENSOR(CPU,      Package0,                  'F','C')     \
SENSOR(CPU,      Ambient,                   'F','C')     \
SENSOR(CPU,      PCH,                       'F','C')     \
SENSOR(CPU,      LeftRail,                  'F','C')     \
SENSOR(CPU,      RightRail,                 'F','C')     \
SENSOR(CPU,      GPU,                       'F','C')     \
SENSOR(Mag,      B0,                        'F','G')     \
SENSOR(Mag,      B1,                        'F','G')     \
SENSOR(Mag,      B2,                        'F','G')     \
SENSOR(CC,       OutputPower,               'F','W')     \
SENSOR(CC,       InputPower,                'F','W')     \
SENSOR(CC,       MaxLastSweepPower,         'F','W')     \
SENSOR(CC,       PV,                        'F','V')     \
SENSOR(CC,       VmpLastSweep,              'F','V')     \
SENSOR(CC,       VocLastSweep,              'F','V')     \
SENSOR(CC,       FilteredBattVoltage,       'F','V')     \
SENSOR(CC,       BatteryMin,                'F','V')     \
SENSOR(CC,       BatteryMax,                'F','V')     \
SENSOR(CC,       HoursOn,                   'F','V')     \
SENSOR(CC,       FaultBits,                 'U','X')     \
SENSOR(CC,       ChargingCurrent,           'F','A')     \
SENSOR(CC,       TLocal,                    'F','c')     \
SENSOR(CC,       TRemote,                   'F','c')     \
SENSOR(HK,       24V,                       'F','V')     \
SENSOR(HK,       BATTERY,                   'F','V')     \
SENSOR(HK,       PV,                        'F','V')     \
SENSOR(HK,       12V_C_V,                   'F','V')     \
SENSOR(HK,       RF_OFF_INV,                'F','V')     \
SENSOR(HK,       12V_B_V,                   'F','V')     \
SENSOR(HK,       12V_A_V,                   'F','V')     \
SENSOR(HK,       12V_rail,                  'F','V')     \
SENSOR(HK,       3_3V_rail,                 'F','V')     \
SENSOR(HK,       12V_D_V,                   'F','V')     \
SENSOR(HK,       12V_C_I,                   'F','A')     \
SENSOR(HK,       12V_B_I,                   'F','A')     \
SENSOR(HK,       12V_A_I,                   'F','A')     \
SENSOR(HK,       5V_Vicor_V,                'F','V')     \
SENSOR(HK,       5V_Vicor_I,                'F','A')     \
SENSOR(HK,       4V_Vicor_V,                'F','V')     \
SENSOR(HK,       4V_Vicor_I,                'F','A')     \
SENSOR(RF,       RB1,                       'F','C')     \
SENSOR(RF,       RB2,                       'F','C')     \
SENSOR(RF,       RB3,                       'F','C')     \
SENSOR(RF,       RB4,                       'F','C')     \
SENSOR(RF,       RB5,                       'F','C')     \
SENSOR(RF,       RB6,                       'F','C')     \
SENSOR(DAQ,      V_TURF,                    'F','V')     \
SENSOR(DAQ,      I_TURF,                    'F','A')     \
SENSOR(DAQ,      T_RPU_TURF,                'F','C')     \
SENSOR(DAQ,      T_APU_TURF,                'F','C')     \
SENSOR(DAQ,      T_TURFIO_HL,               'F','C')     \
SENSOR(DAQ,      T_TURFIO_HR,               'F','C')     \
SENSOR(DAQ,      T_TURFIO_VL,               'F','C')     \
SENSOR(DAQ,      T_TURFIO_VR,               'F','C')     \
SENSOR(DAQ,      T_HS_SURF1_HL,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF2_HL,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF3_HL,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF4_HL,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF5_HL,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF6_HL,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF7_HL,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF1_HR,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF2_HR,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF3_HR,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF4_HR,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF5_HR,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF6_HR,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF7_HR,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF1_VL,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF2_VL,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF3_VL,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF4_VL,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF5_VL,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF6_VL,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF1_VR,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF2_VR,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF3_VR,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF4_VR,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF5_VR,             'F','C')     \
SENSOR(DAQ,      T_HS_SURF6_VR,             'F','C')     \
SENSOR(DAQ,      T_RPU_SURF1_HL,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF2_HL,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF3_HL,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF4_HL,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF5_HL,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF6_HL,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF7_HL,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF1_HR,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF2_HR,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF3_HR,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF4_HR,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF5_HR,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF6_HR,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF7_HR,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF1_VL,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF2_VL,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF3_VL,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF4_VL,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF5_VL,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF6_VL,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF1_VR,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF2_VR,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF3_VR,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF4_VR,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF5_VR,            'F','C')     \
SENSOR(DAQ,      T_RPU_SURF6_VR,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF1_HL,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF2_HL,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF3_HL,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF4_HL,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF5_HL,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF6_HL,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF7_HL,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF1_HR,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF2_HR,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF3_HR,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF4_HR,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF5_HR,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF6_HR,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF7_HR,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF1_VL,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF2_VL,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF3_VL,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF4_VL,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF5_VL,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF6_VL,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF1_VR,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF2_VR,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF3_VR,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF4_VR,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF5_VR,            'F','C')     \
SENSOR(DAQ,      T_APU_SURF6_VR,            'F','C')


#define PUEO_SENSOR_ENUM_DEF(SUBSYS,NAME,TYPETAG,KIND) PUEO_SENSOR_##SUBSYS##_##NAME,

enum e_pueo_sensors
{
  ALL_THE_PUEO_SENSORS(PUEO_SENSOR_ENUM_DEF)
};




#define PUEO_SID(SUBSYS,NAME) PUEO_SENSOR_##SUBSYS##_##NAME

const char * pueo_sensor_id_get_subsystem(uint16_t sensid);
const char * pueo_sensor_id_get_name(uint16_t sensid);
const char * pueo_sensor_id_get_subsystem_plus_name(uint16_t sensid);
char pueo_sensor_id_get_type_tag(uint16_t sensid);
char pueo_sensor_id_get_kind(uint16_t sensid);



#endif
