
R__LOAD_LIBRARY(build/libpueorawdata.so);

#include "pueo/rawdata.h"
#include "pueo/rawio.h"

void wfs_plot(const char * f, int index = 0)
{

  pueo_handle_t h;
  pueo_handle_init(&h, f,"r");

  pueo_full_waveforms_t wfs;
  for (int i = 0; i <= index ; i++)
  {
    pueo_read_full_waveforms(&h, &wfs);
  }

  int idx = 0;

  for (int isurf = 0; isurf < 28; isurf++)
  {
    TCanvas * c = new TCanvas(Form("c%d",isurf),Form("LINK %d, SURF %d", isurf/7, isurf % 7), 1800, 1000);
    c->Divide(4,2,0.001,0.001);

    for (int ichan = 0; ichan < 8; ichan++)
    {
      c->cd(ichan+1);
      TGraph * g = new TGraph(wfs.wfs[idx].length);
      int idx = isurf*8+ichan;
      for (int i = 0; i < g->GetN(); i++)
      {
        g->SetPoint(i,i/3., wfs.wfs[idx].data[i]);
      }
      g->SetTitle(Form("RMS=%f;ns;adc", g->GetRMS(2)));
      g->GetYaxis()->SetTitleOffset(1.7);
      g->GetYaxis()->CenterTitle();
      g->GetXaxis()->CenterTitle();
      g->GetXaxis()->SetRangeUser(0,1024/3.);
      g->Draw("alp");

      gPad->SetRightMargin(0.05);
      gPad->SetLeftMargin(0.15);
      gPad->SetGridx();
      gPad->SetGridy();
      idx++;
    }
    c->Show();
  }
  printf("\n");

}

