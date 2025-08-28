#include "pueo/rawdata.h"
#include "pueo/rawio.h"
#include <stdbool.h>


int main(int nargs, char ** args)
{



  pueo_handle_t h;
  pueo_handle_init(&h, nargs > 1 ? args[1] : "test.wfs","r");
  pueo_packet_t * packet = 0;
  while (true)
  {
    int read = pueo_ll_read_realloc(&h, &packet);
    printf("read: %d\n", read);
    if (read <= 0) break;
    pueo_dump_packet(stdout,packet);
  }
  return 0;
}

