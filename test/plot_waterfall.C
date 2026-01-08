#include "pueo/rawio.h"
#include "FTPair.h"
#include "ROOT/RDataFrame.hxx"
#include <stdio.h>
#include <stdlib.h>

namespace { // because we love globals
  const ROOT::RDF::ColumnNames_t branchNames{"readout time (utc sec)", "power spectrum (db)"};
}

// horrendously slow, but oh well
void save_power_spectrum_using_RDataFrame(const std::string& rawDataFileName, const std::string& rootFileName) {

  pueo_handle_t rawDataFileHandle; // make sure this file contains pueo_full_waveforms_t and not other packet types
  int err = pueo_handle_init(&rawDataFileHandle, rawDataFileName.c_str(), "r");
  if (err < 0) {
    fprintf(stderr, "Bad handle (filename: %s, error code: %d)", rawDataFileName.c_str(), err);
    exit(1);
  }

  pueo_full_waveforms_t fwf;
  const char treename[] = "waveform tree";
  TTree fwf_tree(treename, "");

  uint32_t readoutTime;
  TGraph powerSPectrum;
  fwf_tree.Branch(branchNames[0].c_str(), &readoutTime);
  fwf_tree.Branch(branchNames[1].c_str(), &powerSPectrum);

  while (true) {
    int read_status = pueo_read_full_waveforms(&rawDataFileHandle, &fwf);
    if (read_status < 0) break;
  
    readoutTime = fwf.readout_time.utc_secs;

    TGraph time_domain(fwf.wfs[0].length);
    for (int i = 0; i < time_domain.GetN(); i++)
    {
      time_domain.SetPoint(i, i/3., fwf.wfs[0].data[i]);
    }

    nicemc::FTPair wf_ftpair(time_domain);
    powerSPectrum = wf_ftpair.makePowerSpectrumGraph();
    for (int i=0; i<powerSPectrum.GetN(); i++){
      double db = 10 * std::log10(powerSPectrum.GetPointY(i));
      powerSPectrum.SetPointY(i, db);
    }

    fwf_tree.Fill();
  }

  ROOT::RDataFrame rdf(fwf_tree, branchNames);
  rdf.Snapshot(treename, rootFileName);
}

void plot_waterfall(){
  std::string rootFileName("full_waveform.root");
  save_power_spectrum_using_RDataFrame("2025-12-31-R005.wfs", rootFileName);
}
