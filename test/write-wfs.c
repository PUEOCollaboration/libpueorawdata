#include "pueo/rawdata.h"
#include "pueo/rawio.h"


int main(int nargs, char ** args)
{

  pueo_full_waveforms_t wf =  {.run = 123, .event = 456 };

  for (int chan = 0; chan < 224; chan++)
  {
    for (int i = 0; i < 1024;i++) wf.wfs[chan].data[i] = chan*100  + i % 64;
  }

  pueo_handle_t h;
  pueo_handle_init(&h, nargs > 1 ? args[1] : "test.wfs","w");
  pueo_write_full_waveforms(&h, &wf);
  printf("{");
  pueo_dump_full_waveforms(stdout, &wf);
  wf.event++;
  for (int chan = 0; chan < 224; chan++)
  {
    wf.wfs[chan].channel_id = chan;
    wf.wfs[chan].length = 1024;

    for (int i = 0; i < 1024;i++) wf.wfs[chan].data[i] = chan*100  + 64 + i % 64;
  }


  pueo_write_full_waveforms(&h, &wf);
  pueo_dump_full_waveforms(stdout, &wf);

  printf("}\n");
  return 0;
}

