#ifndef PUEO_RAWIO_PACKETS_H
#define PUEO_RAWIO_PACKETS_H

/** \file rawio_packets.h
 *
 * private internal header for io packets. Not exported!
 *
 * This file is part of libpueorawdata, developed by the PUEO collaboration.
 * \copyright Copyright (C) 2021 PUEO Collaboration
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
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <https://www.gnu.org/licenses/>.

 *
 */


#include <pueo/rawio.h>


// Set up write packet method for each type
#define X_PUEO_WRITE_PACKET(IGNORE,STRUCT_NAME) \
  int pueo_write_packet_##STRUCT_NAME(pueo_handle_t *h, const pueo_##STRUCT_NAME##_t * p); \

// Set up read packet method for each type
#define X_PUEO_READ_PACKET(IGNORE,STRUCT_NAME) \
  int pueo_read_packet_##STRUCT_NAME(pueo_handle_t *h, pueo_##STRUCT_NAME##_t * p, int ver);\

// Set up prepare_header method for each type
#define X_PUEO_PACKET_HEADER(IGNORE,STRUCT_NAME) \
  pueo_packet_head_t pueo_packet_header_for_##STRUCT_NAME(const pueo_##STRUCT_NAME##_t * p, int ver);\



PUEO_IO_DISPATCH_TABLE(X_PUEO_WRITE_PACKET)
PUEO_IO_DISPATCH_TABLE(X_PUEO_READ_PACKET)
PUEO_IO_DISPATCH_TABLE(X_PUEO_PACKET_HEADER)

#endif
