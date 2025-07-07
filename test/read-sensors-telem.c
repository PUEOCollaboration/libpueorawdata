#include "pueo/rawdata.h"
#include "pueo/rawio.h"
#include <stdbool.h>


int main(int nargs, char ** args)
{



  pueo_handle_t h;
  pueo_handle_init(&h, nargs > 1 ? args[1] : "sensors_telem.wfs","r");
  pueo_sensors_telem_t t = {0};
  while (true)
  {
    int read = pueo_read_sensors_telem(&h, &t);
    printf("read: %d\n", read);
    if (read <= 0) break;
    pueo_dump_sensors_telem(stdout,&t);
  }
  return 0;
}

