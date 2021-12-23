#include "pueo/rawio.h" 
#include "rawio_versions.h" 
#include <stddef.h> 
#include <stdio.h> 
#include <string.h> 


/**** Here we define the read and write implementations for each type of
 * packets. 
 *
 * We only support writing to the latest version, but we must support reading
 * from all versions. 
 *
 * In simple cases, we can automatically generate the read/write packet code,
 * even supporting different versions. To be considered simple, new versions
 * must only append variables at the end and be ok with having everything else
 * zeroed.  In that case, the SIMPLE_PUEO_IO_IMPL macro can generate the code. 
 *
 * Otherwise, we have to write pueo_read_packet_X and pueo_write_packet_X
 * ourselves.  In general, these implmentations should check the version, do
 * something sensible if it's an old version, and then increment bytes_read or
 * bytes_written on the handle appropriately. 
 *
 * You can count on pueo_X_size(ver) to return the correct value provided that the macros 
 * in versions. c
 *
 ****/

#define SIMPLE_PUEO_IO_IMPL(STRUCT_NAME) \
  int pueo_write_##STRUCT_NAME(pueo_handle_t *h, const pueo_##STRUCT_NAME##_t *p ) \
  { \
    int nwr = h->write_bytes( sizeof(pueo_##STRUCT_NAME##_t), p, h->aux); \
    h->bytes_written += nwr; return nwr; \
  } \
  int pueo_read_packet_##STRUCT_NAME(pueo_handle_t *h, pueo_##STRUCT_NAME##_t *p, int ver ) \
  { \
    int versize = pueo_##STRUCT_NAME##_size(ver); \
    if (versize < 0 || versize > sizeof(pueo_##STRUCT_NAME##_t)) { fprintf(stderr,"bad ver size of %d for ver %d\n", versize, ver, #STRUCT_NAME); return -1; } \
    int nread = h->read_bytes( versize, p, h->aux);\
    memset(p+versize, 0, sizeof(pueo_##STRUCT_NAME##_t) - versize); \
    h->bytes_read += nread; \
    return nread; \
  }  

SIMPLE_PUEO_IO_IMPL(packet_head) 




///////// More complicated implementations here

//helpers for pueo_waveform_t (which must be embedded into another datatype) 
static int write_waveform(pueo_handle_t*h, const pueo_waveform_t * wf) 
{
  return h->write_bytes(offsetof(pueo_waveform_t, data) + wf->length * sizeof(*wf->data), wf, h->aux); 
}

static int read_waveform(pueo_handle_t*h, pueo_waveform_t * wf) 
{
  int hdrsize = offsetof(pueo_waveform_t,data); 
  int nrd = h->read_bytes(hdrsize, wf, h->aux); 
  if (nrd != hdrsize) return -1; 
  nrd += h->read_bytes(wf->length*sizeof(*wf->data), wf->data, h->aux); 
  if (nrd != hdrsize + wf->length*sizeof(*wf->data)) return -1; 
  return nrd; 
}


/* Write a single waveform. First write the part before the pueo_waveform_t, then the part after
 * If not padded properly, this should still roundtrip ok... 
 **/ 
int pueo_write_packet_single_waveform(pueo_handle_t *h, const pueo_single_waveform_t *p) 
{
  int nwr = 0; 
  nwr += h->write_bytes(offsetof(pueo_single_waveform_t, wf), p, h->aux); 
  nwr += write_waveform(h, &p->wf); 
  h->bytes_written += nwr; 
  return nwr; 
}


int pueo_read_packet_single_waveform(pueo_handle_t *h, pueo_single_waveform_t *p) 
{
  int total_read = h->read_bytes(offsetof(pueo_single_waveform_t, wf), p, h->aux); 
  if (total_read != offsetof(pueo_single_waveform_t, wf)) return -1; 
  int nrd = read_waveform(h, &p->wf); 
  if (nrd < 0) return -1; 
  total_read += nrd; 
  h->bytes_written += total_read; 
  return total_read; 
}

int pueo_write_packet_full_waveforms(pueo_handle_t *h, const pueo_full_waveforms_t *p) 
{
  int nwr = 0; 
  nwr += h->write_bytes(offsetof(pueo_single_waveform_t, wf), p, h->aux); 
  for (int i = 0; i < PUEO_NCHAN; i++) 
  {
    nwr += write_waveform(h, &p->wfs[i]); 
  }

  h->bytes_written += nwr; 
  return nwr; 
}

int pueo_read_packet_full_waveforms(pueo_handle_t *h, pueo_full_waveforms_t *p) 
{
  int total_read = h->read_bytes(offsetof(pueo_single_waveform_t, wf), p, h->aux); 
  if (total_read != offsetof(pueo_single_waveform_t, wf)) return -1; 
  for (int i = 0; i < PUEO_NCHAN; i++)
  {
    int nrd = read_waveform(h, &p->wfs[i]); 
    if (nrd < 0) return -1; 
    total_read += nrd; 
  }
  h->bytes_written += total_read; 
  return total_read; 
}

