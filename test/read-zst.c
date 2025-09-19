#include "pueo/rawdata.h"
#include "pueo/rawio.h"
#include <stdio.h>
#include <stdbool.h>

// Small smoke test: write two small waveform packets to test.zst then read them back
int main(int nargs, char ** args)
{
  const char * fname = nargs > 1 ? args[1] : "test.zst";

  pueo_single_waveform_t wf =  {.run = 123, .event = 456, .wf = { .channel_id = 101, .flags = 0x0f, .length = 16 }};
  for (int i = 0; i < (int)wf.wf.length;i++) wf.wf.data[i] = i;

  pueo_handle_t h;
  if (pueo_handle_init(&h, fname, "w") < 0) {
    fprintf(stderr, "failed to open %s for writing\n", fname);
    return 2;
  }
  pueo_write_single_waveform(&h, &wf);
  wf.event++;
  for (int i = 0; i < (int)wf.wf.length;i++) wf.wf.data[i] = i*2;
  pueo_write_single_waveform(&h, &wf);
  pueo_handle_close(&h);

  // now read back
  if (pueo_handle_init(&h, fname, "r") < 0) {
    fprintf(stderr, "failed to open %s for reading\n", fname);
    return 3;
  }
  int count = 0;
  pueo_single_waveform_t r;
  while (true) {
    int rc = pueo_read_single_waveform(&h, &r);
    if (rc <= 0) break;
    printf("read event=%d\n", r.event);
    count++;
  }
  pueo_handle_close(&h);
  printf("total read=%d\n", count);
  return count == 2 ? 0 : 4;
}
