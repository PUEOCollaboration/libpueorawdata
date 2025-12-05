
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

  pueo_prio_cfg_opt_t c;

  c.noiseThreshold = 0.158; //if above this, drop into Starlink Bin
  c.tdrsSignalThreshold = 0.138; //if above this, drop into TDRSS Bin
  c.extraSignalThreshold = -1; //Another signal value if desired
  c.rmsThreshold = 16; //if channel RMS exceeds this value, count as "loud"
  c.fullBlastThreshold = 87; //number of channels that must be "loud" to classify as full payload blast

  c.topBlastThreshold = 1.5; //Threshold ratio of pkpk for largest top ring / largest bottom ring
  c.bottomBlastThreshold = 0.5;
  c.frontBackThreshold = 12; //Threshold ratio of pkpk for largest front antenna / largest back antenna
  c.aboveHorizontal = 5; //in deg, sets all signals that reconstruct above this elevation as noise

  c.scRiseTimeLowLim = 0.4; //Lower limit for rise time integration
  c.scRiseTimeUpLim = 0.7; //Upper limit for rise time integration

  c.lowpass = 750; //in MHz, lowpass cutoff
  c.highpass = 100; //in MHz, highpass cutoff

  //For Filter Kernel, all values in MHz. For multiple notches, do "XX, XX, ..."
  c.filterCenters = "370";
  c.filterSpans = "10";


  prio.pueo_prio_configure(&c);

  printf("Prioritizer Configured\n");

  char base_filename[] = "/mnt/storagea/wfs/run0200/0";
  char extension[] = "00.wfs.dat.zstd";
  char wf_file[46]; // Buffer to hold the full filename

  int prior_1=0;
  int prior_2=0;

  int n_in_batch = 100;
  
  pueo_handle_t h;
  for (int i=1; i<35; i++){
    snprintf(wf_file, sizeof(wf_file), "%s%03d%s", base_filename, i, extension);
    
    static pueo_full_waveforms_t all_wfs[PUEO_PRIO_MAX_BATCH] = {0};
    static pueo_prior_Event_t events[PUEO_PRIO_MAX_BATCH];
    
    pueo_handle_init(&h, wf_file, "r");
    int n = 0;
    while (true)
    {
      int read = pueo_read_full_waveforms(&h, &all_wfs[n]);
      //printf("read: %d\n", read);
      if (read <= 0) break;
      memcpy(&events[n].allwfms, &all_wfs[n], sizeof(all_wfs[n]));
      events[n].trigL2 = 0;
      n++;
    }

    for (int run=0; run<1; run++){
      pueo_prio_result_t* results = prio.pueo_prio_compute(events, n_in_batch, c);
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
        }
        if (results[n].priority.signal_level == 2){
          prior_2 += 1;
        }
      }
    }
  }

  printf("Priority 1: %d \n",prior_1);
  printf("Priority 2: %d \n",prior_2); 
  return 0;

}
