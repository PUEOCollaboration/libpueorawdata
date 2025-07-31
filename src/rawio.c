/** \file rawio.c
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



#include "pueo/rawio.h"
#include "rawio_packets.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <zlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>


enum pueo_handle_flags
{
  PUEO_HANDLE_ALREADY_READ_HEAD = 1,
  PUEO_HANDLE_PEEK_IF_NEEDED = 2,
  PUEO_HANDLE_NEEDED_TO_PEEK = 3 //TODO: this is a hack. right now we assume this was only for a header!
};

static int check_uri_prefix(const char * uri, const char * prefix, const char ** remainder)
{
  if (strstr(uri,prefix) == uri)
  {
    if (remainder)
    {
      *remainder = uri + strlen(prefix);
    }
    return 1;
  }
  return 0;
}

/******************* Useful function pointer definitions********* */
static int fd_writebytes(int nbytes, const void * bytes, pueo_handle_t * h)
{
  int fd = (intptr_t) h->aux;
  return write(fd, bytes, nbytes);
}

static int fd_readbytes(int nbytes, void * bytes, pueo_handle_t  *h)
{
  int fd = (intptr_t) h->aux;
  return read(fd, bytes, nbytes);
}

static int fd_close(pueo_handle_t * h)
{
  int fd = (intptr_t) h->aux;
  return close(fd);
}

//this is super hacky because we have to peek to read a header...
static int socket_readbytes(int nbytes, void * bytes, pueo_handle_t  *h)
{
  int socket = (intptr_t) h->aux;
  // we have to peek in this case
  if (h->flags & PUEO_HANDLE_PEEK_IF_NEEDED)
  {
    //we only support this case right now, otherwise we have to store how many bytes were peeked
    assert(nbytes == sizeof(pueo_packet_head_t));
    ssize_t  s = recv(socket, bytes, nbytes, MSG_PEEK);
    h->flags |= PUEO_HANDLE_NEEDED_TO_PEEK;
    h->flags &= ~PUEO_HANDLE_PEEK_IF_NEEDED;
    return s;
  }

  if (h->flags & PUEO_HANDLE_NEEDED_TO_PEEK)
  {
    h->flags &= ~PUEO_HANDLE_NEEDED_TO_PEEK; //clear flag

    //ignore the pueo_packet_head_t bytes in this case
    struct iovec iov[2]  = {
      {  .iov_base = NULL, .iov_len = sizeof(pueo_packet_head_t) },
      {  .iov_base = bytes, .iov_len = nbytes }
    };
    struct msghdr msg = { .msg_iov = iov, .msg_iovlen = 2 };
    return recvmsg(socket,&msg,0);
  }

  return read(socket, bytes, nbytes);
}


static int file_writebytes(int nbytes, const void * bytes, pueo_handle_t *h )
{
  FILE *f  = (FILE*) h->aux;
  return fwrite(bytes, 1, nbytes, f);
}
static int file_readbytes(int nbytes, void * bytes, pueo_handle_t * h)
{
  FILE *f = (FILE*) h->aux;
  return fread(bytes, 1, nbytes, f);
}

static int file_close(pueo_handle_t * h)
{
  FILE * f = (FILE*) h->aux;
  return fclose(f);
}

static int gz_writebytes(int nbytes, const void * bytes, pueo_handle_t *h)
{
  gzFile  f  = (gzFile) h->aux;
  return gzwrite(f, bytes, nbytes);
}
static int gz_readbytes(int nbytes, void * bytes, pueo_handle_t *h)
{
  gzFile f = (gzFile) h->aux;
  return gzread(f, bytes, nbytes);
}

static int gz_close(pueo_handle_t  * h)
{
  gzFile f = (gzFile) h->aux;
  return gzclose(f);
}


static int hinit(pueo_handle_t *h)
{
  // zero out
  memset(h,0,sizeof(pueo_handle_t));
}


int pueo_handle_init_file(pueo_handle_t *h, const char * file, const char * mode)
{

  hinit(h);

  //check for .gz

  const char * suffix = rindex(file,'.');
  if (suffix && !strcmp(suffix,".gz"))
  {
    h->aux = gzopen(file, mode);
    if (!h->aux)
    {
      return -1;
    }
    h->close = gz_close;
    h->read_bytes = gz_readbytes;
    h->write_bytes = gz_writebytes;
    return 0;
  }

  //just use good old stdio
  h->aux = fopen(file, mode);
  if (!h->aux) return -1;
  h->close = file_close;
  h->read_bytes = file_readbytes;
  h->write_bytes = file_writebytes;
  return 0;
}

int pueo_handle_init_fd(pueo_handle_t *h, int fd )
{
  hinit(h) ;

  if (fd < 0) return -1;

  h->aux = (void*) ( (intptr_t) fd) ;
  h->close = fd_close;
  h->read_bytes = fd_readbytes;
  h->write_bytes = fd_writebytes;
  return 0;
}

int pueo_handle_close(pueo_handle_t *h)
{
  h->close(h->aux);
  hinit(h);
}


int pueo_handle_init_udp(pueo_handle_t * h, int port, const char *hostname, const char * mode)
{
  hinit(h); 

  bool am_reading = !!strchr(mode,'r');
  bool am_writing = !!strchr(mode,'w');
  bool valid_mode = am_reading ^ am_writing;
  if (!valid_mode)
  {
    fprintf(stderr,"pueo_handle_udp: mode must have one of r and w in it\n");
    return -1;
  }


  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
  {
    fprintf(stderr,"Couldn't create socket\n");
    return -1;
  }

  struct addrinfo hints =
  {
    .ai_family = AF_INET,
    .ai_socktype = SOCK_DGRAM,
  };

  struct addrinfo * result = 0;
  if (getaddrinfo(hostname,NULL,&hints,&result))
  {
    fprintf(stderr,"pueo_handle_udp: problem with getaddrinfo\n");
    return -1;
  }


  struct sockaddr_in sa = { .sin_family = AF_INET, .sin_port = htons(port) };
  memcpy(&sa.sin_addr, &((struct sockaddr_in*) result->ai_addr)->sin_addr, sizeof(sa.sin_addr));
  freeaddrinfo(result);

  if (am_reading)
  {

    if (bind(sock, (struct sockaddr*)  &sa, sizeof(sa)))
    {
      fprintf(stderr,"pueo_handle_udp: couldn't bind to %s:%d\n", inet_ntoa(sa.sin_addr),port);
      return 1;
    }
  }
  else
  {
    if (connect(sock,  (struct sockaddr*) &sa, sizeof(sa)))
    {

      fprintf(stderr,"pueo_handle_udp: couldn't connect to %s:%d\n", inet_ntoa(sa.sin_addr),port);
      return 1;
    }

  }
  h->aux = (void*) ( (intptr_t) sock);
  h->close = fd_close;
  h->write_bytes = fd_writebytes;
  h->read_bytes = socket_readbytes;
  return 0;
}

int pueo_handle_init(pueo_handle_t * h, const char * uri, const char * mode)
{


  // check for some prefixes
  const char * remainder = 0;
  if (check_uri_prefix(uri,"file://", &remainder))
  {
    pueo_handle_init_file(h, remainder, mode);
  }
  else if (check_uri_prefix(uri,"udp:", &remainder))
  {
    //now look for a port and hostname
    const char * slashes = strstr(remainder,"//");
    if (!slashes) return -1;
    int port = atoi(remainder);
    pueo_handle_init_udp(h, port, slashes+2, mode);
  }
  return pueo_handle_init_file(h,uri,mode);
}



// x macro for write dispatch
#define X_PUEO_SWITCH_WRITE(PACKET_TYPE, TYPENAME)\
  case PACKET_TYPE: \
    return pueo_write_##TYPENAME(h, (pueo_##TYPENAME##_t*) p);

int pueo_ll_write(pueo_handle_t * h, pueo_datatype_t type, const void *p)
{
  switch(type)
  {
    PUEO_IO_DISPATCH_TABLE(X_PUEO_SWITCH_WRITE)
  }
}


int pueo_size_inmem(pueo_datatype_t type)
{
#define X_INMEM_SIZE(PACKET_TYPE, TYPENAME) \
    case PACKET_TYPE:\
      return sizeof(pueo_##TYPENAME##_t);

  switch (type)
  {
    PUEO_IO_DISPATCH_TABLE(X_INMEM_SIZE)

    default:
      return -1;
  }
}

static int maybe_read_header(pueo_handle_t *h)
{
  if (h->flags & PUEO_HANDLE_ALREADY_READ_HEAD)
  {
    return 0 ;
  }

  // if we have a socket, we must peek
  h->flags |= PUEO_HANDLE_PEEK_IF_NEEDED;
  int nread =  h->read_bytes(sizeof(pueo_packet_head_t), &h->last_read_header, h);
  h->flags &= ~PUEO_HANDLE_PEEK_IF_NEEDED;
  if (!nread) return EOF;
  h->bytes_read +=nread;
  h->flags |= PUEO_HANDLE_ALREADY_READ_HEAD;
  return nread;
}

int pueo_ll_read(pueo_handle_t *h, pueo_packet_t *dest)
{

  if (!dest) return -1;

  // if required_read_size is zero, that means we need to read the header
  // and set required_read_size
  if (!h->required_read_size)
  {
    if (maybe_read_header(h) < sizeof(pueo_packet_head_t))
    { //uhoh
      return -EIO;
    }

//x macro for size
#define X_PUEO_SWITCH_READ_SIZE(PACKET_TYPE, TYPENAME)\
  case PACKET_TYPE: \
    h->required_read_size = sizeof(pueo_##TYPENAME##_t*); break;

    switch(h->last_read_header.type)
    {
      PUEO_IO_DISPATCH_TABLE(X_PUEO_SWITCH_READ_SIZE)
    }

  }

  //now check for capacity
  if (dest->payload_capacity < h->required_read_size)
  {
    //Clear the head to make clear that it's invalid
    memset(&dest->head,0,sizeof(dest->head));
    return -EMSGSIZE;
  }

  //copy over the header
  memcpy(&dest->head, &h->last_read_header, sizeof(pueo_packet_head_t));

  int nread = 0;

//x macro for read
#define X_PUEO_SWITCH_READ_PACKET(PACKET_TYPE, TYPENAME)\
  case PACKET_TYPE: \
    nread += pueo_read_packet_##TYPENAME(h, (pueo_##TYPENAME##_t*) dest->payload, h->last_read_header.version);

  switch (h->last_read_header.type)
  {
    PUEO_IO_DISPATCH_TABLE(X_PUEO_SWITCH_READ_PACKET)
  }

  if (nread != h->required_read_size)
  {
    return -EIO;
  }

  h->required_read_size = 0;
  h->bytes_read += nread;
  return nread;
}

int pueo_packet_init(pueo_packet_t * p, int capacity)
{
  memset(&p->head,0,sizeof(p->head));
  p->payload_capacity = capacity;
}

int pueo_ll_read_realloc(pueo_handle_t *h, pueo_packet_t **dest)
{
  // Nothing has been allocated. Let's allocate with a capacity of 512 since why not
  if (!*dest)
  {
    *dest = malloc(sizeof(pueo_packet_t) + 512);
    pueo_packet_init(*dest, 512);
  }

  int nread = pueo_ll_read(h, *dest);

  if (nread == -EMSGSIZE)
  {
    free(*dest);
    *dest = malloc(sizeof(pueo_packet_t) + h->required_read_size);
    pueo_packet_init(*dest, h->required_read_size);
    nread = pueo_ll_read(h,*dest);
  }

  return nread;
}


/** define the packet_as_methods */

#define X_PUEO_CAST_IMPL(PACKET_TYPE, STRUCT_NAME) \
const pueo_##STRUCT_NAME##_t* pueo_packet_as_##STRUCT_NAME(const pueo_packet_t * p)\
{  \
  return (p->head.type == PACKET_TYPE && p->payload_capacity > sizeof(pueo_##STRUCT_NAME##_t))\
     ? (const pueo_##STRUCT_NAME##_t*) p->payload : NULL; \
}


PUEO_IO_DISPATCH_TABLE(X_PUEO_CAST_IMPL)





/** Implement pueo_read_X, which is a combination of checking if the header is correct and the  pueo_read_packet_X methods
 * Returns EOF if there's nothing left, but 0 if it's the wrong type!
 *
 **/
#define X_PUEO_READ_IMPL(PACKET_TYPE, STRUCT_NAME) \
int pueo_read_##STRUCT_NAME(pueo_handle_t *h, pueo_##STRUCT_NAME##_t * p)\
{\
  if (!p) return 0;  \
  int nread = maybe_read_header(h); \
  if (nread == EOF) return EOF; \
  if (h->last_read_header.type == PACKET_TYPE) {\
    h->flags &= ~PUEO_HANDLE_ALREADY_READ_HEAD; \
    int nrd = pueo_read_packet_##STRUCT_NAME(h, p, h->last_read_header.version); \
    h->bytes_read += nrd; \
    pueo_packet_head_t check = pueo_packet_header_for_##STRUCT_NAME(p); \
    if (check.cksum != h->last_read_header.cksum) fprintf(stderr,"Checksum check failed (hd: %hx, reconstructed: %hx)!\n", h->last_read_header.cksum, check.cksum);\
    if (nrd != h->last_read_header.num_bytes || h->last_read_header.num_bytes != check.num_bytes) fprintf(stderr,"Length check failed (header: %u, nrd: %d, reconstructed_header: %u)!\n", h->last_read_header.num_bytes, nrd, check.num_bytes);\
    return nrd;\
  }\
  return 0;\
}\

PUEO_IO_DISPATCH_TABLE(X_PUEO_READ_IMPL)

/** Implement pueo_write_X, which is adding a header and the  pueo_write_packet_X methods
 * Returns EOF if there's nothing left, but 0 if it's the wrong type!
 *
 **/
#define X_PUEO_WRITE_IMPL(PACKET_TYPE, STRUCT_NAME) \
int pueo_write_##STRUCT_NAME(pueo_handle_t *h, const pueo_##STRUCT_NAME##_t * p)\
{\
  pueo_packet_head_t  hd = pueo_packet_header_for_##STRUCT_NAME(p); \
  int ret = h->write_bytes(sizeof(hd), &hd, h); \
  if (ret != sizeof(hd)) return -1; \
  h->bytes_written += ret; \
  int ret2= pueo_write_packet_##STRUCT_NAME(h, p);\
  if (ret2 < 0) return ret2;\
  h->bytes_written += ret2; \
  h->packet_write_counter++; \
  return ret+ ret2;\
}\



PUEO_IO_DISPATCH_TABLE(X_PUEO_WRITE_IMPL)
