#include "pueo/rawdata.h"
#include "pueo/rawio.h"
#include <stdbool.h>


int main(int nargs, char ** args)
{



  pueo_handle_t h;
  pueo_handle_init(&h, nargs > 1 ? args[1] : "test.wfs","r");
  pueo_full_waveforms_t wf = {0};
  while (true)
  {
    int read = pueo_read_full_waveforms(&h, &wf);
    printf("read: %d\n", read);
    if (read <= 0) break;
    pueo_dump_full_waveforms(stdout,&wf);
  }
  return 0;
}

