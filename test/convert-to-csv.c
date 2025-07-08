#include "pueo/rawdata.h"
#include "pueo/rawio.h"
#include <stdbool.h>
#include <stdio.h>

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
    FILE *fptr;
    fptr = fopen(args[2], "w");

    for (int i=0; i<PUEO_NCHAN; i++) {
      pueo_waveform_t* thiswf =  &(wf.wfs[i]);
      uint16_t ch_samples = thiswf->length;
      for (int j=0; j<ch_samples; j++) {
        //fprintf(fptr, "!");
        fprintf(fptr, "%i",  thiswf->data[j]);
        if (j<ch_samples-1) {
          fprintf(fptr, ", ");
        }
        fflush(fptr);
      }
      fprintf(fptr, "\n");
      fflush(fptr);
    }
  }
  return 0;
}

