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
    fprintf(stdout, "Usage: %s instream outfile [nevts-per-file] [db-handle]\n", args[0]);
    exit(1);
  }

  pueo_handle_t h;
  pueo_handle_init(&h, args[1], "r");
  char* outpath = args[2];

  int nevts_to_save = -1;
  if(nargs > 3) {
    nevts_to_save = atoi(args[3]);
  }
  
  pueo_db_handle_t * db = NULL;
  if(nargs > 4) {
    db = pueo_db_handle_open(args[4], 0);
    if (!db) {
      fprintf(stderr,"Trouble opening DB handle %s\n", args[2]);
    }
  }

  int last_event = -1;
  int nevts_written = 0;

  pueo_handle_t hout;
  pueo_handle_init(&hout, outpath, "a");

  pueo_packet_t * packet = 0;
  while(true)
  {
    int read = pueo_ll_read_realloc(&h, &packet);
    if (read <= 0) break;

    if(packet -> head.type == PUEO_SINGLE_WAVEFORM) {
      if (db) pueo_db_insert_packet(db, packet);
      const pueo_single_waveform_t* swf = (const pueo_single_waveform_t*)(packet -> payload);
      int event = swf -> event;

      pueo_ll_write(&hout, packet->head.type, packet->payload);
     
      if(event != last_event) {
        last_event = event;
	nevts_written++;
	printf("Events written = %i\n", nevts_written);
      }
      
      if(nevts_written > nevts_to_save) {
	break;
      }
    }
  }

  pueo_handle_close(&hout);
  return 0;
}
