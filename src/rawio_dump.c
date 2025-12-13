/* here we have the dumpers for each data type*/

#include "float16_guard.h"
#include <stdio.h>
#include "pueo/rawio.h"
#include <string.h>
#include "pueo/sensor_ids.h"


const char * dump_tabs="\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
static __thread int dump_ntabs = 0;



//MACROS ARE ALWAYS A GOOD IDEA

#define DUMPINIT(f)  int  ret = 0; FILE * __F = f;
#define DUMPSTART(what) ret+=fprintf(__F,"%.*s \"%s\" : {\n", dump_ntabs++, dump_tabs, what)
#define DUMPSTARTARR(what) ret+=fprintf(__F,"%.*s \"%s\" : [\n", dump_ntabs++, dump_tabs, what)
#define DUMPSTARTARROBJ() ret+=fprintf(__F,"%.*s {\n", dump_ntabs++, dump_tabs)
#define DUMPENDARROBJ() ret+=fprintf(__F,"%.*s },\n", dump_ntabs--, dump_tabs)
#define DUMPKEYVAL(key,frmt,...) ret+=fprintf(__F, "%.*s\"" key "\": " frmt ",\n", dump_ntabs, dump_tabs, __VA_ARGS__)
#define DUMPTIME(x,wut) DUMPKEYVAL(#wut,"%lu.%09u", (uint64_t) x->wut.utc_secs, (uint32_t) x->wut.utc_nsecs)
#define DUMPVAL(x,wut,frmt) DUMPKEYVAL(#wut,frmt, x->wut)
#define DUMPU32(x,wut) DUMPVAL(x,wut,"%u")
#define DUMPU64(x,wut) DUMPVAL(x,wut,"%lu")
#define DUMPU128(x,wut) DUMPVAL(x,wut,"%llu")
#define DUMPU16(x,wut) DUMPVAL(x,wut,"%hu")
#define DUMPI16(x,wut) DUMPVAL(x,wut,"%hd")
#define DUMPU8(x,wut) DUMPVAL(x,wut,"%hhu")
#define DUMPX8(x,wut) DUMPVAL(x,wut,"0x%hhx")
#define DUMPX16(x,wut) DUMPVAL(x,wut,"0x%hx")
#define DUMPX32(x,wut) DUMPVAL(x,wut,"0x%x")
#define DUMPFLT(x,wut) DUMPVAL(x,wut,"%f")
#define DUMPSTR(x,wut) DUMPVAL(x,wut,"\"%s\"")
#define DUMPBOOL(x,wut) DUMPKEYVAL(#wut, "%s", x->wut ? "true" : "false");
#define DUMPARRAY(x,wut,N,frmt) ret+=fprintf(__F,"\%.*s\""#wut"\": [", dump_ntabs, dump_tabs); for (int asdf = 0; asdf < (int) N; asdf++) ret+=fprintf(f," "frmt"%c",x->wut[asdf], asdf== (int) (N-1) ? ' ' : ','); ret+=fprintf(f,"],\n")
#define DUMPEND() ret+=fprintf(__F,"\%.*s}\n", --dump_ntabs, dump_tabs);
#define DUMPENDARR() ret+=fprintf(__F,"\%.*s]\n", --dump_ntabs, dump_tabs)
#define DUMPFINISH() fflush(__F); return ret;


static int pueo_dump_waveform(FILE *f, const pueo_waveform_t * wf)
{
  DUMPINIT(f)
  DUMPSTART("waveform");
  DUMPU8(wf,channel_id);
  DUMPX8(wf,surf_word);
  DUMPU16(wf,length);
  DUMPARRAY(wf,data,wf->length,"%hd");
  DUMPEND();
  DUMPFINISH();
}

int pueo_dump_single_waveform(FILE *f, const pueo_single_waveform_t * wf)
{
  DUMPINIT(f)
  DUMPSTART("single_waveform");
  DUMPU32(wf,run);
  DUMPU32(wf,event);
  DUMPU32(wf,event_second);
  DUMPU32(wf,event_time);
  DUMPU32(wf,last_pps);
  DUMPU32(wf,llast_pps);

  DUMPX32(wf,deadtime_counter);
  DUMPX32(wf,deadtime_counter_last_pps);
  DUMPX32(wf,deadtime_counter_llast_pps);
  DUMPX32(wf,L2_mask);
  DUMPBOOL(wf,soft_trigger);
  DUMPBOOL(wf,pps_trigger);
  DUMPBOOL(wf,ext_trigger);

  ret+=pueo_dump_waveform(f,&wf->wf);
  DUMPEND();
  DUMPFINISH();
}

int pueo_dump_full_waveforms(FILE *f, const pueo_full_waveforms_t * wf)
{
  DUMPINIT(f)
  DUMPSTART("full_waveform");
  DUMPU32(wf,run);
  DUMPU32(wf,event);
  DUMPU32(wf,event_second);
  DUMPU32(wf,event_time);
  DUMPU32(wf,last_pps);
  DUMPU32(wf,llast_pps);

  DUMPX32(wf,deadtime_counter);
  DUMPX32(wf,deadtime_counter_last_pps);
  DUMPX32(wf,deadtime_counter_llast_pps);
  DUMPX32(wf,L2_mask);
  DUMPBOOL(wf,soft_trigger);
  DUMPBOOL(wf,pps_trigger);
  DUMPBOOL(wf,ext_trigger);


  for (int i = 0; i < PUEO_NCHAN; i++)
  {
    ret+=pueo_dump_waveform(f,&wf->wfs[i]);
  }
  DUMPEND();
  DUMPFINISH()
}



int pueo_dump_sensors_telem(FILE *f, const pueo_sensors_telem_t *t)
{
  DUMPINIT(f);
  DUMPSTART("sensors_telem");
  DUMPU32(t,timeref_secs);
  DUMPU32(t,sensor_id_magic);
  DUMPU32(t,num_packets);
  DUMPSTARTARR("sensors");
  for (int i = 0; i < t->num_packets; i++)
  {
    const pueo_sensor_telem_t * s = &t->sensors[i];
    DUMPSTART("pueo_sensor_telem");
    DUMPU16(s,sensor_id);
    DUMPKEYVAL("decoded_sensor_subsystem","%s",pueo_sensor_id_get_compat_subsystem(t->sensors[i].sensor_id, t->sensor_id_magic));
    DUMPKEYVAL("decoded_sensor_name","%s",pueo_sensor_id_get_compat_name(t->sensors[i].sensor_id, t->sensor_id_magic));
    DUMPKEYVAL("decoded_sensor_type","%c",pueo_sensor_id_get_compat_type_tag(t->sensors[i].sensor_id, t->sensor_id_magic));
    DUMPKEYVAL("decoded_sensor_kind","%c",pueo_sensor_id_get_compat_kind(t->sensors[i].sensor_id, t->sensor_id_magic));
    DUMPI16(s,relsecs);

    switch(pueo_sensor_id_get_compat_type_tag(t->sensors[i].sensor_id, t->sensor_id_magic))
    {
      case 'F':
        DUMPKEYVAL("value", "%f", (double) t->sensors[i].val.fval); break;
      case 'U':
        DUMPKEYVAL("value", "%hu", t->sensors[i].val.uval); break;
      case 'I':
      default: 
        DUMPKEYVAL("value", "%hd", t->sensors[i].val.ival); break;
    }
    DUMPEND();
  }
  DUMPENDARR();
  DUMPEND();
  DUMPFINISH();
}

int pueo_dump_sensors_disk(FILE *f, const pueo_sensors_disk_t *t)
{
  DUMPINIT(f)
  DUMPSTART("sensors_disk");
  DUMPU32(t,sensor_id_magic);
  DUMPU32(t,num_packets);
  DUMPSTARTARR("sensors");
  for (int i = 0; i < t->num_packets; i++)
  {
    const pueo_sensor_disk_t * s = &t->sensors[i];
    DUMPSTART("pueo_sensor_disk");
    DUMPU16(s,sensor_id);
    DUMPKEYVAL("decoded_sensor_subsystem","%s",pueo_sensor_id_get_compat_subsystem(t->sensors[i].sensor_id, t->sensor_id_magic));
    DUMPKEYVAL("decoded_sensor_name","%s",pueo_sensor_id_get_compat_name(t->sensors[i].sensor_id, t->sensor_id_magic));
    DUMPKEYVAL("decoded_sensor_type","%c",pueo_sensor_id_get_compat_type_tag(t->sensors[i].sensor_id, t->sensor_id_magic));
    DUMPKEYVAL("decoded_sensor_kind","%c",pueo_sensor_id_get_compat_kind(t->sensors[i].sensor_id, t->sensor_id_magic));
    DUMPU32(s,time_secs);
    DUMPU16(s,time_ms);

    switch(pueo_sensor_id_get_compat_type_tag(t->sensors[i].sensor_id, t->sensor_id_magic))
    {
      case 'F':
        DUMPKEYVAL("value", "%f",(double) t->sensors[i].val.fval); break;
      case 'U':
        DUMPKEYVAL("value", "%u",t->sensors[i].val.uval); break;
      case 'I':
      default: 
        DUMPKEYVAL("value", "%d",t->sensors[i].val.ival); break;
    }
    DUMPEND();
  }
  DUMPENDARR();
  DUMPEND();
  DUMPFINISH();
}

int pueo_dump_nav_att(FILE *f, const pueo_nav_att_t *n)
{
  DUMPINIT(f);
  DUMPSTART(n->source == PUEO_NAV_CPT7 ?"nav_att_cpt7" :
            n->source == PUEO_NAV_BOREAS ? "nav_att_boreas" :
            n->source == PUEO_NAV_ABX2 ? "nav_att_abx2" : "nav_att_unknown");
  DUMPTIME(n, readout_time);
  DUMPTIME(n, gps_time);
  DUMPFLT(n,lat);
  DUMPFLT(n,lon);
  DUMPFLT(n,alt);
  DUMPFLT(n,heading);
  DUMPFLT(n,pitch);
  DUMPFLT(n,roll);
  DUMPFLT(n,heading_sigma);
  DUMPFLT(n,pitch_sigma);
  DUMPFLT(n,roll_sigma);
  DUMPFLT(n,hdop);
  DUMPFLT(n,vdop);
  DUMPU8(n,nsats);
  DUMPX8(n,flags);
  DUMPI16(n,temperature);
  DUMPARRAY(n,antenna_currents,3,"%hd");
  DUMPEND();
  DUMPFINISH();
}

int pueo_dump_nav_pos(FILE *f, const pueo_nav_pos_t *n)
{
  DUMPINIT(f);
  DUMPSTART(n->source == PUEO_NAV_CPT7 ?"nav_pos_cpt7" :
            n->source == PUEO_NAV_BOREAS ? "nav_pos_boreas" :
            n->source == PUEO_NAV_ABX2 ? "nav_pos_abx2" : 
            n->source == PUEO_NAV_TURF ? "nav_pos_turf" : 
            "nav_pos_unknown");
  DUMPTIME(n, readout_time);
  DUMPTIME(n, gps_time);
  DUMPFLT(n,lat);
  DUMPFLT(n,lon);
  DUMPFLT(n,alt);
  DUMPARRAY(n,x,3,"%f");
  DUMPARRAY(n,v,3,"%f");
  DUMPFLT(n,hdop);
  DUMPFLT(n,vdop);
  DUMPU8(n,nsats);
  DUMPX8(n,flags);
  DUMPEND();
  DUMPFINISH();
}

int pueo_dump_slow(FILE *f, const pueo_slow_t * s)
{
  DUMPINIT(f);
  DUMPSTART("slow");
  DUMPU16(s,ncmds);
  DUMPU16(s,time_since_last_cmd);
  DUMPU8(s,last_cmd);
  DUMPU32(s,sipd_uptime);
  DUMPU32(s,cpu_time);
  DUMPU32(s,cpu_uptime);
  DUMPU32(s,can_ping_world);
  DUMPU32(s,starlink_on);
  DUMPU32(s,los_on);
  DUMPU16(s,current_run);
  DUMPU16(s,current_run_secs);
  DUMPU32(s,current_run_events);

  DUMPEND();
  DUMPFINISH();
}

int pueo_dump_ss(FILE *f, const pueo_ss_t * s)
{
  DUMPINIT(f);
  DUMPSTART("ss");
  DUMPSTARTARR("ss");
  for (int i = 0; i < PUEO_SS_NUM_SENSORS; i++)
  {
    DUMPSTARTARROBJ();
    DUMPKEYVAL("idx", "%d", i);
    DUMPKEYVAL("x1", "%u", (uint32_t) s->ss[i].x1);
    DUMPKEYVAL("x2", "%u", (uint32_t) s->ss[i].x2);
    DUMPKEYVAL("y1", "%u", (uint32_t) s->ss[i].y1);
    DUMPKEYVAL("y2", "%u", (uint32_t) s->ss[i].y2);
    DUMPKEYVAL("tempADS1220_raw", "%hu",  (uint16_t) s->ss[i].tempADS1220);
    DUMPKEYVAL("tempSS_raw", "%hu",  (uint16_t) s->ss[i].tempSS );
    DUMPKEYVAL("tempADS1220", "%f",  PUEO_SS_TEMPERATURE_CONVERT_ADS1220(s->ss[i].tempADS1220));
    DUMPKEYVAL("tempSS", "%f",  PUEO_SS_TEMPERATURE_CONVERT_SS(s->ss[i].tempSS));
    DUMPENDARROBJ();
  }
  DUMPENDARR();
  DUMPTIME(s,readout_time);
  DUMPU32(s,sequence_number);
  DUMPX32(s,flags);
  DUMPEND();
  DUMPFINISH();
}

int pueo_dump_cmd_echo(FILE *f, const pueo_cmd_echo_t * e)
{
  DUMPINIT(f);
  DUMPSTART("cmd_echo");
  DUMPU32(e,when);
  unsigned len = e->len_m1;
  len+=1;
  DUMPKEYVAL("len","%u",len);
  DUMPU32(e,count);
  DUMPARRAY(e,data,len,"0x%02x");
  DUMPEND();
  DUMPFINISH();
}

int pueo_dump_startracker(FILE *f, const pueo_startracker_t * t)
{
  DUMPINIT(f);
  DUMPSTART("startracker");
  DUMPTIME(t,time1);
  DUMPTIME(t,time3);
  DUMPEND();
  DUMPFINISH();
}

int pueo_dump_timemark(FILE *f, const pueo_timemark_t * t)
{
  DUMPINIT(f);
  DUMPSTART("timemark");
  DUMPTIME(t,readout_time);
  DUMPTIME(t,rising);
  DUMPTIME(t,falling);
  DUMPU16(t, rise_count);
  DUMPU8(t,channel);
  DUMPX8(t,flags);
  DUMPEND();
  DUMPFINISH();
}

int pueo_dump_logs(FILE *f, const pueo_logs_t * l)
{
  DUMPINIT(f);
  DUMPSTART("logs");
  DUMPU32(l, utc_retrieved);
  DUMPBOOL(l, is_until);
  DUMPU16(l, rel_time_since_or_until);
  DUMPKEYVAL("daemon","%.*s", l->daemon_len, l->buf);
  DUMPKEYVAL("grep","%.*s", l->grep_len, l->buf + l->daemon_len);
  DUMPKEYVAL("logs","%.*s", l->msg_len, l->buf + l->daemon_len + l->grep_len);
  DUMPEND();
  DUMPFINISH();
}

int pueo_dump_file_download(FILE *f, const pueo_file_download_t * dl)
{
  DUMPINIT(f);
  DUMPSTART("file_download");
  DUMPU32(dl, mtime);
  DUMPBOOL(dl, read_ok);
  DUMPU32(dl, offset);
  char fname_padded[31] = {0};
  memcpy(fname_padded, dl->fname, sizeof(dl->fname));
  DUMPKEYVAL("filename","%s", fname_padded);
  DUMPU16(dl, len);

  if (dl->binary)
  {
    DUMPARRAY(dl,bytes,dl->len,"0x%x");
  }
  else
  {
    DUMPKEYVAL("text","%.*s", dl->len, dl->bytes);
  }

  DUMPEND();
  DUMPFINISH();
}


int pueo_dump_nav_sat(FILE *f, const pueo_nav_sat_t * n)
{
  DUMPINIT(f);
  DUMPSTART(n->source == PUEO_NAV_CPT7 ?"nav_sat_cpt7" :
            n->source == PUEO_NAV_BOREAS ? "nav_sat_boreas" :
            n->source == PUEO_NAV_ABX2 ? "nav_sat_abx2" : 
            n->source == PUEO_NAV_TURF ? "nav_sat_turf" : 
            "nav_sat_unknown");
  DUMPTIME(n, readout_time);
  DUMPTIME(n, gps_time);
  DUMPU8(n, nsats_used);
  DUMPU8(n, nsats_visible);

  DUMPSTARTARR("sats");
  for (int i = 0 ; i < n->nsats_visible; i++)
  {
    DUMPSTARTARROBJ();
    DUMPKEYVAL("used", "%hhu", n->sats[i].used);
    DUMPKEYVAL("type", "%hhu", n->sats[i].type);
    DUMPKEYVAL("qualityInd", "%hhu", n->sats[i].qualityInd);
    DUMPKEYVAL("svid", "%hhu", n->sats[i].svid);
    DUMPKEYVAL("el", "%f", (double) n->sats[i].el);
    DUMPKEYVAL("az", "%f", (double) n->sats[i].az);
    DUMPKEYVAL("cno", "%f", (double) n->sats[i].cno);
    DUMPKEYVAL("prRes", "%f", (double) n->sats[i].prRes);
    DUMPENDARROBJ();
  }
  DUMPENDARR();
  DUMPEND();
  DUMPFINISH();
}

int pueo_dump_daq_hsk_summary(FILE * f, const pueo_daq_hsk_summary_t * hsk)
{
  DUMPINIT(f);
  DUMPSTART("daq_hsk");

  // we need to extract some of the bitfields, I tihnk, otherwise printf might get confused? I'm not actually sure if tha'ts true, but either way it will make it easier t oprint

  DUMPU32(hsk, start_second);
  DUMPU32(hsk, end_second);
  DUMPU64(hsk, global_total_avg);
  DUMPU64(hsk, global_total_min);
  DUMPU64(hsk, global_total_max);
  DUMPU64(hsk, global_total_rms);
  DUMPU64(hsk, MIE_total_H_avg);
  DUMPU64(hsk, MIE_total_V_avg);
  DUMPU64(hsk, aux_total_avg);
  DUMPU64(hsk, pps_rate);
  DUMPARRAY(hsk,Hscalers_avg,12,"%hhu");
  DUMPARRAY(hsk,Vscalers_avg,12,"%hhu");

  DUMPSTARTARR("surfs");
  for (int i = 0; i < PUEO_NREALSURF; i++)
  {
    DUMPSTARTARROBJ();
    DUMPKEYVAL("MI_surf_idx", "%d", i);
    DUMPKEYVAL("surf_link", "%d", PUEO_IMISURF_SLOT(i));
    DUMPKEYVAL("surf_slot", "%d", PUEO_IMISURF_SLOT(i));

    DUMPSTARTARR("beams");
    for (int j = 0; j < PUEO_NBEAMS; j++)
    {
      DUMPSTARTARROBJ();
      DUMPKEYVAL("threshold_average","%u", hsk->surf[i].beams[j].thresh_avg);
      DUMPKEYVAL("scaler_average","%u", hsk->surf[i].beams[j].scaler_avg);
      DUMPKEYVAL("scaler_rms","%u", 16 * hsk->surf[i].beams[j].scaler_rms_div_16);
      DUMPENDARROBJ();
    }
    DUMPENDARR();
    DUMPENDARROBJ();
  }
  DUMPENDARR();

  DUMPSTARTARR("L2_averages");
  for (int i = 0; i < 26; i++) printf("%f%s", hsk->enable_mask_fraction[i]/255.,  i < 25 ? "," : "");
  DUMPENDARR();

  DUMPEND();
  DUMPFINISH();
}

//TODO add more things
int pueo_dump_daq_hsk(FILE * f, const pueo_daq_hsk_t * hsk)
{
  DUMPINIT(f);
  DUMPSTART("daq_hsk");

  DUMPTIME(hsk, scaler_readout_time);
  DUMPTIME(hsk, l2_readout_time);
  DUMPU32(hsk, soft_rate);
  DUMPU32(hsk, pps_rate);
  DUMPU32(hsk, l2_enable_mask);

  DUMPARRAY(hsk,Hscalers,12,"%u");
  DUMPARRAY(hsk,Vscalers,12,"%u");

  uint32_t total_L2 = 0;
  for (int i = 0; i < 12; i++) { total_L2 += hsk->Hscalers[i] + hsk->Vscalers[i]; }
  DUMPKEYVAL("total_L2_rate", "%u", total_L2);

  DUMPEND();
  DUMPFINISH()

}


int pueo_dump_saved_priorities(FILE * f, const pueo_saved_priorities_t *p)
{
  DUMPINIT(f);
  DUMPSTART("saved_priorities");
  DUMPU32(p, run);
  DUMPU32(p, event);
  DUMPBOOL(p, prio.topring_blast_flag);
  DUMPBOOL(p, prio.botring_blast_flag);
  DUMPBOOL(p, prio.fullpayload_blast_flag);
  DUMPBOOL(p, prio.frontback_blast_flag);
  DUMPX16(p, prio.anthro_base_flag);
  DUMPU16(p, prio.cal_type);
  DUMPU16(p, prio.signal_level);
  DUMPEND();
  DUMPFINISH()
}

int pueo_dump_prio_status(FILE * f, const pueo_prio_status_t * s)
{
  DUMPINIT(f);
  DUMPSTART("prio_status");
  DUMPTIME(s,start_time);
  DUMPTIME(s,end_time);
  DUMPARRAY(s, delay_frac, 6, "%f");
  DUMPARRAY(s, S_frac, 4, "%f");
  DUMPARRAY(s, blast_frac, 4, "%f");
  DUMPARRAY(s, anthro_frac, 6, "%f");
  DUMPU32(s, total_events);
  DUMPU32(s, total_force);
  DUMPFLT(s, starlink_partition_free_GB);
  DUMPEND();
  DUMPFINISH();
}



