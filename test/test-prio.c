
#include <pueo/prio_interface.h>
#include <pueo/rawio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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


  const pueo_prio_cfg_opt_t c = {.noiseThreshold = 0.161, .tdrsSignalThreshold = 0.147, .extraSignalThreshold = -1, .rmsThreshold = 16,
                           .fullBlastThreshold = 20, .topBlastThreshold = 1.5, .bottomBlastThreshold = 0.5, .frontBackThreshold = 6,
                           .aboveHorizontal = 5, .scRiseTimeLowLim = 0.4, .scRiseTimeUpLim = 0.7, .lowpass = 750, .highpass = 100, 
                           .filterCenters = {370,}, .filterSpans = {10,}};
  
  prio.pueo_prio_configure(&c);

  printf("Prioritizer Configured\n");

  char base_filename[] = "/mnt/storagea/wfs/run0579/";
  char extension[] = "00.wfs.dat";
  char wf_file[46]; // Buffer to hold the full filename

  int prior_1=0;
  int prior_2=0;
  int n_in_batch = 100;
  
  pueo_handle_t h;
  for (int i=1; i<100; i++){
    snprintf(wf_file, sizeof(wf_file), "%s%04d%s", base_filename, i, extension);
    
    static pueo_full_waveforms_t all_wfs[PUEO_PRIO_MAX_BATCH] = {0};
    static pueo_prior_Event_t events[PUEO_PRIO_MAX_BATCH];
    
    pueo_handle_init(&h, wf_file, "r");
    int n = 0;
    while (true)
    {
      int read = pueo_read_full_waveforms(&h, &all_wfs[n]);
      //printf("read: %d\n", read);
      if (read <= 0) break;
      //printf("L2 Mask: %d\n", all_wfs[n].L2_mask);
      int val = 0;
      for (int i=0; i<24; i++){
        int mask = ((1<<i) & all_wfs[n].L2_mask);
        if (mask != 0){
          val = i;
        }
      }
      //printf("\n");
      events[n].trigL2 = val;//n % 24;//val;
      memcpy(&events[n].allwfms, &all_wfs[n], sizeof(all_wfs[n]));
      n++;
    }

    for (int run=0; run<1; run++){
      const pueo_prio_result_t* results = prio.pueo_prio_compute(events, n_in_batch);
      if (run == 0) {
        for (int ch=0; ch<192; ch++){
          //printf(" result { ch = %d, .rms = %f, pkpk = %f}\n",ch, results[0].rms[ch],results[0].pk2pk[ch]);
        }
      }
    
      for (int n=0; n<n_in_batch; n++){
        //printf(" result { .phi = %f, .theta = %f, .imp = %f, .map_peak = %f, .rms = %f, .pkpk = %f }\n", results[n].phi, results[n].theta, results[n].imp, results[n].map_peak, results[n].rms[191], results[n].pk2pk[191]);
        //printf(" result { Rise Time Priority = %d, Front/Back Blast = %d, Full Payload Blast = %d, Top Ring Blast = %d, Bottom Ring Blast = %d} \n", results[n].priority.signal_level, results[n].priority.frontback_blast_flag, results[n].priority.fullpayload_blast_flag, results[n].priority.topring_blast_flag, results[n].priority.botring_blast_flag);
        //printf("Rise Time Priority = %d, Az = %f, Al = %f, Rise Time = %f \n", results[n].priority.signal_level, results[n].phi, results[n].theta, results[n].imp);
        if (results[n].priority.signal_level == 1){
          prior_1 += 1;
          printf("Prior 1 File: %d\n", i);
          printf("Event: %d\n", n);
          printf("Phi: %f\n",results[n].phi);
          printf("Theta: %f\n",results[n].theta);
        }
        if (results[n].priority.signal_level == 2){
          prior_2 += 1;
          printf("File: %d\n",i);
          printf("Event: %d\n",n);
          printf("Phi: %f\n",results[n].phi);
          printf("Theta: %f\n",results[n].theta);
          printf("Full Blast: %d\n", results[n].priority.fullpayload_blast_flag);
          printf("Front/Back Blast: %d\n", results[n].priority.frontback_blast_flag);
        }
      }
    }

    pueo_handle_close(&h);
  }

  printf("Priority 1: %d \n",prior_1);
  printf("Priority 2: %d \n",prior_2); 
  return 0;

}
