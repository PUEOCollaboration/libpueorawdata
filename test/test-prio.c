
#include <../inc/pueo/prio_interface.h>
#include <../inc/pueo/rawio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <valgrind/callgrind.h>

#define BATCH_SIZE 

int main(int nargs, char ** args)
{

  pueo_prio_impl_t prio;
  const char * sofile = "/home/pueo/prioritizerTesting-codeCleanUp/build/libpueoprior.so";
  if (pueo_prio_create(sofile, &prio))
  {
    fprintf(stderr,"Coudln't load prio from %s\n", sofile);
    return 1;
  }

  prio.pueo_prio_init();

  printf("Prioritizer Initialized\n");

  pueo_prio_cfg_opt_t c = prio.pueo_prio_configure();

  printf("Prioritizer Configured\n");

  const char * wf_file = "../rachel4.wfs";///mnt/storagea/wfs/run0138/002000.wfs.dat.zstd";
  pueo_handle_t h;
  pueo_handle_init(&h, wf_file, "r");

  static pueo_full_waveforms_t all_wfs[PUEO_PRIO_MAX_BATCH] = {0};
  //pueo_prio_result_t * results[PUEO_PRIO_MAX_BATCH];
  static pueo_prior_Event_t events[PUEO_PRIO_MAX_BATCH];

  int n_in_batch = 100;
  int n = 0;
  while (true)
  {
    int read = pueo_read_full_waveforms(&h, &all_wfs[n]);
    //printf("read: %d\n", read);
    if (read <= 0) break;
    //pueo_dump_full_waveforms(stdout,&all_wfs[0]);
    memcpy(&events[n].allwfms, &all_wfs[n], sizeof(all_wfs[n]));
    //printf("%d\n",all_wfs[0].wfs[12].data[100]);
    events[n].trigL2 = 0;
    n++;
  }

  /*for (int n=0; n<n_in_batch; n++){
      int read = pueo_read_full_waveforms(&h, &all_wfs[0]);
      printf("read: %d\n", read);
      pueo_dump_full_waveforms(stdout, &all_wfs[0]);
      memcpy(&events[n].allwfms, &all_wfs[n], sizeof(all_wfs[n]));
      printf("%d\n",all_wfs[n].wfs[12].data[100]);
      events[n].trigL2 = 0;
    }*/
  
  //printf("%d\n",events[0].trigL2);
  
  for (int run=0; run<1; run++){
    pueo_prio_result_t* results = prio.pueo_prio_compute(events, n_in_batch, c);
    if (run == 0) {
      for (int ch=0; ch<192; ch++){
        //printf(" result { ch = %d, .rms = %f, pkpk = %f}\n",ch, results[0].rms[ch],results[0].pk2pk[ch]);
      }
    }
  }
  
  for (int n=0; n<n_in_batch; n++){
    //printf(" result { .phi = %f, .theta = %f, .imp = %f, .map_peak = %f, .rms = %f, .pkpk = %f }\n", results[n].phi, results[n].theta, results[n].imp, results[n].map_peak, results[n].rms[191], results[n].pk2pk[191]);
  }  
  
  for (int ch=0; ch<192; ch++){
    //printf(" result { ch = %d, .rms = %f, pkpk = %f}\n",ch, results[0].rms[ch],results[0].pk2pk[ch]);
  }
  /*if (nargs < 3)
  {
    fprintf(stderr,"test-prio prio.so  wffile.dat");
    return 1;
  }

  const char * sofile = args[1];
  const char * wf_file = args[2];

  pueo_prio_impl_t prio;
  if (pueo_prio_create(sofile, &prio))
  {
    fprintf(stderr,"Coudln't load prio from %s\n", sofile);
    return 1;
  }

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
  }*/

  return 0;

}
