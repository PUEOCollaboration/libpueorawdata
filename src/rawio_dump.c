/* here we have the dumpers for each data type*/

#include <stdio.h>
#include "pueo/rawio.h"
#include "pueo/sensor_ids.h"


const char * dump_tabs="\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
static int dump_ntabs = 0;

#define DUMPSTART(f, what) ret+=fprintf(f,"%.*s \"%s\" : {\n", dump_ntabs++, dump_tabs, what)
#define DUMPSTARTARR(f, what) ret+=fprintf(f,"%.*s \"%s\" : [\n", dump_ntabs++, dump_tabs, what)
#define DUMPKEYVAL(f,key,frmt,...) ret+=fprintf(f, "%.*s\"" key "\": " frmt ",\n", dump_ntabs, dump_tabs, __VA_ARGS__)
#define DUMPVAL(f,x,wut,frmt) DUMPKEYVAL(f,#wut,frmt, x->wut)
#define DUMPU32(f,x,wut) DUMPVAL(f,x,wut,"%u")
#define DUMPU64(f,x,wut) DUMPVAL(f,x,wut,"%lu")
#define DUMPU16(f,x,wut) DUMPVAL(f,x,wut,"%hu")
#define DUMPI16(f,x,wut) DUMPVAL(f,x,wut,"%hd")
#define DUMPU8(f,x,wut) DUMPVAL(f,x,wut,"%hhu")
#define DUMPX8(f,x,wut) DUMPVAL(f,x,wut,"0x%hhx")
#define DUMPFLT(f,x,wut) DUMPVAL(f,x,wut,"%f")
#define DUMPARRAY(f,x,wut,N,frmt) ret+=fprintf(f,"\%.*s\""#wut"\": [", dump_ntabs, dump_tabs); for (int asdf = 0; asdf < N; asdf++) ret+=fprintf(f," "frmt"%c",x->wut[asdf], asdf==N-1 ? ' ' : ','); ret+=fprintf(f,"],\n")
#define DUMPEND(f) ret+=fprintf(f,"\%.*s}\n", --dump_ntabs, dump_tabs)
#define DUMPENDARR(f) ret+=fprintf(f,"\%.*s]\n", --dump_ntabs, dump_tabs)



static int pueo_dump_waveform(FILE *f, const pueo_waveform_t * wf)
{
  int ret = 0;
  DUMPSTART(f,"waveform");
  DUMPU8(f,wf,channel_id);
  DUMPX8(f,wf,flags);
  DUMPU16(f,wf,length);
  DUMPARRAY(f,wf,data,wf->length,"%hd");
  DUMPEND(f);
  return ret;
}

int pueo_dump_single_waveform(FILE *f, const pueo_single_waveform_t * wf)
{
  int ret = 0;
  DUMPSTART(f,"single_waveform");
  DUMPU32(f,wf,run);
  DUMPU32(f,wf,event);
  DUMPU32(f,wf,event_second);
  DUMPU32(f,wf,event_time);
  DUMPU32(f,wf,last_pps);
  DUMPU32(f,wf,llast_pps);
  DUMPU32(f,wf,trigger_meta[0]);
  DUMPU32(f,wf,trigger_meta[1]);
  DUMPU32(f,wf,trigger_meta[2]);
  DUMPU32(f,wf,trigger_meta[3]);
  ret+=pueo_dump_waveform(f,&wf->wf);
  DUMPEND(f);
  return ret;
}

int pueo_dump_full_waveforms(FILE *f, const pueo_full_waveforms_t * wf)
{
  int ret = 0;
  DUMPSTART(f,"single_waveform");
  DUMPU32(f,wf,run);
  DUMPU32(f,wf,event);
  DUMPU32(f,wf,event_second);
  DUMPU32(f,wf,event_time);
  DUMPU32(f,wf,last_pps);
  DUMPU32(f,wf,llast_pps);
  DUMPU32(f,wf,trigger_meta[0]);
  DUMPU32(f,wf,trigger_meta[1]);
  DUMPU32(f,wf,trigger_meta[2]);
  DUMPU32(f,wf,trigger_meta[3]);
  for (int i = 0; i < PUEO_NCHAN; i++)
  {
    ret+=pueo_dump_waveform(f,&wf->wfs[i]);
  }
  DUMPEND(f);
  return ret;
}



int pueo_dump_sensors_telem(FILE *f, const pueo_sensors_telem_t *t)
{
  int ret = 0;
  DUMPSTART(f,"sensors_telem");
  DUMPU32(f,t,timeref_secs);
  DUMPU32(f,t,sensor_id_magic);
  DUMPU32(f,t,num_packets);
  DUMPSTARTARR(f,"sensors");
  for (int i = 0; i < t->num_packets; i++)
  {
    const pueo_sensor_telem_t * s = &t->sensors[i];
    DUMPSTART(f,"pueo_sensor_telem");
    DUMPU16(f,s,sensor_id);
    DUMPKEYVAL(f,"decoded_sensor_subsystem","%s",pueo_sensor_id_get_subsystem(t->sensors[i].sensor_id));
    DUMPKEYVAL(f,"decoded_sensor_name","%s",pueo_sensor_id_get_name(t->sensors[i].sensor_id));
    DUMPKEYVAL(f,"decoded_sensor_type","%c",pueo_sensor_id_get_type_tag(t->sensors[i].sensor_id));
    DUMPKEYVAL(f,"decoded_sensor_kind","%c",pueo_sensor_id_get_kind(t->sensors[i].sensor_id));
    DUMPI16(f,s,relsecs);

    switch(pueo_sensor_id_get_type_tag(t->sensors[i].sensor_id))
    {
      case 'F':
        DUMPKEYVAL(f,"value", "%f", (double) t->sensors[i].val.fval); break;
      case 'U':
        DUMPKEYVAL(f,"value", "%hu", t->sensors[i].val.uval); break;
      case 'I':
      default: 
        DUMPKEYVAL(f,"value", "%hd", t->sensors[i].val.ival); break;
    }
    DUMPEND(f);
  }
  DUMPENDARR(f);
  DUMPEND(f);
  return ret;
}

int pueo_dump_sensors_disk(FILE *f, const pueo_sensors_disk_t *t)
{
  int ret = 0;
  DUMPSTART(f,"sensors_disk");
  DUMPU32(f,t,sensor_id_magic);
  DUMPU32(f,t,num_packets);
  DUMPSTARTARR(f,"sensors");
  for (int i = 0; i < t->num_packets; i++)
  {
    const pueo_sensor_disk_t * s = &t->sensors[i];
    DUMPSTART(f,"pueo_sensor_disk");
    DUMPU16(f,s,sensor_id);
    DUMPKEYVAL(f,"decoded_sensor_subsystem","%s",pueo_sensor_id_get_subsystem(t->sensors[i].sensor_id));
    DUMPKEYVAL(f,"decoded_sensor_name","%s",pueo_sensor_id_get_name(t->sensors[i].sensor_id));
    DUMPKEYVAL(f,"decoded_sensor_type","%c",pueo_sensor_id_get_type_tag(t->sensors[i].sensor_id));
    DUMPKEYVAL(f,"decoded_sensor_kind","%c",pueo_sensor_id_get_kind(t->sensors[i].sensor_id));
    DUMPU32(f,s,time_secs);
    DUMPU16(f,s,time_ms);

    switch(pueo_sensor_id_get_type_tag(t->sensors[i].sensor_id))
    {
      case 'F':
        DUMPKEYVAL(f,"value", "%f",(double) t->sensors[i].val.fval); break;
      case 'U':
        DUMPKEYVAL(f,"value", "%u",t->sensors[i].val.uval); break;
      case 'I':
      default: 
        DUMPKEYVAL(f,"value", "%d",t->sensors[i].val.ival); break;
    }
    DUMPEND(f);
  }
  DUMPENDARR(f);
  DUMPEND(f);
  return ret;
}

int pueo_dump_nav_att(FILE *f, const pueo_nav_att_t *n)
{
  int ret = 0;
  DUMPSTART(f,n->source == PUEO_NAV_CPT7 ?"nav_att_cpt7" :
            n->source == PUEO_NAV_BOREAS ? "nav_att_boreas" :
            n->source == PUEO_NAV_ABX2 ? "nav_att_abx2" : "nav_att_unknown");
  DUMPKEYVAL(f,"readout_time", "%lu.09%lu", (uint64_t)n->readout_time.utc_secs, (uint64_t) n->readout_time.utc_nsecs);
  DUMPKEYVAL(f,"gps_time", "%lu.09%lu", (uint64_t)n->gps_time.utc_secs, (uint64_t) n->gps_time.utc_nsecs);
  DUMPFLT(f,n,lat);
  DUMPFLT(f,n,lon);
  DUMPFLT(f,n,alt);
  DUMPFLT(f,n,heading);
  DUMPFLT(f,n,pitch);
  DUMPFLT(f,n,roll);
  DUMPARRAY(f,n,v,3,"%f");
  DUMPARRAY(f,n,acc,3,"%f");
  DUMPFLT(f,n,hdop);
  DUMPFLT(f,n,vdop);
  DUMPU8(f,n,nsats);
  DUMPX8(f,n,flags);
  DUMPEND(f);
  return ret;
}

int pueo_dump_slow(FILE *f, const pueo_slow_t * s)
{
  int ret = 0;
  DUMPSTART(f,"slow");
  DUMPU16(f,s,ncmds);
  DUMPU16(f,s,time_since_last_cmd);
  DUMPU8(f,s,last_cmd);
  DUMPU32(f,s,sipd_uptime);
  DUMPU32(f,s,cpu_time);
  DUMPU32(f,s,cpu_uptime);
  DUMPU32(f,s,can_ping_world);
  DUMPU32(f,s,starlink_on);
  DUMPU32(f,s,los_on);
  DUMPU16(f,s,current_run);
  DUMPU16(f,s,current_run_secs);
  DUMPU32(f,s,current_run_events);

  DUMPEND(f);

  return ret;
}

