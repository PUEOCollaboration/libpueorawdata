#include "pueo/rawio.h"
#include "FTPair.h"
#include "ROOT/RDataFrame.hxx"
#include "TCanvas.h"
#include <stdio.h>
#include <filesystem>

// note: make sure this file contains pueo_full_waveforms_t and not other packet types
// TODO: maybe add some sort of warning?
void create_root_file(const std::filesystem::path& rawDataFileName) {

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
  rdf.Snapshot(myTree.GetName(), rawDataFileName.filename().replace_extension(".root").c_str());
}

void plot_waterfall(){
  std::filesystem::path rawDataFile("2025-12-31-R005.wfs");
  std::filesystem::path rootFileName = rawDataFile.filename().replace_extension(".root");

  // if .root file doesn't exist, create one
  if (!std::filesystem::is_regular_file(rootFileName))
    create_root_file(rawDataFile);

  // read .root from disk to create an RDataFrame; syntax is (treename, filename)
  ROOT::RDataFrame rdf(rootFileName.stem().c_str(), rootFileName.c_str());

  auto numEvents_ptr = rdf.Count(); // lazy
  auto firstReadoutTime_ptr = rdf.Min("readout time (UTC sec)"); // lazy
  auto lastReadoutTime_ptr = rdf.Max("readout time (UTC sec)"); //lazy

  std::vector<ROOT::RDF::RResultPtr<::TProfile2D>> waterfall_plots;

  std::vector<std::basic_string<char>> channel_column_names = rdf.GetColumnNames();
  channel_column_names.pop_back();
  channel_column_names.pop_back(); // last two columns are "frequency (GHz)" and "readout time (UTC sec)"

  for (auto& ch: channel_column_names){
    waterfall_plots.emplace_back(
      rdf.Profile2D<uint32_t, std::vector<double>>(
        {
          "", Form("%s Spectrogram", ch.c_str()),
          // accessing the pointer triggers rdf loop once here
          static_cast<int>(*numEvents_ptr), static_cast<double>(*firstReadoutTime_ptr), static_cast<double>(*lastReadoutTime_ptr), 
          100u, 0.0, 1.5
        }, 
        // x-axis is readout time, y is frequency, weight is power
        "readout time (UTC sec)", "frequency (GHz)", ch.c_str()
      )
    );
  }

  TCanvas c1("", "", 1920, 1080);
  c1.Divide(1,3);
  c1.cd(1);
  waterfall_plots[0]->Draw("colz");
  waterfall_plots[0]->SetStats(kFALSE);
  waterfall_plots[0]->SetXTitle("Unix Epoch [seconds])");
  waterfall_plots[0]->SetYTitle("Frequency [GHz]");
  waterfall_plots[0]->GetZaxis()->SetTitle("Power [db]");
  waterfall_plots[0]->GetZaxis()->SetLabelOffset(0.01);
  gPad->SetRightMargin(0.13);
  c1.cd(2);
  waterfall_plots[1]->Draw("colz");
  waterfall_plots[1]->SetStats(kFALSE);
  waterfall_plots[1]->SetXTitle("Unix Epoch [seconds])");
  waterfall_plots[1]->SetYTitle("Frequency [GHz]");
  waterfall_plots[1]->GetZaxis()->SetTitle("Power [db]");
  waterfall_plots[1]->GetZaxis()->SetLabelOffset(0.01);
  gPad->SetRightMargin(0.13);
  c1.cd(3);
  waterfall_plots[2]->Draw("colz");
  waterfall_plots[2]->SetStats(kFALSE);
  waterfall_plots[2]->SetXTitle("Unix Epoch [seconds])");
  waterfall_plots[2]->SetYTitle("Frequency [GHz]");
  waterfall_plots[2]->GetZaxis()->SetTitle("Power [db]");
  waterfall_plots[2]->GetZaxis()->SetLabelOffset(0.01);
  gPad->SetRightMargin(0.13);
  c1.SaveAs("foobar.svg");

  printf("ran this many times: %d", rdf.GetNRuns());
}
