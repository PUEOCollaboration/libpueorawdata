#include "pueo/rawio.h"
#include "rawio_versions.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "pueocrc.h"

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
 * in versions.c are dfeined.
 *
 ****/

#define SIMPLE_PUEO_O_IMPL(STRUCT_NAME, IGNORED) \
  int pueo_write_packet_##STRUCT_NAME(pueo_handle_t *h, const pueo_##STRUCT_NAME##_t *p ) \
  { \
    int nwr = h->write_bytes( sizeof(pueo_##STRUCT_NAME##_t), p, h); \
    h->bytes_written += nwr; return nwr; \
  }

#define SIMPLE_PUEO_H_IMPL(STRUCT_NAME, TYPE_NAME) \
  pueo_packet_head_t pueo_packet_header_for_##STRUCT_NAME(const pueo_##STRUCT_NAME##_t *p , int ver) \
  { \
    pueo_packet_head_t hd = {\
      .type = PUEO_##TYPE_NAME,\
      .f1 = 0xf1,\
      .version = ver,\
      .cksum = pueo_crc16(p,pueo_##STRUCT_NAME##_size(ver)),\
      .num_bytes = pueo_##STRUCT_NAME##_size(ver) \
    }; \
    return hd;\
  }

#define SIMPLE_PUEO_I_IMPL(STRUCT_NAME, IGNORED) \
  int pueo_read_packet_##STRUCT_NAME(pueo_handle_t *h, pueo_##STRUCT_NAME##_t *p, int ver ) \
  {\
    int versize = pueo_##STRUCT_NAME##_size(ver); \
    if (versize < 0 || versize > (int) sizeof(pueo_##STRUCT_NAME##_t)) { fprintf(stderr,"bad ver size of %d for ver %d for %s\n", versize, ver, #STRUCT_NAME); return -1; } \
    int nread = h->read_bytes( versize, p, h);\
    memset((uint8_t*)p+versize, 0, sizeof(pueo_##STRUCT_NAME##_t) - versize); \
    h->bytes_read += nread; \
    return nread; \
  }

#define SIMPLE_PUEO_IO_IMPL(STRUCT_NAME, TYPE_NAME) \
   SIMPLE_PUEO_I_IMPL(STRUCT_NAME, TYPE_NAME) \
   SIMPLE_PUEO_O_IMPL(STRUCT_NAME, TYPE_NAME) \
   SIMPLE_PUEO_H_IMPL(STRUCT_NAME, TYPE_NAME)


SIMPLE_PUEO_IO_IMPL(nav_att, NAV_ATT)
SIMPLE_PUEO_IO_IMPL(slow, SLOW)
SIMPLE_PUEO_IO_IMPL(ss,SS)

///////// More complicated implementations here

//helpers for pueo_waveform_t (which must be embedded into another datatype)
static int write_waveform(pueo_handle_t*h, const pueo_waveform_t * wf)
{
  return h->write_bytes(offsetof(pueo_waveform_t, data) + wf->length * sizeof(*wf->data), wf, h);
}

static int read_waveform(pueo_handle_t*h, pueo_waveform_t * wf)
{
  int hdrsize = offsetof(pueo_waveform_t, data);
  int nrd = h->read_bytes(hdrsize, wf, h);
  if (nrd != hdrsize) return -1;
  int want = wf->length * sizeof(*wf->data);
  nrd = h->read_bytes(want, wf->data, h);
  if (nrd != want) return -1;
  return hdrsize + nrd;
}

static void update_len_cksum_waveform(uint32_t * len, uint16_t * cksum, const pueo_waveform_t *wf)
{
  if (!wf) return;
  size_t size = offsetof(pueo_waveform_t,data) +wf->length *sizeof(*wf->data) ;
  if (cksum) *cksum = pueo_crc16_continue(*cksum, wf, size);
  if (len) *len += size;
}

/* Write a single waveform. First write the part before the pueo_waveform_t, then the part after
 * If not padded properly, this should still roundtrip ok...
 **/
int pueo_write_packet_single_waveform(pueo_handle_t *h, const pueo_single_waveform_t *p)
{
  int nwr = 0;
  nwr += h->write_bytes(offsetof(pueo_single_waveform_t, wf), p, h);
  nwr += write_waveform(h, &p->wf);
  h->bytes_written += nwr;
  return nwr;
}

pueo_packet_head_t pueo_packet_header_for_single_waveform(const pueo_single_waveform_t *p, int ver)
{
  pueo_packet_head_t hd = {.type = PUEO_SINGLE_WAVEFORM, .f1 = 0xf1, .version = ver};
  uint32_t len = 0;
  uint16_t crc = CRC16_START;
  len += offsetof(pueo_full_waveforms_t, wfs);
  crc = pueo_crc16_continue(crc,p, len);
  update_len_cksum_waveform(&len,&crc,&p->wf);
  hd.num_bytes = len;
  hd.cksum = crc;
  return hd;
}


int pueo_read_packet_single_waveform(pueo_handle_t *h, pueo_single_waveform_t *p, int ver)
{
  (void) ver;
  int hdrsize = offsetof(pueo_single_waveform_t, wf);
  int total_read = h->read_bytes(hdrsize, p, h);
  if (total_read != hdrsize) return -1;
  int nrd = read_waveform(h, &p->wf);
  if (nrd < 0) return -1;
  total_read += nrd;
  h->bytes_written += total_read;
  return total_read;
}

pueo_packet_head_t pueo_packet_header_for_full_waveforms(const pueo_full_waveforms_t *p, int ver)
{
  pueo_packet_head_t hd = {.type = PUEO_FULL_WAVEFORMS, .f1 = 0xf1, .version = ver };
  uint32_t len = 0;
  uint16_t crc = CRC16_START;
  len += offsetof(pueo_full_waveforms_t, wfs);
  crc = pueo_crc16_continue(crc,p, len);
  for (int i = 0; i < PUEO_NCHAN; i++) update_len_cksum_waveform(&len,&crc,&p->wfs[i]);
  hd.num_bytes = len;
  hd.cksum = crc;
  return hd;
}

int pueo_write_packet_full_waveforms(pueo_handle_t *h, const pueo_full_waveforms_t *p)
{
  int nwr = 0;
  nwr += h->write_bytes(offsetof(pueo_full_waveforms_t, wfs), p, h);
  for (int i = 0; i < PUEO_NCHAN; i++)
  {
    nwr += write_waveform(h, &p->wfs[i]);
  }

  h->bytes_written += nwr;
  return nwr;
}

int pueo_read_packet_full_waveforms(pueo_handle_t *h, pueo_full_waveforms_t *p, int ver)
{
  (void) ver;
  int total_read = h->read_bytes(offsetof(pueo_single_waveform_t, wf), p, h);
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

pueo_packet_head_t pueo_packet_header_for_sensors_disk(const pueo_sensors_disk_t * t, int ver)
{
   int my_size = ver == 0 ? sizeof(pueo_sensors_disk_t) : offsetof(pueo_sensors_disk_t,sensors) + t->num_packets * sizeof(pueo_sensor_disk_t);
    pueo_packet_head_t hd = {
      .type = PUEO_SENSORS_DISK,
      .f1 = 0xf1,
      .version = ver,
      .cksum = pueo_crc16(t,my_size),
      .num_bytes = my_size
    };
    return hd;
}

int pueo_read_packet_sensors_disk(pueo_handle_t * h, pueo_sensors_disk_t * t, int ver)
{
  int nrd = 0;
  nrd =  h->read_bytes(offsetof(pueo_sensors_disk_t, sensors),t,h);
  nrd += h->read_bytes( (ver == 0 ? MAX_SENSORS_PER_PACKET_DISK : t->num_packets) * sizeof(pueo_sensor_disk_t), t->sensors, h);
  memset((uint8_t*) t+nrd, 0, sizeof(*t)-nrd);
  h->bytes_read += nrd;
  return nrd;
}


int pueo_write_packet_sensors_disk(pueo_handle_t * h, const pueo_sensors_disk_t * t)
{
  int nwr = 0;
  nwr =  h->write_bytes(offsetof(pueo_sensors_disk_t, sensors) + t->num_packets * sizeof(pueo_sensor_disk_t), t, h);
  h->bytes_written += nwr;
  return nwr;
}

pueo_packet_head_t pueo_packet_header_for_sensors_telem(const pueo_sensors_telem_t * t, int ver)
{
   int my_size = ver == 0 ? sizeof(pueo_sensors_telem_t) : offsetof(pueo_sensors_telem_t,sensors) + t->num_packets * sizeof(pueo_sensor_telem_t);
    pueo_packet_head_t hd = {
      .type = PUEO_SENSORS_TELEM,
      .f1 = 0xf1,
      .version = ver,
      .cksum = pueo_crc16(t,my_size),
      .num_bytes = my_size
    };
    return hd;
}

int pueo_read_packet_sensors_telem(pueo_handle_t * h, pueo_sensors_telem_t * t, int ver)
{
  int nrd = 0;
  nrd =  h->read_bytes(offsetof(pueo_sensors_telem_t, sensors),t,h);
  nrd += h->read_bytes( (ver == 0 ? MAX_SENSORS_PER_PACKET_TELEM : t->num_packets) * sizeof(pueo_sensor_telem_t), t->sensors, h);
  memset((uint8_t*) t+nrd, 0, sizeof(*t)-nrd);
  h->bytes_read += nrd;
  return nrd;
}


int pueo_write_packet_sensors_telem(pueo_handle_t * h, const pueo_sensors_telem_t * t)
{
  int nwr = 0;
  nwr =  h->write_bytes(offsetof(pueo_sensors_telem_t, sensors) + t->num_packets * sizeof(pueo_sensor_telem_t), t, h);
  h->bytes_written += nwr;
  return nwr;
}


int pueo_write_packet_encoded_waveforms(pueo_handle_t *h, const pueo_encoded_waveform_t *p)
{
  //TODO
  return -1;
}

int pueo_read_packet_encoded_waveforms(pueo_handle_t *h, pueo_encoded_waveform_t *p, int ver)
{
  //TODO
  return -1;
}

