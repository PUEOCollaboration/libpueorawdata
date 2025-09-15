#include "pueo/rawdata.h"
#include "pueo/rawio.h"
#include <stdbool.h>


int main(int nargs, char ** args)
{



  pueo_handle_t h;
  pueo_handle_init(&h, nargs > 1 ? args[1] : "test.wfs","r");
  fprintf(stderr, "Reading from %s\n", nargs > 1 ? args[1] : "test.wfs");
  pueo_single_waveform_t wf = {0};
  fprintf(stderr, "Starting read loop\n");
  while (true)
  {
    fprintf(stderr, "Reading waveform\n");
    int read = pueo_read_single_waveform(&h, &wf);
    printf("read: %d\n", read);
    if (read <= 0) break;
    pueo_dump_single_waveform(stdout,&wf);
  }
  return 0;
}

