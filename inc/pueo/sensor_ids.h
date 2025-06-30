#ifndef _PUEO_RAW_DATA_H
#define _PUEO_RAW_DATA_H

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

enum
{
  PUEO_SENSOR_FLOAT = 'F',
  PUEO_SENSOR_INT = 'I',
  PUEO_SENSOR_UINT = 'U',
} e_pueo_sensor_type_tag;

enum
{
  PUEO_SENSOR_VOLTS = 'V',
  PUEO_SENSOR_AMPS = 'A',
  PUEO_SENSOR_CELSIUS = 'C',
  PUEO_SENSOR_MICROTESLA = 'M',
  PUEO_SENSOR_BITMASK = 'B',
} e_pueo_sensor_kind;



//WARNING WARNING WARNING
//Reordering elements will scramble everything. Once we start taking data 4realz, don't do it without changing the magic
//and building a compatibility shim. You can add stuff at the end though.
// This is an X macro
// Syntax is subsystem, name, typetag (U,I,F), kind (VACMB)
#define ALL_THE_PUEO_SENSORS(SENSOR)        \
  SENSOR(HK,    CCLocalTemp,    'F', 'C')   \
  SENSOR(HK,    CCLocalRemote,  'F', 'C')   \
  SENSOR(HK,    CCBattVoltage,  'F', 'V')   \
  SENSOR(HK,    CCHours,        'F', 'V')




#define PUEO_SENSOR_ENUM_DEF(SUBSYS,NAME,TYPETAG,KIND) PUEO_SENSOR_##SUBSYS##_##NAME,

enum
{
  ALL_THE_PUEO_SENSORS(PUEO_SENSOR_ENUM_DEF)
} e_pueo_sensors;



#define PUEO_SENSOR_ID(subsys,name) PUEO_SENSOR_##SUBSYS##_##NAME

const char * pueo_sensor_id_get_subsystem(uint16_t sensid);
const char * pueo_sensor_id_get_name(uint16_t sensid);
char pueo_sensor_id_get_type_tag(uint16_t sensid);
char pueo_sensor_id_get_kind(uint16_t sensid);




#endif
