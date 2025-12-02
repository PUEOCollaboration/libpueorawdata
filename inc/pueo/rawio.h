#ifndef _PUEO_RAWIO_H
#define _PUEO_RAWIO_H

/** \file pueo/rawio.h
 *
 * I/O functions for PUEO.
 *
 * Note that this header file serves as poor documentation due to the use of
 * XMACROS for a modicum of consistency.
 *
 * pueo/pueo.h for constants  etc.  See pueo/rawdata.h for the data structures
 * themselves
 *
 * Eventually doc/io.md will document this!
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

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <pueo/pueo.h>
#include <pueo/rawdata.h>

#ifdef __cplusplus
extern "C"
{
#endif


/** The pueo_packet_t is needed only when reading data that you have to dispatch on
 *  Recommended you initialize with pueo_packet_init.
 *   TODO: Would it be better to just use a union with all possible payload sizes? I'm not sure... that would be big.
 */
typedef struct pueo_packet
{
  pueo_packet_head_t head;
  int payload_capacity;
  uint8_t payload[]; //flexible array member
} pueo_packet_t;


int pueo_packet_init(pueo_packet_t * p, int capacity);



/** This defines a generic interface for something we read or write from.
 *
 * It is not expected most users will need to do this (see methods below) , but the interface is exported in case.
 *
 * */
typedef struct pueo_handle
{
  uint32_t packet_write_counter;  //Will increment each time when writing.
  pueo_packet_head_t last_read_header; //you can use this to interrogate the packet number, and also if you need to make more room
  int required_read_size;  //if non-zero, that means the last read didn't have enough space to fit the payload. This tells you how much space it needs.
  int flags ; // for internal use;

  uint64_t bytes_written;
  uint64_t bytes_read;
  char * description;

  //Function pointers
  int (*write_bytes) (size_t nbytes, const void *bytes, struct pueo_handle * h);
  int (*read_bytes)  (size_t nbytes, void * bytes, struct pueo_handle * h);
  int (*close) (struct pueo_handle *h);

  void *aux;

  //needed for UDP, when done writing.
  int (*done_write_packet) (struct pueo_handle *h);
} pueo_handle_t;


/**
 *
 * This supports many backends via a string uri.
 * Currently supported prefixes are:
 *
 *    (none):  treat as file://
 *     file://filename treat as a file
 *     udp:port//host  udp socket
 *
 * @param h A handle to initialize (will be zeroed out!)
 * @param uri a recognized URI. Will default to a filename if no prefix. If ends with .gz will use zlib
 * @param mode in many cases just r or w, but some URI's support additional mode. Note that in most cases you either wnat just r or w, not both

 */
int pueo_handle_init(pueo_handle_t * h, const char * uri, const char * mode);

/** This initializes a handle corresponding to a file. If it ends with .gz, zlib is used automatically,
 * If it ends with .zst, zstd is used autoagically.
 * If writing a gzfile, mode is passed to gzopen so you can use it to set compression level /strategy

 **/
int pueo_handle_init_file(pueo_handle_t * h, const char * file, const char * mode);

/* if you already have a FILE * (e.g. through fmemopen). If close is true, the FILE * will be closed when the handle is closed. 
 * */
int pueo_handle_init_filep(pueo_handle_t * h, FILE * fptr, bool close);


/* This initailizes an fd-like handle */
int pueo_handle_init_fd(pueo_handle_t * h, int fd);
int pueo_handle_init_fd_with_desc(pueo_handle_t * h, int fd, const char * description);


int pueo_handle_init_udp(pueo_handle_t *h, int port, const char *hostname, const char * mode);

// This  will normally be equivalent to something like h->close(h->aux)
int pueo_handle_close(pueo_handle_t  *h);


/** In-memory size of type */
int pueo_size_inmem(pueo_datatype_t type);



/**
 * Low-level write, dispatching appropriately based on the datatype.
 *
 * Note that for every type, there is also a pueo_write_X that is type-safe.
 */
int pueo_ll_write(pueo_handle_t *h, pueo_datatype_t type, const void * p);



/** Low-level read that writes into a generic pueo_packet_t with a given
 * payload capacity.
 *
 * If there is insufficient payload capacity for the payload
 * (which can only be determined when reading the header), -EMSGSIZE is returned
 * and you'll want to try again.  After receiving -EMSGSIZE, to figure out what
 * capacity you need afyou can query h->required_read_size. After a succsesful
 * read, h->required_read_size will be 0.
 *
 * An alternative to using pueo_packet_t is to use pueo_read_X (where X e.g. single_waveform) which will attempt to
 * read a given type from a stream. Those functions are typesafe, but will return 0 if the type is not the right one!
 *
 */
int pueo_ll_read(pueo_handle_t *h, pueo_packet_t * dest);

/** Similar to above, except we pass a pointer to a pointer to dest and it will be realloced if it has insufficient capacity.
 **/
int pueo_ll_read_realloc(pueo_handle_t *h, pueo_packet_t **dest);

int pueo_dump_packet(FILE *f, const pueo_packet_t * p);


/** IO dispatch table, for use with X macros. See https://en.wikipedia.org/wiki/X_Macro if you don't know what this is.
 *
 * Basically we want the association between the struct and the type code to be robust.
 * The arguments are the TYPE and the "meat" of the struct (i.e. pueo_packet_head_t becomes packet_head)
 *
 ***/
#define PUEO_IO_DISPATCH_TABLE(X) \
  X(PUEO_FULL_WAVEFORMS, full_waveforms)\
  X(PUEO_SINGLE_WAVEFORM, single_waveform)\
  X(PUEO_NAV_ATT, nav_att)\
  X(PUEO_NAV_SAT, nav_sat)\
  X(PUEO_NAV_POS, nav_pos)\
  X(PUEO_SENSORS_TELEM, sensors_telem)\
  X(PUEO_SENSORS_DISK, sensors_disk)\
  X(PUEO_SLOW, slow)\
  X(PUEO_CMD_ECHO, cmd_echo)\
  X(PUEO_SS, ss)\
  X(PUEO_TIMEMARK, timemark)\
  X(PUEO_LOGS, logs)\
  X(PUEO_DAQ_HSK, daq_hsk)


//  X(PUEO_ENCODED_WAVEFORM, encoded_waveform)


// Set up write method for each type
#define X_PUEO_WRITE(IGNORE,STRUCT_NAME) \
  int pueo_write_##STRUCT_NAME(pueo_handle_t *h, const pueo_##STRUCT_NAME##_t * p);

// Set up read method for each type
#define X_PUEO_READ(IGNORE,STRUCT_NAME) \
  int pueo_read_##STRUCT_NAME(pueo_handle_t *h, pueo_##STRUCT_NAME##_t * p);

// This sets up safe-ish methods for using a pueo_packet_t. Will either return the pointer, or NULL if not the right type.
#define X_PUEO_CAST(IGNORE,STRUCT_NAME) \
  const pueo_##STRUCT_NAME##_t* pueo_packet_as_##STRUCT_NAME(const pueo_packet_t * p);

// This sets up a dumper
#define X_PUEO_DUMP(IGNORE,STRUCT_NAME) \
  int  pueo_dump_##STRUCT_NAME(FILE* f, const pueo_##STRUCT_NAME##_t * p);

// This sets up a database insertion
#define X_PUEO_INSERT_DB(IGNORE,STRUCT_NAME) \
  int  pueo_db_insert_##STRUCT_NAME(pueo_db_handle_t * h, const pueo_##STRUCT_NAME##_t * p);


// database entry, for serializing packets to database (mostly for housekeeping)
// all of the bits for this will be in rawio_db.c (including for each datatype)
typedef struct pueo_db_handle pueo_db_handle_t;

//flags when creating database
enum e_pueo_db_flags
{
  PUEO_DB_MAYBE_INIT_TABLES     = 1<<1,  //Initialize tables (always with if not exist)
  PUEO_DB_INIT_WITH_TIMESCALEDB = 1<<2, // when initializing tables with PGSQL, also create timescaledb hypertables
  PUEO_DB_VERBOSE               = 1<<3 // write out a bunch of extra stuff
};

pueo_db_handle_t * pueo_db_handle_open(const char * uri, uint64_t flags);

// Open a handle to a PGSQL database (requires libpq)
pueo_db_handle_t * pueo_db_handle_open_pgsql(const char * conninfo, uint64_t flags);

// Write .sql files to a directory
pueo_db_handle_t * pueo_db_handle_open_sqlfiles_dir(const char * dir, uint64_t flags);

// open a handle to a sqlite database (requires sqlite)
pueo_db_handle_t * pueo_db_handle_open_sqlite(const char * sqlite, uint64_t flags);

//Close a DB handle (and also frees associated memory and sets h to NULL)
void pueo_db_handle_close(pueo_db_handle_t ** h);

int pueo_db_insert_packet(pueo_db_handle_t * db,  const pueo_packet_t * p);

PUEO_IO_DISPATCH_TABLE(X_PUEO_WRITE)
PUEO_IO_DISPATCH_TABLE(X_PUEO_READ)
PUEO_IO_DISPATCH_TABLE(X_PUEO_CAST)
PUEO_IO_DISPATCH_TABLE(X_PUEO_DUMP)
PUEO_IO_DISPATCH_TABLE(X_PUEO_INSERT_DB)

#ifdef __cplusplus
}
#endif


#endif
