#include "pueo/rawdata.h"
#include "pueo/rawio.h"
#include <stdbool.h>


int main(int nargs, char ** args)
{



  pueo_handle_t h;
  pueo_handle_init(&h, nargs > 1 ? args[1] : "sensors_telem.wfs","r");
  pueo_db_handle_t * db = NULL;
  if (nargs > 2) db = pueo_db_handle_open(args[2], 0);
  pueo_sensors_telem_t t = {0};
  while (true)
  {
    int read = pueo_read_sensors_telem(&h, &t);
    printf("read: %d\n", read);
    if (read <= 0) break;
    if (db) pueo_db_insert_sensors_telem(db, &t);
    pueo_dump_sensors_telem(stdout,&t);
  }
  pueo_handle_close(&h);
  if (db) pueo_db_handle_close(&db);
  return 0;
}

