#define _GNU_SOURCE 
#include "pueo/rawdata.h"
#include "pueo/rawio.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>

#include <stdlib.h>


int main(int nargs, char ** args)
{



  pueo_handle_t h;
  pueo_handle_init(&h, nargs > 1 ? args[1] : "test.wfs","r");
  const char * outdir = NULL;
  pueo_db_handle_t * db = NULL;
  if (nargs > 2)
  {
    db = pueo_db_handle_open(args[2],0);
    if (!db)
    {
      fprintf(stderr,"Trouble opening DB handle %s\n", args[2]);
    }
  }


  if (nargs > 3) 
  {
    outdir = args[3];

  }

  pueo_packet_t * packet = 0;
  printf("{\n");
  while (true)
  {
    int read = pueo_ll_read_realloc(&h, &packet);
//    fprintf(stderr,"read: %d\n", read);
    if (read <= 0) break;
    pueo_dump_packet(stdout,packet);
    if (db) pueo_db_insert_packet(db, packet);
    if (outdir) 
    {
      char * ofile = NULL;
      struct timespec now;
      clock_gettime(CLOCK_REALTIME,&now);
      asprintf(&ofile,"%s/%d.%09d.dat", outdir, now.tv_sec, now.tv_nsec);
      pueo_handle_t hout;
      pueo_handle_init(&hout, ofile, "w");
      pueo_ll_write(&hout, packet->head.type, packet->payload);
      pueo_handle_close(&hout);
    }
    free(packet);
    packet = 0;
  }
  printf("}\n");
  return 0;
}

