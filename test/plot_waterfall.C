#include "pueo/rawio.h"
#include "FTPair.h"
#include "ROOT/RDataFrame.hxx"
#include "TCanvas.h"
#include <stdio.h>
#include <stdlib.h>

namespace { // because we love globals
  const ROOT::RDF::ColumnNames_t branchNames{"readout time (utc sec)", "power (db)", "frequency (GHz)"};
  const char treename[] = "powerSpectrum tree";
}

// horrendously slow and probably uses a good amount of memory, but oh well
void create_root_file_using_RDataFrame(const std::string& rawDataFileName, const std::string& rootFileName) {

  pueo_handle_t rawDataFileHandle; // make sure this file contains pueo_full_waveforms_t and not other packet types
  int err = pueo_handle_init(&rawDataFileHandle, rawDataFileName.c_str(), "r");
  if (err < 0) {
    fprintf(stderr, "Bad handle (filename: %s, error code: %d)", rawDataFileName.c_str(), err);
    exit(1);
  }

  pueo_full_waveforms_t fwf;
  TTree fwf_tree(treename, "");

  uint32_t readoutTime;
  std::vector<double> power_db, freq_ghz;
  fwf_tree.Branch(branchNames[0].c_str(), &readoutTime);
  fwf_tree.Branch(branchNames[1].c_str(), &power_db);
  fwf_tree.Branch(branchNames[2].c_str(), &freq_ghz);

  while (true) {
    int read_status = pueo_read_full_waveforms(&rawDataFileHandle, &fwf);
    if (read_status < 0) break;

    freq_ghz.clear();
    power_db.clear();
    readoutTime = fwf.readout_time.utc_secs;

    TGraph time_domain(fwf.wfs[53].length);
    for (int i = 0; i < time_domain.GetN(); i++)
    {
      time_domain.SetPoint(i, i/3., fwf.wfs[53].data[i]);
    }

    nicemc::FTPair wf_ftpair(time_domain);
    TGraph freq_domain = wf_ftpair.makePowerSpectrumGraph();

    for (int i=0; i<freq_domain.GetN(); i++){
      double db = 10 * std::log10(freq_domain.GetPointY(i));
      power_db.emplace_back(db);
      freq_ghz.emplace_back(freq_domain.GetPointX(i));
    }

    fwf_tree.Fill();
  }

  ROOT::RDataFrame rdf(fwf_tree, branchNames);
  rdf.Snapshot(treename, rootFileName);
}

void plot_waterfall(){
  std::string rootFileName("full_waveform.root");
  // create_root_file_using_RDataFrame("2025-12-31-R005.wfs", rootFileName);

  ROOT::RDataFrame rdf(treename, rootFileName); 

  auto numEvents_ptr = rdf.Count(); // lazy
  auto firstReadoutTime_ptr = rdf.Min(branchNames[0]); // lazy
  auto lastReadoutTime_ptr = rdf.Max(branchNames[0]); //lazy
  
  auto waterfall = rdf.Histo2D<std::vector<double>, std::vector<double>>(
    {
      "", "Ch 54 (Known Chirping Channel) Spectrogram", 
      // accessing the pointer triggers rdf loop once here
      static_cast<int>(*numEvents_ptr), static_cast<double>(*firstReadoutTime_ptr), static_cast<double>(*lastReadoutTime_ptr), 
      100u, 0.0, 1.5
    }, 
    // x-axis is readout time, y is frequency, weight is power
    branchNames[0], branchNames[2], branchNames[1]
  );

  TCanvas c1("", "", 1920, 1080);
  waterfall->Draw("colz");
  waterfall->SetStats(kFALSE);
  waterfall->SetXTitle("Unix Epoch [seconds])");
  waterfall->SetYTitle("Frequency [GHz]");
  waterfall->GetZaxis()->SetTitle("Power [db]");
  waterfall->GetZaxis()->SetLabelOffset(0.01);
  gPad->SetRightMargin(0.13);
  c1.SaveAs("foobar.svg");
}
