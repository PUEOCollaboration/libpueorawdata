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

int main(int nargs, char** args)
{
  if(nargs < 3) {
    fprintf(stdout, "Usage: %s instream outfile [db-handle]\n", args[0]);
    exit(1);
  }

  pueo_handle_t h;
  pueo_handle_init(&h, args[1], "r");
  char* outpath = args[2];
  
  pueo_db_handle_t * db = NULL;
  if(nargs > 3) {
    db = pueo_db_handle_open(args[3], 0);
    if (!db) {
      fprintf(stderr,"Trouble opening DB handle %s\n", args[3]);
    }
  }

  pueo_packet_t * packet = 0;
  while(true)
  {
    int read = pueo_ll_read_realloc(&h, &packet);
    if (read <= 0) break;

    if(packet -> head.type == PUEO_FILE_DOWNLOAD) {
      if (db) pueo_db_insert_packet(db, packet);
      const pueo_file_download_t* pfd = (const pueo_file_download_t*)(packet -> payload);
      char fname = pfd -> fname;

      char* ofile = NULL;
      char* mode = NULL;

      if(is_directory(outpath)) {
        // We're given a directory, so make up a filename
        struct timespec now;
        clock_gettime(CLOCK_REALTIME,&now);
        asprintf(&ofile,"%s/%d.%09d.%s", outpath, now.tv_sec, now.tv_nsec, fname);
        asprintf(&mode, "w");
      }
      else {
        // We're given a full filename, append to that location
        ofile = outpath;
        asprintf(&mode, "a");
      };

      pueo_handle_t hout;
      pueo_handle_init(&hout, ofile, "a");
      pueo_ll_write(&hout, packet->head.type, packet->payload);
      pueo_handle_close(&hout);
    }
  }
  return 0;
}
