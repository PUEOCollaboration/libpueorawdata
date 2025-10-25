
#include <pueo/prio_interface.h>
#include <pueo/rawio.h>
#include <stdio.h>
#include <stdlib.h>

#define BATCH_SIZE 

int main(int nargs, char ** args)
{

  if (nargs < 3)
  {
    fprintf(stderr,"test-prio prio.so  wffile.dat");
    return 1;
  }

  const char * sofile = args[1];
  const char * wf_file = args[2];

  pueo_prio_impl_t prio;
  pueo_prio_create(sofile, &prio);

  pueo_handle_t h;
  pueo_handle_init(&h, wf_file, "r");

  pueo_full_waveforms_t wfs[PUEO_PRIO_MAX_BATCH];
  pueo_prio_result_t results[PUEO_PRIO_MAX_BATCH];


  int n_in_batch = 0;

  prio.pueo_prio_init();
  pueo_prio_cfg_opt_t opts[] = {
    { .key = "max-batch", .val = {.uval = PUEO_PRIO_MAX_BATCH } }
  };
  prio.pueo_prio_configure( 1, opts);

  while (true)
  {

    int rd = pueo_read_full_waveforms(&h, &wfs[n_in_batch]);

    if (rd > 0) n_in_batch++;

    if (rd <= 0 || n_in_batch == PUEO_PRIO_MAX_BATCH)
    {

      if (n_in_batch)
      {
        prio.pueo_prio_compute(n_in_batch, wfs, results);

        for (int i = 0; i < n_in_batch; i++)
        {
          printf(" result { .phi = %f, .theta = %f, .imp = %f, .map_peak = %f }\n", results[i].phi, results[i].theta, results[i].imp, results[i].map_peak);
        }
        n_in_batch = 0;
      }

      if (rd <= 0) break;
    }
  }

  return 0;

}
