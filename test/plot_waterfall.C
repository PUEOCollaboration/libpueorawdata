#include "pueo/rawio.h"
#include "FTPair.h"
#include "TGraph.h"
#include "TH2D.h"
#include "TCanvas.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


void plot_waterfall(){

  pueo_handle_t fwf_file_handle;
  char * filename = NULL;
  asprintf(&filename, "2025-12-31-R005.wfs");
  int handleinit = pueo_handle_init(&fwf_file_handle, filename, "r");
  if (handleinit < 0) {
    fprintf(stderr, "Bad handle (filename: %s, error code: %d)", filename, handleinit);
    free(filename);
    exit(1);
  }

  pueo_full_waveforms_t fwf;
  struct tm* time_utc = NULL; // don't free
  // while (true) {
    int read_status = pueo_read_full_waveforms(&fwf_file_handle, &fwf);
    if (read_status < 0) {
      // break;
    }
  
    const time_t readout_time  = fwf.readout_time.utc_secs;
    time_utc = gmtime(&readout_time);
    char time_buffer[24];
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S UTC", time_utc);

    TGraph time_domain(fwf.wfs[0].length);
    for (int i = 0; i < time_domain.GetN(); i++)
    {
      time_domain.SetPoint(i, i/3., fwf.wfs[0].data[i]);
    }

    time_domain.SetTitle(
      Form("Run %d, Event %d, CH %d, %s", fwf.run, fwf.event, fwf.wfs[0].channel_id, time_buffer)
    );

    nicemc::FTPair wf_ftpair(time_domain);
    TGraph freq_domain = wf_ftpair.makePowerSpectrumGraph();
    for (int i=0; i<freq_domain.GetN(); i++){
      double power_i = freq_domain.GetPointY(i);
      double db = 10 * std::log10(power_i);
      freq_domain.SetPointY(i, db);
    }

    TCanvas c1("", "", 1920, 1080);
    TH2D waterfall_hist("", Form("Run %u", fwf.run), 100, 0, 100, 100, 0, 100);
    waterfall_hist.Draw("colz");
    c1.SaveAs("foobar.svg");
  // }

  free(filename);
}
