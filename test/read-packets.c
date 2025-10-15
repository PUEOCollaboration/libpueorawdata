#include "pueo/rawdata.h"
#include "pueo/rawio.h"
#include <stdbool.h>


int main(int nargs, char ** args)
{



  pueo_handle_t h;
  pueo_handle_init(&h, nargs > 1 ? args[1] : "test.wfs","r");

  pueo_db_handle_t * db = NULL;
  if (nargs > 2)
  {
    db = pueo_db_handle_open(args[2],0);
    if (!db)
    {
      fprintf(stderr,"Trouble opening DB handle %s\n", args[2]);
    }
  }
  pueo_packet_t * packet = 0;
  while (true)
  {
    int read = pueo_ll_read_realloc(&h, &packet);
    printf("read: %d\n", read);
    if (read <= 0) break;
    pueo_dump_packet(stdout,packet);
    if (db) pueo_db_insert_packet(db, packet);
  }
  return 0;
}

