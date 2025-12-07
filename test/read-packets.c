#define _GNU_SOURCE
#include "pueo/rawdata.h"
#include "pueo/rawio.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>

#include <stdlib.h>

int is_directory(const char* path)
{
  struct stat path_stat;
  stat(path, &path_stat);
  return S_ISDIR(path_stat.st_mode);
}

int main(int nargs, char ** args)
{
  pueo_handle_t h;
  pueo_handle_init(&h, nargs > 1 ? args[1] : "test.wfs","r");
  char* outpath = NULL;

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
    outpath = args[3];
  }

  pueo_packet_t * packet = 0;
  printf("{\n");
  while (true)
  {
    int read = pueo_ll_read_realloc(&h, &packet);
    if (read <= 0) break;
    pueo_dump_packet(stdout,packet);
    if (db) pueo_db_insert_packet(db, packet);

    if (outpath) {
      char* ofile = NULL;
      char* mode = NULL;

      if(is_directory(outpath)) {
        // We're given a directory, so make up a filename
        struct timespec now;
        clock_gettime(CLOCK_REALTIME,&now);
        asprintf(&ofile,"%s/%d.%09d.dat", outpath, now.tv_sec, now.tv_nsec);
        asprintf(&mode, "w");
      }
      else {
        // We're given a full filename, append to that location
        ofile = outpath;
        asprintf(&mode, "a");
      };

      pueo_handle_t hout;
      pueo_handle_init(&hout, ofile, mode);
      pueo_ll_write(&hout, packet->head.type, packet->payload);
      pueo_handle_close(&hout);
    } 
    free(packet);
    packet = 0;
  }
  printf("}\n");
  return 0;
}

