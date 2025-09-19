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

#define _GNU_SOURCE


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
};

struct udp_aux
{
  int socket;
  uint16_t nin;
  uint16_t nout;
  char buf[65524]; // slightly bigger than max UDP packet size of 65507 so that we pad the page
                   // we could save some memory by making this smaller,but if we don't use it it's just virtual memory anyway.
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
static int fd_writebytes(size_t nbytes, const void * bytes, pueo_handle_t * h)
{
  int fd = (intptr_t) h->aux;
  return write(fd, bytes, nbytes);
}

static int fd_readbytes(size_t nbytes, void * bytes, pueo_handle_t  *h)
{
  int fd = (intptr_t) h->aux;
  return read(fd, bytes, nbytes);
}

static int fd_close(pueo_handle_t * h)
{
  int fd = (intptr_t) h->aux;
  return close(fd);
}

static int socket_close(pueo_handle_t *h)
{
  struct udp_aux  * aux = ( struct udp_aux*)  h->aux;
  if (!aux) return 0;
  close(aux->socket);
  free(aux);
  h->aux = NULL;
  return 0;
}

#define UDP_MAX 65507
static int socket_writebytes(size_t nbytes, const void * bytes, pueo_handle_t *h)
{
  struct udp_aux  * aux = ( struct udp_aux*)  h->aux;

  if (aux->nin + nbytes > UDP_MAX)
  {
    fprintf(stderr,"Trying to write more than UDP_MAX bytes to a socket. Will be truncated.\n");
    nbytes = UDP_MAX-aux->nin;
  }
  memcpy(aux->buf + aux->nin, bytes, nbytes);
  aux->nin +=nbytes;
  return nbytes;
}

static int socket_done(pueo_handle_t * h)
{
  struct udp_aux  * aux = ( struct udp_aux*)  h->aux;
  int r = 0;
  int socket = aux->socket;
  if (write(socket, aux->buf, aux->nin) < 0)
  {
    r = -1;
  }
  aux->nin = 0;
  return r;
}

static int socket_readbytes(size_t nbytes, void * bytes, pueo_handle_t  *h)
{
  struct udp_aux  * aux = ( struct udp_aux*)  h->aux;

  //we have a new thing
  if (aux->nin == aux->nout )
  {
    ssize_t  s = recv(aux->socket, aux->buf, sizeof(aux->buf), 0);
    if (s < 0) return -1;
    h->flags |= PUEO_HANDLE_ALREADY_READ_HEAD;
    aux->nin = s;
    aux->nout = 0;
  }

   uint32_t nleft = aux->nin - aux->nout;
   uint32_t ncopy = nleft < nbytes ? nleft : nbytes;
   memcpy(bytes, aux->buf+aux->nout, ncopy);
   aux->nout += ncopy;
   return ncopy;
}


static int file_writebytes(size_t nbytes, const void * bytes, pueo_handle_t *h )
{
  FILE *f  = (FILE*) h->aux;
  return fwrite(bytes, 1, nbytes, f);
}

static int file_readbytes(size_t nbytes, void * bytes, pueo_handle_t * h)
{
  FILE *f = (FILE*) h->aux;
  return fread(bytes, 1, nbytes, f);
}

static int file_close(pueo_handle_t * h)
{
  FILE * f = (FILE*) h->aux;
  return fclose(f);
}

static int gz_writebytes(size_t nbytes, const void * bytes, pueo_handle_t *h)
{
  gzFile  f  = (gzFile) h->aux;
  return gzwrite(f, bytes, nbytes);
}
static int gz_readbytes(size_t nbytes, void * bytes, pueo_handle_t *h)
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
  return 0;
}


int pueo_handle_init_filep(pueo_handle_t *h, FILE* f, bool close)
{

  hinit(h);
  if (!f) return -1;
  h->aux = f;
  h->close = close ? file_close : NULL;
  h->read_bytes = file_readbytes;
  h->write_bytes = file_writebytes;
  asprintf(&h->description, "FILE* at 0x%p", f);
  return 0;
}

int pueo_handle_init_file(pueo_handle_t *h, const char * file, const char * mode)
{

  hinit(h);

  //check for .gz or .zst suffix

  const char * suffix = rindex(file,'.');
  if ((suffix && !strcmp(suffix,".gz")) || ((suffix && !strcmp(suffix,".zst"))))
  {
    h->aux = gzopen(file, mode);
    if (!h->aux)
    {
      return -1;
    }
    h->close = gz_close;
    h->read_bytes = gz_readbytes;
    h->write_bytes = gz_writebytes;
    asprintf(&h->description,"gzfile %s", file);
    return 0;
  }

  //just use good old stdio
  h->aux = fopen(file, mode);
  if (!h->aux) return -1;
  h->close = file_close;
  h->read_bytes = file_readbytes;
  h->write_bytes = file_writebytes;
  h->description = strdup(file);
  return 0;
}


int pueo_handle_init_fd_with_desc(pueo_handle_t *h, int fd, const char * desc)
{
  hinit(h) ;

  if (fd < 0) return -1;

  h->aux = (void*) ( (intptr_t) fd) ;
  h->close = fd_close;
  h->read_bytes = fd_readbytes;
  h->write_bytes = fd_writebytes;
  if (desc)
  {
    h->description = strdup(desc);
  }
  else
  {
    char fdbuf[64];
    sprintf(fdbuf,"/proc/self/fds/%d", fd);
    char * fdname =realpath(fdbuf,NULL);
    asprintf(&h->description, "fd %d (%s)", fd, fdname);
    free(fdname);
  }
  return 0;
}

int pueo_handle_init_fd(pueo_handle_t *h, int fd)
{
  return pueo_handle_init_fd_with_desc(h,fd,NULL);
}

int pueo_handle_close(pueo_handle_t *h)
{
  if (h->close) h->close(h);
  free(h->description);
  return hinit(h);
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
    fprintf(stderr,"pueo_handle_udp: problem with getaddrinfo(%s)\n", hostname);
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
      return -1;
    }
  }
  else
  {

    // we need to be allowed to broadcas
     int enable_broadcast = 1;
     setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &enable_broadcast, sizeof(enable_broadcast));

    if (connect(sock,  (struct sockaddr*) &sa, sizeof(sa)))
    {

      fprintf(stderr,"pueo_handle_udp: couldn't connect to %s:%d\n", inet_ntoa(sa.sin_addr),port);
      return -1;
    }

  }
  struct udp_aux * aux = calloc(1,sizeof(struct udp_aux));
  h->aux = (void*) aux;
  aux->socket = sock;
  asprintf(&h->description, "udp-%s://%s:%d",mode, hostname, port);
  h->close = socket_close;
  h->write_bytes = am_writing ? socket_writebytes : NULL;
  h->read_bytes = am_reading ? socket_readbytes: NULL;
  h->done_write_packet = am_writing ? socket_done : NULL;
  return 0;
}


int pueo_handle_init(pueo_handle_t * h, const char * uri, const char * mode)
{


  // check for some prefixes
  const char * remainder = 0;
  if (check_uri_prefix(uri,"file://", &remainder))
  {
    return pueo_handle_init_file(h, remainder, mode);
  }
  else if (check_uri_prefix(uri,"udp:", &remainder))
  {
    //now look for a port and hostname
    const char * slashes = strstr(remainder,"//");
    if (!slashes) return -1;
    char * colon = strstr(remainder,":");
    if (!colon) return -1;
    *colon = 0;
    int port = atoi(colon+1);
    return pueo_handle_init_udp(h, port, slashes+2, mode);
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
    default:
      return -1;
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
  int nread =  h->read_bytes(sizeof(pueo_packet_head_t), &h->last_read_header, h);
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
    int r = maybe_read_header(h);
    if (r == EOF) return EOF;
    else if ( r< (int) sizeof(pueo_packet_head_t))
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
    nread += pueo_read_##TYPENAME(h, (pueo_##TYPENAME##_t*) dest->payload); break;

  switch (h->last_read_header.type)
  {
    PUEO_IO_DISPATCH_TABLE(X_PUEO_SWITCH_READ_PACKET)
  }

  h->required_read_size = 0;
  h->flags &= ~PUEO_HANDLE_ALREADY_READ_HEAD;
  return nread;
}

int pueo_packet_init(pueo_packet_t * p, int capacity)
{
  memset(&p->head,0,sizeof(p->head));
  p->payload_capacity = capacity;
  return 0;
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
  return (p->head.type == PACKET_TYPE && p->payload_capacity > (int) sizeof(pueo_##STRUCT_NAME##_t))\
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
    pueo_packet_head_t check = pueo_packet_header_for_##STRUCT_NAME(p, h->last_read_header.version); \
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
  pueo_packet_head_t  hd = pueo_packet_header_for_##STRUCT_NAME(p, PACKET_TYPE##_VER); \
  int ret = h->write_bytes(sizeof(hd), &hd, h); \
  if (ret != sizeof(hd)) return -1; \
  h->bytes_written += ret; \
  int ret2= pueo_write_packet_##STRUCT_NAME(h, p);\
  if (ret2 < 0) return ret2;\
  int ret3 = 0; \
  if (h->done_write_packet) ret3 = h->done_write_packet(h); \
  if (ret3) return ret3; \
  h->bytes_written += ret2; \
  h->packet_write_counter++; \
  return ret+ ret2;\
}\



PUEO_IO_DISPATCH_TABLE(X_PUEO_WRITE_IMPL)



// x macro for dump dispatch
#define X_PUEO_SWITCH_DUMP(PACKET_TYPE, TYPENAME)\
  case PACKET_TYPE: \
    return pueo_dump_##TYPENAME(f, (pueo_##TYPENAME##_t*) p->payload);

int pueo_dump_packet(FILE * f, const pueo_packet_t  *p)
{
  switch(p->head.type)
  {
    PUEO_IO_DISPATCH_TABLE(X_PUEO_SWITCH_DUMP)
    default:
      return -1;
  }
}


