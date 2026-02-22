#include "pueo/rawio.h"
#include "FTPair.h" // from nicemc, for making power spectral density plots
#include "ROOT/RDataFrame.hxx"
#include "TCanvas.h"
#include "TStyle.h" // to set CERN ROOT's epoch offset (default is 1990 Jan 1st and not 1970 Jan 1st)
#include <stdio.h>
#include <filesystem>

// note: make sure this file contains pueo_full_waveforms_t and not other packet types
// TODO: maybe add some sort of warning?
void create_root_file(const std::filesystem::path& raw, const std::string& root);


void plot_waterfall(){
  gSystem->Load("${PUEO_UTIL_INSTALL_DIR}/lib64/libNiceMC.so");
  std::filesystem::path rawData("2025-12-31-R005.wfs");

  std::filesystem::create_directory(rawData.stem());
  std::filesystem::path rootified = rawData.stem() / (rawData.stem().string() + ".root");

  // if .root file doesn't exist, create one
  if (!std::filesystem::is_regular_file(rootified)){
    printf("Converting %s to %s...\n", rawData.c_str(), rootified.c_str());
    create_root_file(rawData, rootified);
  }

  // read .root from disk to create an RDataFrame; syntax is (treename, filename)
  ROOT::RDataFrame rdf(rawData.stem().c_str(), rootified.c_str());
  puts("Drawing spectrograms for all channels...\n");

  auto numEvents_ptr = rdf.Count(); // book lazy action
  auto firstReadoutTime_ptr = rdf.Min<uint32_t>("readout time (UTC sec)"); // lazy
  auto lastReadoutTime_ptr = rdf.Max<uint32_t>("readout time (UTC sec)"); //lazy

  printf( // accessing the pointer triggers rdf loop once here and all three lazy results above are now populated
    "The first event is at %u seconds since the UNIX epoch,"
    "and the last event is at %u seconds\n",
    *firstReadoutTime_ptr, *lastReadoutTime_ptr
  );

  // create a vector to book all RDataFrame runs (ie the method call Profile2D() is lazy)
  std::vector<ROOT::RDF::RResultPtr<::TProfile2D>> waterfall_plots;
  std::vector<std::basic_string<char>> channel_column_names = rdf.GetColumnNames();
  channel_column_names.pop_back();
  channel_column_names.pop_back(); // last two columns are "frequency (GHz)" and "readout time (UTC sec)"

  for (auto& ch: channel_column_names){
    waterfall_plots.emplace_back(
      rdf.Profile2D<uint32_t, std::vector<double>>(
        {
          "", Form("%s Spectrogram", ch.c_str()),
          static_cast<int>(*numEvents_ptr), static_cast<double>(*firstReadoutTime_ptr), static_cast<double>(*lastReadoutTime_ptr), 
          100u, 0.0, 1.5
        }, 
        // x-axis is readout time, y is frequency, weight (ie "z-axis") is power (in db)
        "readout time (UTC sec)", "frequency (GHz)", ch.c_str()
      )
    );
  }

  // now actually draw the histograms and save them, should only trigger the rdf loop once
  for (int idx=0; idx<channel_column_names.size(); ++idx){
    std::string this_channel = channel_column_names.at(idx);
    TCanvas c1(this_channel.c_str(), "", 1920, 1080);
    gPad->SetRightMargin(0.13);
    waterfall_plots[idx]->Draw("colz");
    waterfall_plots[idx]->SetStats(kFALSE);
    waterfall_plots[idx]->SetYTitle("Frequency [GHz]");
    gStyle->SetTimeOffset(0); // no offset so that we start from 1970 Jan 1st
    waterfall_plots[idx]->SetXTitle("Time");
    waterfall_plots[idx]->GetXaxis()->SetTimeDisplay(1);
    waterfall_plots[idx]->GetXaxis()->SetTimeFormat("%Y-%m-%d %H:%M:%S");
    waterfall_plots[idx]->GetXaxis()->SetNdivisions(3, kFALSE);
    waterfall_plots[idx]->GetZaxis()->SetTitle("Power [db]");
    waterfall_plots[idx]->GetZaxis()->SetLabelOffset(0.01);
    c1.SaveAs(Form("%s.png", (rootified.parent_path() / this_channel).c_str()));
  }
  printf("The RDataFrame looped this many times: %d\n", rdf.GetNRuns()); // should be 2
}

void create_root_file(const std::filesystem::path& rawDataFileName, const std::string& rootFileName) {

  pueo_handle_t rawDataFileHandle;
  int err = pueo_handle_init(&rawDataFileHandle, rawDataFileName.c_str(), "r");
  if (err < 0) {
    fprintf(stderr, "Bad handle (filename: %s, error code: %d)", rawDataFileName.c_str(), err);
    exit(1);
  }

  TTree myTree(rawDataFileName.stem().c_str(), "");
  std::array<std::vector<double>, PUEO_NCHAN> all_channel_power_spectrum;
  for (auto& vec: all_channel_power_spectrum) vec.reserve(PUEO_MAX_BUFFER_LENGTH);
  std::vector<double> frequency_mesh;
  frequency_mesh.reserve(PUEO_MAX_BUFFER_LENGTH);
  uint32_t readout_time;

  // Create branches; channels 160-167 & 216-223 are the empty channels
  for (int ich=0; ich<160; ++ich){
    myTree.Branch(Form("channel %d", ich), &all_channel_power_spectrum.at(ich));
  }
  for (int ich=168; ich<216; ++ich){
    myTree.Branch(Form("channel %d", ich), &all_channel_power_spectrum.at(ich));
  }
  myTree.Branch("readout time (UTC sec)", &readout_time);
  myTree.Branch("frequency (GHz)", &frequency_mesh);

  pueo_full_waveforms_t fwf;
  TGraph time_domain(PUEO_MAX_BUFFER_LENGTH); // ie length 1024 time domain signal
  TGraph freq_domain;

  // loop over all events in the raw data, remember to clear() the vectors!
  while (true) {
    int read_status = pueo_read_full_waveforms(&rawDataFileHandle, &fwf);
    if (read_status < 0) break;
    for (auto& vec: all_channel_power_spectrum) vec.clear();
    frequency_mesh.clear();

    readout_time = fwf.readout_time.utc_secs;

    // convert each channel's time domain data to a power spectral density TGraph
    for (int ich=0; ich<160; ++ich){

      for (int i = 0; i < PUEO_MAX_BUFFER_LENGTH; i++)
      {
        time_domain.SetPoint(i, i/3., fwf.wfs[ich].data[i]);
      }

      nicemc::FTPair wf_ftpair(time_domain);
      freq_domain = wf_ftpair.makePowerSpectrumGraph();

      for (int i=0; i<freq_domain.GetN(); i++){
        double db = 10 * std::log10(freq_domain.GetPointY(i));
        all_channel_power_spectrum.at(ich).emplace_back(db);
      }
    }

    // copy-pasta :)
    for (int ich=168; ich<216; ++ich){
      for (int i = 0; i < PUEO_MAX_BUFFER_LENGTH; i++)
      {
        time_domain.SetPoint(i, i/3., fwf.wfs[ich].data[i]);
      }

      nicemc::FTPair wf_ftpair(time_domain);
      freq_domain = wf_ftpair.makePowerSpectrumGraph();

      for (int i=0; i<freq_domain.GetN(); i++){
        double db = 10 * std::log10(freq_domain.GetPointY(i));
        all_channel_power_spectrum.at(ich).emplace_back(db);
      }
    }

    for (int i=0; i<freq_domain.GetN(); ++i){
      frequency_mesh.emplace_back(freq_domain.GetPointX(i));
    }

    myTree.Fill();
  }

  ROOT::RDataFrame rdf(myTree);
  rdf.Snapshot(myTree.GetName(), rootFileName.c_str());
}
