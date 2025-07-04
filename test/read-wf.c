#include "pueo/rawdata.h"
#include "pueo/rawio.h"


int main(int nargs, char ** args)
{



  pueo_handle_t h;
  pueo_handle_init(&h, nargs > 1 ? args[1] : "test.wfs","r");
  pueo_single_waveform_t wf = {0};
  pueo_read_single_waveform(&h, &wf);
  pueo_dump_single_waveform(stdout,&wf);
  return 0;
}

