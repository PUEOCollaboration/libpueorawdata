
/**
 * \file pueo/sensor_ids.c
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

 */


#include "pueo/sensor_ids.h"
#include <assert.h>

#define SENSOR_SUBSYS_DEF(SUBSYS,NAME,TT, K) #SUBSYS,
#define SENSOR_NAME_DEF(SUBSYS,NAME,TT, K) #NAME,
#define SENSOR_SUBSYSNAME_DEF(SUBSYS,NAME,TT, K) #SUBSYS"_"#NAME,
#define SENSOR_TT_DEF(SUBSYS,NAME,TT, K) TT,
#define SENSOR_K_DEF(SUBSYS,NAME,TT, K) K,


static const char * sensor_subsystems[] = {
  ALL_THE_PUEO_SENSORS(SENSOR_SUBSYS_DEF)
};

static const char * sensor_names[] = {
  ALL_THE_PUEO_SENSORS(SENSOR_NAME_DEF)
};

static const char * sensor_subsysnames[] = {
  ALL_THE_PUEO_SENSORS(SENSOR_SUBSYSNAME_DEF)
};



static const char  sensor_type_tags[] = {
  ALL_THE_PUEO_SENSORS(SENSOR_TT_DEF)
};

static const char  sensor_kinds[] = {
  ALL_THE_PUEO_SENSORS(SENSOR_K_DEF)
};

_Static_assert(sizeof(sensor_subsystems) == sizeof(sensor_names));
_Static_assert(sizeof(sensor_kinds) == sizeof(sensor_type_tags));
_Static_assert(sizeof(sensor_kinds)*sizeof(const char *) == sizeof(sensor_names));

#define MAX_SENSORS sizeof(sensor_kinds)

_Static_assert(MAX_SENSORS < (1 << PUEO_SENSOR_ID_BITS));



const char * pueo_sensor_id_get_subsystem(uint16_t sensid)
{
  assert(sensid < MAX_SENSORS);
  return sensor_subsystems[sensid];
}

const char * pueo_sensor_id_get_name(uint16_t sensid)
{
  assert(sensid < MAX_SENSORS);
  return sensor_names[sensid];
}

const char * pueo_sensor_id_get_subsystem_plus_name(uint16_t sensid)
{
  assert(sensid < MAX_SENSORS);
  return sensor_subsysnames[sensid];
}


char pueo_sensor_id_get_type_tag(uint16_t sensid)
{
  assert(sensid < MAX_SENSORS);
  return sensor_type_tags[sensid];
}
char pueo_sensor_id_get_kind(uint16_t sensid)
{
  assert(sensid < MAX_SENSORS);
  return sensor_kinds[sensid];
}



