
/** This is a fake prioritizer that serves an example for how to do this , but in C++!*/


// we need to include <pueo/prio_interface.h> and link against libpueorawdata
#include <pueo/prio_interface.h>
#include <iostream>


// This declares all the functions.
// This is redundant in some sense, but will detect errors if you have the prototypes wrong
// For C++, this also makes sure they get C linkage, which is very important!

PUEO_PRIO_FUNCTIONS(PUEO_PRIO_DECLARE)


// pueo_prio_init should be called on load
int pueo_prio_init(void)
{
   std:: cout << "Called pueo_prio_init" << std::endl;
  return 0;
}

//pueo_prio_configure may be called at any time and set one or more options
int pueo_prio_configure(unsigned N, const pueo_prio_cfg_opt_t * opts)
{
  for (unsigned i = 0; i < N; i++)
  {
     std::cout << "Got opt " << opts[i].key << " (ival, uval, fval) = "  << opts[i].val.ival << "," << opts[i].val.uval << "," <<  opts[i].val.fval << std::endl;
  }
  return 0;
}

// pueo_prio_compute takes N waveforms and fills the result. Right now this is a blocking API (it's called and it will finish when it finishes).
int pueo_prio_compute(unsigned N, const pueo_full_waveforms_t *wfs, pueo_prio_result_t * result)
{
  for (unsigned i = 0; i < N; i++)
  {
    result[i].blast_params[0] = 1;
    result[i].blast_params[1] = 2;
    result[i].blast_params[2] = 3;
    result[i].blast_params[3] = 4;
    result[i].phi = i*i;
    result[i].theta = -i;
    result[i].imp = 0.1;
    result[i].map_peak = 0.2;
    result[i].peak_coherent = 1.2;
    for (unsigned j = 0; j < 192; j++) {
	    result[i].rms[j] = j;
	    result[i].pk2pk[j] = j+1;
	}
  }

  return 0;
}

