#include "pueo/rawdata.h"
#include "pueo/rawio.h"


int main(int nargs, char ** args)
{

  pueo_single_waveform_t wf =  {.run = 123, .event = 456, .wf = { .channel_id = 101, .flags = 0x0f, .length = 1024 }};

  for (int i = 0; i < 1024;i++) wf.wf.data[i] = i;

  pueo_handle_t h;
  pueo_handle_init(&h, nargs > 1 ? args[1] : "test.wfs","w");
  pueo_write_single_waveform(&h, &wf);
  printf("{");
  pueo_dump_single_waveform(stdout, &wf);
  wf.event++;
  for (int i = 0; i < 1024;i++) wf.wf.data[i] = i*2;
  pueo_write_single_waveform(&h, &wf);
  pueo_dump_single_waveform(stdout, &wf);
  printf("}\n");
  return 0;
}

