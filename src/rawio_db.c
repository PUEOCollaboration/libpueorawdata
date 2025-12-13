/* here we have DB Insertion scripts for each datatype we want to insert into the database
 **/

#define _GNU_SOURCE
#include "float16_guard.h"

#include <fcntl.h>
#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "pueo/rawio.h"
#include "pueo/rawdata.h"
#include "pueo/sensor_ids.h"

#ifdef PGSQL_ENABLED
#include <libpq-fe.h>
#else
typedef void PGconn;
#endif

#ifdef SQLITE_ENABLED
#include <sqlite3.h>
#else
typedef void sqlite3;
typedef void sqlite3_value;
typedef void sqlite3_context;
#endif



struct pueo_db_handle
{
  enum
  {
    DB_PGSQL,
    DB_SQLDIR,
    DB_SQLITE
  } type;

  char * description;
  const char * infinity;
  const char * minfinity;

  enum
  {
    DB_INVALID,
    DB_READY,
    DB_BEGUN,
    DB_FAIL
  } state;


  union
  {
    struct
    {
      int dirfd;
      FILE * current;
    } sqldir;

    struct  //keep these binary compatible after the first field
    {
      PGconn * psql;
      FILE * memstream;
      char * buf;
      size_t bufN;
    } psql;
    struct  //keep these binary compatible after the first field
    {
      sqlite3 * db;
      FILE * memstream;
      char * buf;
      size_t bufN;
    }sqlite;
  } backend;

  struct timespec txn_begin;
  struct timespec txn_end;
  uint64_t flags;
};

static int init_db(pueo_db_handle_t * h);

pueo_db_handle_t * pueo_db_handle_open_sqlfiles_dir(const char * dir, uint64_t flags)
{
  // Attempt to make the directory (it might already exist though)
  errno = 0;
  if (mkdir(dir, 0775))
  {
    if (errno != EEXIST)
    {
      fprintf(stderr,"Could not create %s and it doesn't already exist\n", dir);
      return NULL;
    }
  }

  //now try to open it
  int dirfd = open(dir, O_DIRECTORY);
  if (dirfd < 0)
  {
    fprintf(stderr,"Could not open %s as directory or don't have permissions to write\n", dir);
    return NULL;
  }

  pueo_db_handle_t * h = calloc(1, sizeof(pueo_db_handle_t));
  h->type = DB_SQLDIR;
  h->infinity = "'infinity'";
  h->minfinity = "'-infinity'";
  asprintf(&h->description,"SQLDIR %s", dir);
  h->backend.sqldir.dirfd = dirfd;
  h->state = DB_READY;
  h->flags = flags;
  init_db(h);
  return h;
}




pueo_db_handle_t * pueo_db_handle_open(const char * uri, uint64_t flags)
{
  if (strstr(uri,"postgresql://") == uri)
  {
    return pueo_db_handle_open_pgsql(uri, flags);
  }
  else if (strstr(uri,"timescaledb+postgresql://") == uri)
  {
    return pueo_db_handle_open_pgsql(uri + strlen("timescaledb+"), flags | PUEO_DB_INIT_WITH_TIMESCALEDB);
  }
  // made up uri scheme for old style pgsql conninfo
  else if (strstr(uri,"PGSQL_CONNINFO:") == uri)
  {
    return pueo_db_handle_open_pgsql(uri + strlen("PGSQL_CONNINFO:"),flags);
  }
  // made up uri scheme for old style pgsql conninfo but for TIMESCALEDB
  else if (strstr(uri,"TIMESCALEDB_CONNINFO:") == uri)
  {
    return  pueo_db_handle_open_pgsql(uri + strlen("TIMESCALEDB_CONNINFO:"),flags | PUEO_DB_INIT_WITH_TIMESCALEDB);
  }
  else if (strstr(uri,"sqldir://")==uri)
  {
    return pueo_db_handle_open_sqlfiles_dir(uri + strlen("sqldir://"),flags);
  }
  else if (strstr(uri,"sqlite://")==uri)
  {
    return pueo_db_handle_open_sqlite(uri + strlen("sqlite://"),flags);
  }
  else return NULL;
}

pueo_db_handle_t * pueo_db_handle_open_pgsql(const char * conninfo, uint64_t flags)
{
#ifndef PGSQL_ENABLED
  fprintf(stderr, "You asked to open a pgsql handle but compiled without pgsql support. What were you expecting to happen?\n"); 
  return NULL;
#else
  PGconn * psql = PQconnectdb(conninfo);
  if (PQstatus(psql) != CONNECTION_OK)
  {
    fprintf(stderr,"Failed to open pgsql handle with conninfo =  %s\n", conninfo);
    return NULL;
  }

  pueo_db_handle_t * h = calloc(1, sizeof(pueo_db_handle_t));
  h->type = DB_PGSQL;
  asprintf(&h->description,"PGSQL connection with conninfo %s", conninfo);
  h->backend.psql.psql = psql;
  h->state = DB_READY;
  h->infinity = "'infinity'";
  h->minfinity = "'-infinity'";
  h->flags = flags;
  init_db(h);
  return h;
#endif
}

#ifdef SQLITE_ENABLED
static void sqlite_to_timestamp(sqlite3_context *ctx, int nargs, sqlite3_value ** args);
#endif

pueo_db_handle_t * pueo_db_handle_open_sqlite(const char * dbfile, uint64_t flags)
{
#ifndef SQLITE_ENABLED
	(void) dbfile;
	(void) flags;
  fprintf(stderr, "You asked to open a sqlite handle but compiled without sqlite support. What were you expecting to happen?\n"); 
  return NULL;
#else
  sqlite3* db = NULL;
  int r = sqlite3_open(dbfile, &db);
  if (r)
  {
    fprintf(stderr,"Failed to open sqlite db %s: %s\n", dbfile, sqlite3_errmsg(db));
    sqlite3_close(db);
    return NULL;
  }


  // register to_timestamp polyfill

  r = sqlite3_create_function(db, "to_timestamp", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0, sqlite_to_timestamp, 0, 0);
  if (r)
  {
    fprintf(stderr,"Failed to create to_timestamp polyfill for sqlite. Probably bad things will happen\n");
  }

  pueo_db_handle_t * h = calloc(1, sizeof(pueo_db_handle_t));
  h->type = DB_SQLITE;
  asprintf(&h->description,"SQLITE connection to %s", dbfile);
  h->backend.sqlite.db = db;
  h->state = DB_READY;
  h->infinity = "1e999";
  h->minfinity = "-1e999";
  h->flags = flags;
  init_db(h);
  return h;
#endif
}



static FILE * begin_sql_stream(pueo_db_handle_t * h)
{
  if (h->state != DB_READY)
  {
    fprintf(stderr,
        "Trying to start sql stream to %s when not ready. %sl\n",
        h->description, h->state == DB_INVALID ? "State is invalid " :
                        h->state == DB_BEGUN ? "Previous transaction not closed." 
                        : "DB is a failed state");

    return NULL;
  }

  clock_gettime(CLOCK_REALTIME, &h->txn_begin);

  if (h->type == DB_SQLDIR)
  {
    struct tm current_tm = {0};

    if (!gmtime_r(&h->txn_begin.tv_sec, &current_tm))
    {
      fprintf(stderr,"Um, couldn't get the current time? What's up with that?\n");
      return NULL;
    }

    char fname[128];
    sprintf(fname,"%02d-%02d-%04dT%02d.%02d.%02d.%09ld.sql",
        current_tm.tm_mon+1, current_tm.tm_mday,
        current_tm.tm_year+1900, current_tm.tm_hour,
        current_tm.tm_min, current_tm.tm_sec, h->txn_begin.tv_nsec);


    int fd = openat(h->backend.sqldir.dirfd, fname, O_CREAT | O_RDWR | O_EXCL, 00644);
    if (fd < 0)
    {
      fprintf(stderr,"Could not open %s\n", fname);
      return NULL;
    }

    h->backend.sqldir.current = fdopen(fd,"w");
    if (h->backend.sqldir.current)
    {
      h->state = DB_BEGUN;
    }
    else
    {
      fprintf(stderr,"Problem with fdopen...\n");
    }
    return h->backend.sqldir.current;
  }
  else if (h->type == DB_PGSQL || h->type == DB_SQLITE)
  {

    // we will open a memstream that will be used for the transaction.
    h->backend.psql.memstream = open_memstream(&h->backend.psql.buf, &h->backend.psql.bufN);
    if (!h->backend.psql.memstream)
    {
      fprintf(stderr,"Couldn't allocate memstream for transaction\n");
    }
    else
    {
      h->state = DB_BEGUN;
    }
    return h->backend.psql.memstream;
  }

  fprintf(stderr,"Unkown backend: %d\n", h->type);
  return NULL;
}


static int commit_sql_stream(pueo_db_handle_t *h)
{
  if (!h) return -1;
  if (h->state != DB_BEGUN)
  {
    fprintf(stderr,"handle %s is not ready to commit\n", h->description);
    return -1;
  }


  clock_gettime(CLOCK_REALTIME, &h->txn_end);

  if (h->type == DB_SQLDIR)
  {

    errno = 0;
    int ret = fclose(h->backend.psql.memstream);
    if (ret)
    {

      fprintf(stderr,"Problem with fclose: %d (%s)\n", errno, strerror(errno));
    }
    h->state = DB_READY;
    return ret;
  }
#ifdef PGSQL_ENABLED
  else if (h->type == DB_PGSQL)
  {

    fclose(h->backend.psql.memstream);
    PGresult * r = PQexec(h->backend.psql.psql, h->backend.psql.buf);
    h->backend.psql.bufN  = 0;
    int ret = 0;
    int status = PQresultStatus(r);
    if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK)
    {
      fprintf(stderr,"Problem (%s) with psql query: %s\n", PQresStatus(status), PQresultErrorMessage(r));
      fprintf(stderr,"Query was: %s\n", h->backend.psql.buf);
      ret = -1;
    }

    PQclear(r);
    h->state = DB_READY;
    free(h->backend.psql.buf);
    return ret ;
  }
#endif
#ifdef SQLITE_ENABLED
  else if (h->type == DB_SQLITE)
  {

    fclose(h->backend.sqlite.memstream);
    char * errmsg = NULL;
    int r = sqlite3_exec(h->backend.sqlite.db, h->backend.sqlite.buf, NULL,NULL,&errmsg);
    if (r)
    {
      fprintf(stderr,"sqlite error: %s\n", errmsg);
      fprintf(stderr,"Query was: %s\n", h->backend.sqlite.buf);
      sqlite3_free(errmsg);
    }
    h->backend.sqlite.bufN  = 0;

    h->state = DB_READY;
    free(h->backend.sqlite.buf);
    return r ;
  }
#endif


  h->state = DB_FAIL;
  return -1;
}



//Stub means we haven't implemented yet but we need to
#define STUB_INSERT_DB(X) \
int pueo_db_insert_##X(pueo_db_handle_t * h, const pueo_##X##_t * x) { (void) h; (void) x; _Pragma("GCC warning \"stub implementation\""); fprintf(stderr,"WARNING: pueo_db_insert_" #X "() has stub implementation\n"); return -1; }

//Unsupported means we don't intend to support inserting this type into a DB
#define UNSUPPORTED_INSERT_DB(X)\
int pueo_db_insert_##X(pueo_db_handle_t * h, const pueo_##X##_t * x) { (void) h; (void) x; fprintf(stderr,"WARNING: pueo_db_insert_" #X "() should not be called\n"); return -1; }



// these should never need to be entered into a DB
UNSUPPORTED_INSERT_DB(full_waveforms)
UNSUPPORTED_INSERT_DB(sensors_disk)


STUB_INSERT_DB(nav_sat)
STUB_INSERT_DB(file_download)
STUB_INSERT_DB(saved_priorities)
  
int pueo_db_insert_ss(pueo_db_handle_t * h, const pueo_ss_t* ss)
{

  FILE * f = begin_sql_stream(h);

  fprintf(f, "INSERT INTO sun_sensors (time");
  for (int i = 0; i < PUEO_SS_NUM_SENSORS; i++)
  {
    fprintf(f,", x1_ss%d, x2_ss%d, y1_ss%d, y2_ss%d, tempSS_ss%d, tempADS1220_ss%d",
        i,i,i,i,i,i);
  }

  fprintf(f, ") VALUES(TO_TIMESTAMP(%lu.%09u) ", (uint64_t) ss->readout_time.utc_secs,  (uint32_t) ss->readout_time.utc_nsecs);

  for (int i = 0; i < PUEO_SS_NUM_SENSORS; i++)
  {
        fprintf(f, ", %u, %u, %u, %u, %f, %f",
        (uint32_t) ss->ss[i].x1, (uint32_t) ss->ss[i].x2,
        (uint32_t) ss->ss[i].y1, (uint32_t) ss->ss[i].y2,
        PUEO_SS_TEMPERATURE_CONVERT_SS(ss->ss[i].tempSS),
        PUEO_SS_TEMPERATURE_CONVERT_ADS1220(ss->ss[i].tempADS1220)
        );
  }

  fprintf(f,")\n");

  return commit_sql_stream(h);
}

int pueo_db_insert_slow(pueo_db_handle_t * h, const pueo_slow_t* slow) {
  
  FILE * f = begin_sql_stream(h);
  fprintf(f, "INSERT INTO slow_packets(time, ncmds, time_since_last_cmd, "
             "last_cmd, sipd_uptime, cpu_uptime, can_ping_world, starlink_on, "
             "los_on, gpu_present, nic_present, turf_seen, hsk_seen, ss_seen, "
             "current_run, current_run_secs, current_run_events, acqd_running, "
             "prioritizerd_running, current_run_rf_events, hsk_uptime, "
             "pals_A_index, pals_A_free, pals_B_index, pals_B_free, "
             "ssd0_free, ssd1_free, ssd2_free, ssd3_free, ssd4_free, "
             "heading_abx, heading_boreas, heading_cpt7, turf_fix_type, "
             "NIC_temperature, SFC_temperature, pwr_from_sun, pwr_usage, "
             "battery_state, AMPA_current, VPol_Crate_on, HPol_Crate_on, SFC_Voltage_on, "
             "VPol_Current, HPol_Current, SFC_Current");
  for(int i = 0; i < PUEO_NUM_L2; i++) {
    fprintf(f, ", L2_rates_V_%i", i);
  }
  for(int i = 0; i < PUEO_NUM_L2; i++) {
    fprintf(f, ", L2_rates_H_%i", i);
  }
  fprintf(f, " ) "
             "VALUES (TO_TIMESTAMP(%i.0), %i, %i, "
             "%i, %i, %i, %i, %i, "
             "%i, %i, %i, %i, %i, %i, "
             "%i, %i, %i, %i, "
             "%i, %i, %i, "
             "%i, %i, %i, %i, "
             "%i, %i, %i, %i, %i, "
             "%i, %i, %i, %i, "
             "%i, %i, %i, %i, "
             "%i, %i, %i, %i, "
             "%i, %i, %i, %i",
      slow->cpu_time, slow->ncmds, slow->time_since_last_cmd,
      slow->last_cmd, slow->sipd_uptime, slow->cpu_uptime, slow->can_ping_world, slow->starlink_on,
      slow->los_on, slow->gpu_present, slow->nic_present, slow->turf_seen, slow->hsk_seen, slow->ss_seen,
      slow->current_run, slow->current_run_secs, slow->current_run_events, slow->acqd_running,
      slow->prioritizerd_running, slow->current_run_rf_events, slow->hsk_uptime,
      slow->pals[0].index, slow->pals[0].free, slow->pals[1].index, slow->pals[1].free,
      slow->ssd.ssd0_free, slow->ssd.ssd1_free, slow->ssd.ssd2_free, slow->ssd.ssd3_free, slow->ssd.ssd4_free,
      slow->nav.heading_abx, slow->nav.heading_boreas, slow->nav.heading_cpt7, slow->nav.turf_fix_type,
      slow->NIC_temperature, slow->SFC_temperature, slow->pwr_from_sun, slow->pwr_usage,
      slow->battery_state, slow->AMPA_current, slow->VPol_Crate_on, slow->HPol_Crate_on,
      slow->SFC_Voltage_on, slow->VPol_Current, slow->HPol_Current, slow->SFC_Current
  );
  for(int i = 0; i < PUEO_NUM_L2; i++) {
    fprintf(f, ", %i", slow->L2_rates[i][0]);
  }
  for(int i = 0; i < PUEO_NUM_L2; i++) {
    fprintf(f, ", %i", slow->L2_rates[i][1]);
  }
  fprintf(f, ");");

  return commit_sql_stream(h);
}

int pueo_db_insert_single_waveform(pueo_db_handle_t *h, const pueo_single_waveform_t *wf)
{
  FILE * f = begin_sql_stream(h);

  int max = -INT_MAX;
  int min = INT_MAX;
  double sum2= 0;
  double sum = 0;
  int N = 1024;
  for (int i = 0; i < N; i++)
  {
    int v = wf->wf.data[i];
    if (v > max) max = v;
    if (v < min) min = v;
    sum += v;
    sum2 += v*v;
  }

  double mean = sum / N;
  double rms = sqrt(sum2/N-mean*mean);


  fprintf(f, "INSERT INTO single_waveforms(time, run, event, channel, max, min, rms, subsecond, "
             "prio_trig_type, prio_topring_blast_flag, prio_botring_blast_flag, prio_fullpayload_blast_flag, "
             "prio_frontback_blast_flag, prio_anthro_base_flag, prio_cal_type, prio_signal_level)"
             "VALUES (TO_TIMESTAMP(%u.%09u), %d, %d, %d, %d, %d, %f, %f, "
	     "%d, %d, %d, %d, %d, %d, %d, %d);",
              wf->event_second,  wf->readout_time.utc_nsecs,
              wf->run, wf->event, wf->wf.channel_id, max, min, rms,
              ( (double) wf->event_time - wf->last_pps) / (wf->last_pps - wf->llast_pps),
              wf->prio.trig_type, wf->prio.topring_blast_flag, wf->prio.botring_blast_flag, wf->prio.fullpayload_blast_flag,
              wf->prio.frontback_blast_flag, wf->prio.anthro_base_flag, wf->prio.cal_type, wf->prio.signal_level
             );

  return commit_sql_stream(h);
}


int pueo_db_insert_nav_att(pueo_db_handle_t *h, const pueo_nav_att_t * att)
{

  FILE * f = begin_sql_stream(h);
  fprintf(f,"INSERT INTO nav_atts (readout_time, gps_time, lat, lon, alt, heading,"
            " heading_sigma, pitch, pitch_sigma, roll, roll_sigma, hdop, vdop, source, nsats, flags, temperature, antenna_current_0, antenna_current_1, antenna_current_2)"
           " VALUES(TO_TIMESTAMP(%lu.%09u), TO_TIMESTAMP(%lu.%09u), %f, %f, %f, %f,"
           " %f, %f, %f, %f, %f, %f, %f, '%c', %d, %d, %d, %d, %d, %d);",
           (uint64_t) att->readout_time.utc_secs, (uint32_t) att->readout_time.utc_nsecs, (uint64_t) att->gps_time.utc_secs,
           (uint32_t) att->gps_time.utc_nsecs, att->lat, att->lon, att->alt, att->heading,
           att->heading_sigma, att->pitch, att->pitch_sigma, att->roll, att->roll_sigma, att->hdop,
           att->vdop, att->source, att->nsats, att->flags, att->temperature, att->antenna_currents[0], att->antenna_currents[1], att->antenna_currents[1]);

  return commit_sql_stream(h);
}

int pueo_db_insert_nav_pos(pueo_db_handle_t *h, const pueo_nav_pos_t * pos)
{

  FILE * f = begin_sql_stream(h);
  fprintf(f,"INSERT INTO nav_poss (readout_time, gps_time, lat, lon, alt,"
            "hdop, vdop, source, nsats, flags, x, y, z, vx, vy, vz)"
           " VALUES(TO_TIMESTAMP(%lu.%09u), TO_TIMESTAMP(%lu.%09u), %f, %f, %f, %f,"
           " %f, %f, %f, %f, %f, %f, %f, '%c', %d, %d);",
           (uint64_t) pos->readout_time.utc_secs, (uint32_t) pos->readout_time.utc_nsecs, (uint64_t) pos->gps_time.utc_secs,
           (uint32_t) pos->gps_time.utc_nsecs, pos->lat, pos->lon, pos->alt, pos->hdop,
           pos->vdop, pos->source, pos->nsats, pos->flags, pos->x[0], pos->x[1], pos->x[2], pos->v[0], pos->v[1], pos->v[2]);

  return commit_sql_stream(h);
}


int pueo_db_insert_timemark(pueo_db_handle_t * h, const pueo_timemark_t * t)
{
  FILE * f = begin_sql_stream(h);
  fprintf(f,"INSERT INTO timemarks(readout_time, readout_time_ns, risetime, risetime_ns, falltime, falltime_ns, rise_count,"
             " flags, channel) VALUES (TO_TIMESTAMP(%lu), %u, TO_TIMESTAMP(%lu), %u, TO_TIMESTAMP(%lu), %u, %hu, %hhu, %hhu)",
             t->readout_time.utc_secs, t->readout_time.utc_nsecs,
             t->rising.utc_secs, t->rising.utc_nsecs,
             t->falling.utc_secs, t->falling.utc_nsecs,
             t->rise_count, t->flags, t->channel);


  return commit_sql_stream(h);

}

int pueo_db_insert_startracker(pueo_db_handle_t * h, const pueo_startracker_t * st)
{
  FILE * f = begin_sql_stream(h);
  fprintf(f,"INSERT INTO startrackers(st1_timestamp_s, st1_timestamp_ns, st3_timestamp_s, st3_timestamp_ns)"
             " VALUES (TO_TIMESTAMP(%lu), %u, TO_TIMESTAMP(%lu), %u);",
             st->time1.utc_secs, st->time1.utc_nsecs, st->time3.utc_secs, st->time3.utc_nsecs);


  return commit_sql_stream(h);

}

int pueo_db_insert_sensors_telem(pueo_db_handle_t * h, const pueo_sensors_telem_t * t)
{

  //helper local functions

#define DB_INSERT_TEMPLATE(X)\
    const char * X##_insert_string  = "INSERT INTO " #X "s (time, device, sensor, "#X ") VALUES (TO_TIMESTAMP(%u), '%s', '%s',"

  DB_INSERT_TEMPLATE(temperature);
  DB_INSERT_TEMPLATE(voltage);
  DB_INSERT_TEMPLATE(current);
  DB_INSERT_TEMPLATE(power);
  DB_INSERT_TEMPLATE(flag);
  DB_INSERT_TEMPLATE(magnetic_field);

#undef DB_INSERT_TEMPLATE

  const char * get_insert_string(char c)
  {
    switch(c)
    {
      case 'V':
        return voltage_insert_string;
      case 'A':
        return current_insert_string;
      case 'C':
        return temperature_insert_string;
      case 'W':
        return power_insert_string;
      case 'X':
        return flag_insert_string;
      case 'G':
        return magnetic_field_insert_string;
      default:
        return "yikes!";
    }
  }

  int telem_sensor_print_val(FILE * f, pueo_sensor_telem_t t, uint16_t magic)
  {
    char sensor_type = pueo_sensor_id_get_compat_type_tag(t.sensor_id, magic);

    if (sensor_type == 'F')
    {
      double d= (double) t.val.fval;
      if (isinf(d))
      {
        if (d > 0) return fprintf(f,"%s", h->infinity);
        else return fprintf(f,"%s", h->minfinity);
      }
      else
      {
        return fprintf(f,"%f", d);
      }
    }

    if (sensor_type == 'I')
    {
      return fprintf(f,"%hd", t.val.ival);
    }

    if (sensor_type == 'U')
    {
      return fprintf(f,"%hu", t.val.uval);
    }

    return 0;
  }


  FILE * f = begin_sql_stream(h);
  if(!f) return -1;

  for (unsigned i = 0; i < t->num_packets; i++)
  {
     uint32_t when = t->timeref_secs + t->sensors[i].relsecs;
     const char * sensor_name = pueo_sensor_id_get_compat_name(t->sensors[i].sensor_id, t->sensor_id_magic);
     const char * sensor_subsystem = pueo_sensor_id_get_compat_subsystem(t->sensors[i].sensor_id, t->sensor_id_magic);
     char sensor_kind = pueo_sensor_id_get_compat_kind(t->sensors[i].sensor_id, t->sensor_id_magic);

     fprintf(f, get_insert_string(sensor_kind), when, sensor_subsystem, sensor_name);
     telem_sensor_print_val(f,t->sensors[i], t->sensor_id_magic);
     fprintf(f,");\n");
 }

  return commit_sql_stream(h);
}

int pueo_db_insert_cmd_echo(pueo_db_handle_t * h, const pueo_cmd_echo_t * e)
{
  FILE * f = begin_sql_stream(h);
  fprintf(f,"INSERT INTO cmd_echos(time,len,count, data) VALUES (TO_TIMESTAMP(%u),%u,%u,",
      e->when, e->len_m1 + 1, e->count);

  // for pgsql, used escape string to insert into bytea. For sqlite, just a normal string.
  if (h->type != DB_SQLITE)
  {
    fprintf(f,"E'\\x");

  }
  for (unsigned i = 0; i <= e->len_m1; i++)
  {
    fprintf(f,"%02x", e->data[i]);
  }

  fprintf(f,"');");

  return commit_sql_stream(h);
}

int pueo_db_insert_logs(pueo_db_handle_t *h, const pueo_logs_t * l)
{
  FILE * f = begin_sql_stream(h);
  fprintf(f,"INSERT INTO logs(time, is_until, since_or_until, daemon, grep, logs) VALUES"
             "(TO_TIMESTAMP(%u), %d, %hu, '%.*s', '%.*s', '%.*s');",
             l->utc_retrieved, (int) l->is_until, l->rel_time_since_or_until,
             l->daemon_len, l->buf,
             l->grep_len, l->buf + l->daemon_len,
             l->msg_len, l->buf + l->daemon_len + l->grep_len);

  return commit_sql_stream(h);
}


void pueo_db_handle_close(pueo_db_handle_t ** hptr)
{
  if (!hptr || !*hptr) return;
  pueo_db_handle_t * h = *hptr;
  *hptr = NULL;

  if (h->state == DB_BEGUN)
  {
    fprintf(stderr,"Huh? Why hasn't it been committed\n");
    commit_sql_stream(h);
  }


  if (h->type == DB_SQLDIR)
  {
    close(h->backend.sqldir.dirfd);
  }
#ifdef PGSQL_ENABLED
  else if (h->type == DB_PGSQL)
  {
    PQfinish(h->backend.psql.psql);
  }
#endif
#ifdef SQLITE_ENABLED
  else if (h->type == DB_SQLITE)
  {
    sqlite3_close(h->backend.sqlite.db);
  }
#endif

  free(h);
}


/// yucky yuck yucky yuck

#define DB_MAKE_INDEX(X,T) \
const char * X##_index_string = "CREATE INDEX IF NOT EXISTS "#X"_time_idx on " #X "s ( " #T ");\n\n";\
const char * X##_create_TIMESCALEDB = "SELECT create_hypertable('" #X "s', by_range('"#T"'), if_not_exists => TRUE);\n"; \
if (h->type != DB_SQLITE &&  ( h->flags & PUEO_DB_INIT_WITH_TIMESCALEDB)) { fputs(X##_create_TIMESCALEDB,f); }\
else { fputs(X##_index_string, f); }

#define DB_TIME_TYPE_PGSQL "TIMESTAMPTZ"
#define DB_TIME_TYPE_SQLITE "DATETIME"
#define DB_INDEX_DEF_PGSQL "SERIAL"
#define DB_INDEX_DEF_SQLITE "INTEGER PRIMARY KEY AUTOINCREMENT"

static void timemark_init(FILE *f, pueo_db_handle_t *h)
{
  fprintf(f,"CREATE TABLE IF NOT EXISTS timemarks (uid %s, readout_time %s NOT NULL, readout_time_ns INTEGER, risetime %s NOT NULL, "
            "risetime_ns INTEGER,  falltime %s NOT NULL, falltime_ns INTEGER, "
            "rise_count INTEGER, flags INTEGER, channel INTEGER);\n",
      h->type == DB_SQLITE  ? DB_INDEX_DEF_SQLITE : DB_INDEX_DEF_PGSQL,
      h->type == DB_SQLITE  ? DB_TIME_TYPE_SQLITE : DB_TIME_TYPE_PGSQL,
      h->type == DB_SQLITE  ? DB_TIME_TYPE_SQLITE : DB_TIME_TYPE_PGSQL,
      h->type == DB_SQLITE  ? DB_TIME_TYPE_SQLITE : DB_TIME_TYPE_PGSQL);

  DB_MAKE_INDEX(timemark, risetime);
}

static void startracker_init(FILE *f, pueo_db_handle_t *h)
{
  fprintf(f,"CREATE TABLE IF NOT EXISTS startrackers (uid %s, st1_timestamp_s %s NOT NULL, st1_timestamp_ns INTEGER, st3_timestamp_s %s NOT NULL, st3_timestamp_ns INTEGER);\n",
      h->type == DB_SQLITE  ? DB_INDEX_DEF_SQLITE : DB_INDEX_DEF_PGSQL,  
      h->type == DB_SQLITE  ? DB_TIME_TYPE_SQLITE : DB_TIME_TYPE_PGSQL,
      h->type == DB_SQLITE  ? DB_TIME_TYPE_SQLITE : DB_TIME_TYPE_PGSQL);

  DB_MAKE_INDEX(startracker, st1_timestamp_s);
}


//TODO fill in the rest of this
static void daq_hsk_init(FILE *f, pueo_db_handle_t * h)
{
  fprintf(f,"CREATE TABLE IF NOT EXISTS daq_hsks ( uid %s, time %s NOT NULL ",
      h->type == DB_SQLITE  ? DB_INDEX_DEF_SQLITE : DB_INDEX_DEF_PGSQL,
      h->type == DB_SQLITE  ? DB_TIME_TYPE_SQLITE : DB_TIME_TYPE_PGSQL);
    for(int i = 0; i < 4; i++) {
      for(int j=0;j<7; j++){
        fprintf(f, ", turfio%i_surf%i_L1rate INTEGER", i,j);
      }
    }
    fprintf(f,", L2_rateH INTEGER, L2_rateV INTEGER, soft_rate INTEGER, pps_rate INTEGER, ext_rate INTEGER, "
      "MIE_total_H INTEGER, MIE_total_V INTEGER,"
      "LF_total_H INTEGER, LF_total_V INTEGER,"
      "aux_total INTEGER, global_total INTEGER"
      ")\n;");

  DB_MAKE_INDEX(daq_hsk, time)
}

//TODO SOMEONE PLEASE DO THIS @PARTYKEITH
static void daq_hsk_summary_init(FILE *f, pueo_db_handle_t * h)
{
  fprintf(f, "CREATE TABLE IF NOT EXISTS daq_hsk_summarys ( uid %s, time %s NOT NULL",
      h->type == DB_SQLITE  ? DB_INDEX_DEF_SQLITE : DB_INDEX_DEF_PGSQL,
      h->type == DB_SQLITE  ? DB_TIME_TYPE_SQLITE : DB_TIME_TYPE_PGSQL);
    for(int i = 0; i < PUEO_NREALSURF; i++) {
        fprintf(f, ", surf%i_thresh_avg_beamavg INTEGER, surf%i_scaler_avg_beamavg INTEGER, surf%i_scaler_rms_div_16_beamavg INTEGER, surf%i_max_thresh_avg INTEGER, surf%i_threshold_avg_beamMaxIndex INTEGER, surf%i_scaler_avg_forMaxBeam INTEGER, surf%i_scaler_rms_div_16_forMaxBeam INTEGER", i,i,i,i,i,i,i);
    }
    for(int j=0;j<12; j++){
        fprintf(f, ", L2_H%i_scaler_avg INTEGER ",j);
    }
    for(int j=0;j<12; j++){
        fprintf(f, ", L2_V%i_scaler_avg INTEGER ",j);
    }
    fprintf(f,", MIE_total_H_avg INTEGER, MIE_total_V_avg INTEGER, aux_total_avg INTEGER, pps_rate INTEGER, global_total_avg INTEGER, global_total_min INTEGER, global_total_max INTEGER, global_total_rms INTEGER , start_second INTEGER, duration INTEGER");
  fprintf(f, ");\n");

  DB_MAKE_INDEX(daq_hsk_summary, time);

}

int pueo_db_insert_daq_hsk(pueo_db_handle_t *h, const pueo_daq_hsk_t *hsk)
{

  FILE * f = begin_sql_stream(h);
  fprintf(f, "INSERT INTO daq_hsks(time");
    for(int i = 0; i < 4; i++) {
      for(int j=0;j<7; j++){
        fprintf(f, ", turfio%i_surf%i_L1rate", i,j);
      }
    }
  fprintf(f,", L2_rateH , L2_rateV , soft_rate , pps_rate , ext_rate , "
      "MIE_total_H , MIE_total_V ,"
      "LF_total_H , LF_total_V ,"
      "aux_total , global_total"
      ")");

  fprintf(f, " VALUES(TO_TIMESTAMP(%lu.%09u) ", (uint64_t) hsk->scaler_readout_time.utc_secs,  (uint32_t) hsk->scaler_readout_time.utc_nsecs);
    for(int i = 0; i < 4; i++) {
      for(int j=0;j<7; j++){
        fprintf(f, ", %i", hsk->turfio_L1_rate[i][j]);
      }
    }
  int sumL2H=0;
  int sumL2V=0;
  for(int i=0;i<12;i++){
    sumL2H+=(int) hsk->Hscalers[i];
    sumL2V+=(int) hsk->Vscalers[i];
  }
  fprintf(f,", %i, %i, %i, %i, %i, %i, %i, %i, %i,%i,%i", 
      sumL2H, sumL2V, hsk->soft_rate,hsk->pps_rate,hsk->ext_rate,
      hsk->MIE_total_H, hsk->MIE_total_V, hsk->LF_total_H,
      hsk->LF_total_V,hsk->aux_total,hsk->global_total
    );
  fprintf(f, ");");

  return commit_sql_stream(h);
}

int pueo_db_insert_daq_hsk_summary(pueo_db_handle_t *h, const pueo_daq_hsk_summary_t *hsk)
{

  FILE * f = begin_sql_stream(h);
  fprintf(f, "INSERT INTO daq_hsk_summarys(time");
    for(int i = 0; i < PUEO_NREALSURF; i++) {
        fprintf(f, ", surf%i_thresh_avg_beamavg, surf%i_scaler_avg_beamavg, surf%i_scaler_rms_div_16_beamavg, surf%i_max_thresh_avg, surf%i_threshold_avg_beamMaxIndex, surf%i_scaler_avg_forMaxBeam, surf%i_scaler_rms_div_16_forMaxBeam", i,i,i,i,i,i,i);
    }
    for(int j=0;j<12; j++){
        fprintf(f, ", L2_H%i_scaler_avg",j);
    }
    for(int j=0;j<12; j++){
        fprintf(f, ", L2_V%i_scaler_avg",j);
    }
  fprintf(f,", MIE_total_H_avg, MIE_total_V_avg, aux_total_avg, pps_rate, global_total_avg, global_total_min, global_total_max, global_total_rms, start_second, duration");

  //fprintf(f, "VALUES(TO_TIMESTAMP(%lu.%09u) ", (uint64_t) ss->readout_time.utc_secs,  (uint32_t) ss->readout_time.utc_nsecs);

  fprintf(f, " ) VALUES (TO_TIMESTAMP(%i.0) ", hsk->end_second);
    for(int i = 0; i < PUEO_NREALSURF; i++) {
      // calc averages per beam
        int max = 0;
        int max_index = 0;
        double thr_avg_sum = 0;
        double sclr_avg_sum = 0;
        double sclr_rms_avg_sum = 0;
        int N = PUEO_NBEAMS;
        for (int j = 0; j < N; j++){
          int v = (int) hsk->surf[i].beams[j].thresh_avg;
          int v1 = (int) hsk->surf[i].beams[j].scaler_avg;
          int v2 = (int) hsk->surf[i].beams[j].scaler_rms_div_16;
          if (v > max){
            max = v;
            max_index=j;
          }
          thr_avg_sum += v;
          sclr_avg_sum += v1;
          sclr_rms_avg_sum += v2;
        }
        if(max_index>PUEO_NBEAMS) max_index=0;
        fprintf(f, ", %f, %f, %f, %i, %i, %i, %i", thr_avg_sum/N,sclr_avg_sum/N,sclr_rms_avg_sum/N,hsk->surf[i].beams[max_index].thresh_avg,max_index,hsk->surf[i].beams[max_index].scaler_avg,hsk->surf[i].beams[max_index].scaler_rms_div_16);
    }
    for(int j=0;j<12; j++){
      fprintf(f, ", %i",hsk->Hscalers_avg[j]);
    }
    for(int j=0;j<12; j++){
      fprintf(f, ", %i",hsk->Vscalers_avg[j]);
    }
    fprintf(f,", %i, %i, %i, %i, %i, %i, %i, %i, %i, %i", 
      hsk->MIE_total_H_avg, hsk->MIE_total_V_avg, hsk->aux_total_avg,
      hsk->pps_rate, hsk->global_total_avg, hsk->global_total_min,
      hsk->global_total_max, hsk->global_total_rms, hsk->start_second,
      (int)hsk->end_second-(int)hsk->start_second
    );

  fprintf(f, ");");

  return commit_sql_stream(h);
}






static void ss_init(FILE *f, pueo_db_handle_t * h)
{
  //////sun sensor db
  fprintf(f,"CREATE TABLE IF NOT EXISTS sun_sensors ( uid %s, time %s NOT NULL",
      h->type == DB_SQLITE  ? DB_INDEX_DEF_SQLITE : DB_INDEX_DEF_PGSQL,
      h->type == DB_SQLITE  ? DB_TIME_TYPE_SQLITE : DB_TIME_TYPE_PGSQL);

  for (int i = 0 ; i < PUEO_SS_NUM_SENSORS; i++)
  {
    fprintf(f, ", x1_ss%d INTEGER, x2_ss%d INTEGER, y1_ss%d INTEGER, y2_ss%d INTEGER, tempSS_ss%d REAL, tempADS1220_ss%d REAL", i,i,i,i,i,i);
  }
  fprintf(f,");\n");

  DB_MAKE_INDEX(sun_sensor, time)

}

static void nav_att_init(FILE *f, pueo_db_handle_t *h)
{
  fprintf(f, "CREATE TABLE IF NOT EXISTS nav_atts (uid %s, readout_time %s NOT NULL, gps_time %s not NULL,"
             "lat REAL, lon REAL, alt REAL, heading REAL, heading_sigma REAL, pitch REAL, pitch_sigma REAL, roll REAL, roll_sigma REAL,"
             "hdop REAL, vdop REAL, source CHAR(1), nsats INTEGER, flags INTEGER, temperature INTEGER, antenna_current_0 INTEGER,"
			 "antenna_current_1 INTEGER, antenna_current_2 INTEGER);\n",
            h->type == DB_SQLITE  ? DB_INDEX_DEF_SQLITE : DB_INDEX_DEF_PGSQL,
            h->type == DB_SQLITE  ? DB_TIME_TYPE_SQLITE : DB_TIME_TYPE_PGSQL,
            h->type == DB_SQLITE  ? DB_TIME_TYPE_SQLITE : DB_TIME_TYPE_PGSQL);

  DB_MAKE_INDEX(nav_att, readout_time)

}

static void nav_pos_init(FILE *f, pueo_db_handle_t *h)
{
  fprintf(f, "CREATE TABLE IF NOT EXISTS nav_poss (uid %s, readout_time %s NOT NULL, gps_time %s not NULL,"
             "lat REAL, lon REAL, alt REAL,"
             "hdop REAL, vdop REAL, source CHAR(1), nsats INTEGER, flags INTEGER);\n",
            h->type == DB_SQLITE  ? DB_INDEX_DEF_SQLITE : DB_INDEX_DEF_PGSQL,
            h->type == DB_SQLITE  ? DB_TIME_TYPE_SQLITE : DB_TIME_TYPE_PGSQL,
            h->type == DB_SQLITE  ? DB_TIME_TYPE_SQLITE : DB_TIME_TYPE_PGSQL);

  DB_MAKE_INDEX(nav_pos, readout_time)

}

static void single_wf_init(FILE * f, pueo_db_handle_t *h)
{

  fprintf(f,"CREATE TABLE IF NOT EXISTS single_waveforms ( uid %s, time %s NOT NULL, run INTEGER, event INTEGER, channel INTEGER, max INTEGER, min INTEGER, rms REAL, subsecond REAL);\n",
      h->type == DB_SQLITE  ? DB_INDEX_DEF_SQLITE : DB_INDEX_DEF_PGSQL,
      h->type == DB_SQLITE  ? DB_TIME_TYPE_SQLITE : DB_TIME_TYPE_PGSQL);

  DB_MAKE_INDEX(single_waveform, time)
}

static void slow_init(FILE * f, pueo_db_handle_t *h)
{

  fprintf(f, "CREATE TABLE IF NOT EXISTS slow_packets ( uid %s, time %s NOT NULL, ncmds INTEGER, time_since_last_cmd INTEGER, "
             "last_cmd INTEGER, sipd_uptime INTEGER, cpu_uptime INTEGER, can_ping_world INTEGER, starlink_on INTEGER, "
             "los_on INTEGER, gpu_present INTEGER, nic_present INTEGER, turf_seen INTEGER, hsk_seen INTEGER, ss_seen INTEGER, "
             "current_run INTEGER, current_run_secs INTEGER, current_run_events INTEGER, acqd_running INTEGER, "
             "prioritizerd_running INTEGER, current_run_rf_events INTEGER, hsk_uptime INTEGER, "
             "pals_A_index INTEGER, pals_A_free INTEGER, pals_B_index INTEGER, pals_B_free INTEGER, "
             "ssd0_free INTEGER, ssd1_free INTEGER, ssd2_free INTEGER, ssd3_free INTEGER, ssd4_free INTEGER, "
             "heading_abx INTEGER, heading_boreas INTEGER, heading_cpt7 INTEGER, turf_fix_type INTEGER, "
             "NIC_temperature INTEGER, SFC_temperature INTEGER,  pwr_from_sun INTEGER, pwr_usage INTEGER, "
             "battery_state INTEGER, AMPA_current INTEGER,  VPol_Crate_on INTEGER, HPol_Crate_on INTEGER, SFC_Voltage_on INTEGER, "
             "VPol_Current INTEGER, HPol_Current INTEGER,  SFC_Current INTEGER\n",
      h->type == DB_SQLITE  ? DB_INDEX_DEF_SQLITE : DB_INDEX_DEF_PGSQL,
      h->type == DB_SQLITE  ? DB_TIME_TYPE_SQLITE : DB_TIME_TYPE_PGSQL);
  for(int i = 0; i < PUEO_NUM_L2; i++) {
    fprintf(f, ", L2_rates_V_%i INTEGER", i);
  }
  for(int i = 0; i < PUEO_NUM_L2; i++) {
    fprintf(f, ", L2_rates_H_%i INTEGER", i);
  }
  fprintf(f, ");");

  DB_MAKE_INDEX(slow_packet, time);
}

static void cmd_echo_init(FILE *f, pueo_db_handle_t *h)
{
  fprintf(f, "CREATE TABLE IF NOT EXISTS cmd_echos (uid %s, time %s NOT NULL, len INTEGER, count INTEGER, data %s);\n",
      h->type == DB_SQLITE  ? DB_INDEX_DEF_SQLITE : DB_INDEX_DEF_PGSQL,
      h->type == DB_SQLITE  ? DB_TIME_TYPE_SQLITE : DB_TIME_TYPE_PGSQL,
      h->type == DB_SQLITE  ? "TEXT" : "BYTEA" );

  DB_MAKE_INDEX(cmd_echo, time);
}


static void logs_init (FILE * f, pueo_db_handle_t *h)
{
  fprintf(f, "CREATE TABLE IF NOT EXISTS logs (uid %s, time %s NOT NULL, is_until BOOLEAN, since_or_until INTEGER, daemon VARCHAR, grep VARCHAR, logs VARCHAR);\n",
  h->type == DB_SQLITE  ? DB_INDEX_DEF_SQLITE : DB_INDEX_DEF_PGSQL,
  h->type == DB_SQLITE  ? DB_TIME_TYPE_SQLITE : DB_TIME_TYPE_PGSQL);
  DB_MAKE_INDEX(log, time);
}

static int init_db(pueo_db_handle_t * h)
{
  if ( 0 == (h->flags & PUEO_DB_MAYBE_INIT_TABLES)) return 0;

#define DB_MAYBE_CREATE_HSK_TEMPLATE(X,DB)\
    const char * X##_create_string_##DB = "CREATE TABLE IF NOT EXISTS " #X "s (uid " DB_INDEX_DEF_##DB ",  time " DB_TIME_TYPE_##DB " NOT NULL , device TEXT, sensor TEXT, "#X " REAL);\n";\


  FILE * f = begin_sql_stream(h);

  if (h->type != DB_SQLITE && (h->flags  & PUEO_DB_INIT_WITH_TIMESCALEDB))
    fputs("CREATE EXTENSION IF NOT EXISTS timescaledb CASCADE;\n",f);


#define DB_MAYBE_CREATE_HSK(X) \
  DB_MAYBE_CREATE_HSK_TEMPLATE(X,PGSQL) \
  DB_MAYBE_CREATE_HSK_TEMPLATE(X,SQLITE)\
  if (h->type == DB_SQLITE) { fputs(X##_create_string_SQLITE,f); }\
  else { fputs(X##_create_string_PGSQL,f); }\
  DB_MAKE_INDEX(X,time)


  DB_MAYBE_CREATE_HSK(temperature)
  DB_MAYBE_CREATE_HSK(voltage)
  DB_MAYBE_CREATE_HSK(current)
  DB_MAYBE_CREATE_HSK(power)
  DB_MAYBE_CREATE_HSK(flag)
  DB_MAYBE_CREATE_HSK(magnetic_field)

  ss_init(f,h);
  nav_att_init(f,h);
  single_wf_init(f,h);
  cmd_echo_init(f,h);
  logs_init(f,h);
  slow_init(f,h);
  daq_hsk_init(f,h);
  daq_hsk_summary_init(f,h);
  startracker_init(f,h);
  timemark_init(f,h);

  commit_sql_stream(h);

  return 0;
}


//polyfill for SQLITE so can use same to_timestamp as in pgsql
#ifdef SQLITE_ENABLED
static void sqlite_to_timestamp(sqlite3_context *ctx, int nargs, sqlite3_value ** args)
{
  if (nargs < 1)
  {
    sqlite3_result_null(ctx);
    return;
  }

  int64_t as_int = sqlite3_value_int64(args[0]);
  if (as_int == 0)
  {
    // treat 0 as NULL time
    sqlite3_result_null(ctx);
    return;
  }

  time_t t = (time_t) as_int;
  struct tm tm = {0};
  gmtime_r(&t, &tm);

  char formatted[32];
  size_t len = strftime(formatted,sizeof(formatted), "%Y-%m-%d %H:%M:%S", &tm);
  sqlite3_result_text(ctx, formatted, len, SQLITE_TRANSIENT);
}
#endif
