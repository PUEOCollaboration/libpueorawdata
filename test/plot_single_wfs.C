
R__LOAD_LIBRARY(build/libpueorawdata.so);

#include "pueo/rawdata.h"
#include "pueo/rawio.h"

void plot_single_wfs(const char * infile, const char * out_file)
{

  gStyle->SetLineScalePS(1);
  pueo_handle_t h;
  pueo_handle_init(&h, infile,"r");

  pueo_single_waveform_t wf;
  if (pueo_read_single_waveform(&h, &wf) <= 0)
  {
    printf("Hmm... didn't get any bytes?\n");
    return;
  }

  double subs = double(wf.event_time - wf.last_pps) / double(wf.last_pps - wf.llast_pps);
  printf("Subsecond: %f\n", subs );

  TCanvas * c = new TCanvas("c","c", 1800, 1000);
  c->cd();

  TGraph * g = new TGraph(wf.wf.length);
  for (int i = 0; i < g->GetN(); i++)
  {
    g->SetPoint(i,i/3., wf.wf.data[i]);
  }

  g->SetTitle(Form("Run %d Event %d CH %d (subsecond=%f), RMS=%f;ns;adc", wf.run, wf.event, wf.wf.channel_id, subs, g->GetRMS(2)));
  g->GetYaxis()->CenterTitle();
  g->GetXaxis()->CenterTitle();
  g->GetXaxis()->SetRangeUser(0,1024/3.);
  g->Draw("al");

  gPad->SetGridx();
  gPad->SetGridy();
  if (out_file)
  {
    c->SaveAs(out_file);
  }

}

