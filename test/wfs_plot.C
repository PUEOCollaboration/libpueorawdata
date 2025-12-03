
R__LOAD_LIBRARY(build/libpueorawdata.so);

#include "pueo/rawdata.h"
#include "pueo/rawio.h"

void wfs_plot(const char * f, int index = 0, bool plot = true, const char * save = "plot.pdf")
{

  gStyle->SetLineScalePS(1);
  pueo_handle_t h;
  pueo_handle_init(&h, f,"r");

  pueo_full_waveforms_t wfs;
  for (int i = 0; i <= index ; i++)
  {
    pueo_read_full_waveforms(&h, &wfs);
  }

  printf("Subsecond: %f\n",  double(wfs.event_time - wfs.last_pps) / double(wfs.last_pps - wfs.llast_pps));
  int idx = 0;
  if (!plot) return;

  int iplot = 0;
  for (int isurf = 0; isurf < 28; isurf++)
  {
    TCanvas * c = new TCanvas(Form("c%d",isurf),Form("LINK %d, SURF %d", isurf/7, isurf % 7), 1800, 1000);
    c->Divide(4,2,0.001,0.001);

    for (int ichan = 0; ichan < 8; ichan++)
    {
      c->cd(ichan+1);
      TGraph * g = new TGraph(wfs.wfs[idx].length);
      g->SetEditable(false);
      int idx = isurf*8+ichan;
      for (int i = 0; i < g->GetN(); i++)
      {
        g->SetPoint(i,i/3., wfs.wfs[idx].data[i]);
      }
      g->SetTitle(Form("Link %d Slot %d CH %d, RMS=%f;ns;adc", isurf / 7, isurf % 7, ichan, g->GetRMS(2)));
      g->GetYaxis()->SetTitleOffset(1.7);
      g->GetYaxis()->CenterTitle();
      g->GetXaxis()->CenterTitle();
      g->GetXaxis()->SetRangeUser(0,1024/3.);
      g->Draw("al");

      gPad->SetRightMargin(0.05);
      gPad->SetLeftMargin(0.15);
      gPad->SetGridx();
      gPad->SetGridy();
      idx++;

     }
     if (isurf % 7< 6 )
     {
        if (save) c->SaveAs(Form("plot.pdf%s", iplot == 0 ? "(" : iplot == 23 ? ")" : ""));
        iplot++;
     }

    c->Show();
  }
  printf("\n");

}

