/* here we have DB Insertion scripts for each datatype we want to insert into the database
 **/

#define _GNU_SOURCE

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
  pueo_db_handle_t * h = NULL;
  if (strstr(uri,"postgresql://") == uri)
  {
    h = pueo_db_handle_open_pgsql(uri, flags);
  }
  if (strstr(uri,"timescaledb+postgresql://") == uri)
  {
    h = pueo_db_handle_open_pgsql(uri + strlen("timescaledb+"), flags | PUEO_DB_INIT_WITH_TIMESCALEDB);
  }
  // made up uri scheme for old style pgsql conninfo
  else if (strstr(uri,"PGSQL_CONNINFO:") == uri)
  {
    h = pueo_db_handle_open_pgsql(uri + strlen("PGSQL_CONNINFO:"),flags);
  }
  // made up uri scheme for old style pgsql conninfo but for TIMESCALEDB
  else if (strstr(uri,"TIMESCALEDB_CONNINFO:") == uri)
  {
    h = pueo_db_handle_open_pgsql(uri + strlen("TIMESCALEDB_CONNINFO:"),flags | PUEO_DB_INIT_WITH_TIMESCALEDB);
  }
  else if (strstr(uri,"sqldir://")==uri)
  {
    h = pueo_db_handle_open_sqlfiles_dir(uri + strlen("sqldir://"),flags);
  }
  else if (strstr(uri,"sqlite://")==uri)
  {
    h = pueo_db_handle_open_sqlite(uri + strlen("sqlite://"),flags);
  }
  else return NULL;

  return h;
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
    if (PQresultStatus(r) != PGRES_COMMAND_OK)
    {
      fprintf(stderr,"Problem with psql query: %s\n", PQresultErrorMessage(r));
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


STUB_INSERT_DB(slow)

int pueo_db_insert_ss(pueo_db_handle_t * h, const pueo_ss_t* ss)
{

  FILE * f = begin_sql_stream(h);

  fprintf(f, "INSERT INTO sun_sensors (time");
  for (int i = 0; i < PUEO_SS_NUM_SENSORS; i++)
  {
    fprintf(f,", x1_ss%d, x2_ss%d, y1_ss%d, y2_ss%d, tempSS_ss%d, tempADS1220_ss%d",
        i,i,i,i,i,i);
  }

  fprintf(f, "VALUES(TO_TIMESTAMP(%lu.%09u) ", (uint64_t) ss->readout_time.utc_secs,  (uint32_t) ss->readout_time.utc_nsecs);

  for (int i = 0; i < PUEO_SS_NUM_SENSORS; i++)
  {
        fprintf(f, ", %hu, %hu, %hu, %hu, %f, %f",
        (uint16_t) ss->ss[i].x1, (uint16_t) ss->ss[i].x2,
        (uint16_t) ss->ss[i].y1, (uint16_t) ss->ss[i].y2,
        PUEO_SS_TEMPERATURE_CONVERT(ss->ss[i].tempSS),
        PUEO_SS_TEMPERATURE_CONVERT(ss->ss[i].tempADS1220)
        );
  }

  fprintf(f,")\n");

  return commit_sql_stream(h);
}

int pueo_db_insert_single_waveform(pueo_db_handle_t *h, const pueo_single_waveform_t *wf)
{
  FILE * f = begin_sql_stream(h);

  int max = -INT_MAX;
  int min = INT_MAX;
  double sum2= 0;
  double sum = 0;
  int N = wf->wf.length;
  for (int i = 0; i < N; i++)
  {
    int v = wf->wf.data[i];
    if (v > max) max = v;
    if (v < min) min = v;
    sum += v;
    sum2 += v*v;
  }

  double mean = sum / N;
  double rms = sqrt(mean*mean - sum2/N);


  fprintf(f, "INSERT INTO single_waveforms(time, run, event, channel, max, min, rms)"
             "VALUES (%u.%09u, %d, %d, %d, %d, %d, %f);",
              wf->event_second,  wf->readout_time.utc_nsecs,
              wf->run, wf->event, wf->wf.channel_id, max, min, rms
             );

  return commit_sql_stream(h);
}


int pueo_db_insert_nav_att(pueo_db_handle_t *h, const pueo_nav_att_t * att)
{

  FILE * f = begin_sql_stream(h);
  fprintf(f,"INSERT INTO nav_att (readout_time, gps_time, lat, lon, alt, heading,"
            " heading_sigma, pitch, pitch_sigma, roll, roll_sigma, hdop, vdop, source, nsats, flags"
           " VALUES(TO_TIMESTAMP(%lu.%09u), TO_TIMESTAMP(%lu.%09u), %f, %f, %f, %f,"
           " %f, %f, %f, %f, %f, %f, %f, '%c', %d, %d);",
           (uint64_t) att->readout_time.utc_secs, (uint32_t) att->readout_time.utc_nsecs, (uint64_t) att->gps_time.utc_secs,
           (uint32_t) att->gps_time.utc_nsecs, att->lat, att->lon, att->alt, att->heading,
           att->heading_sigma, att->pitch, att->pitch_sigma, att->roll, att->roll_sigma, att->hdop,
           att->vdop, att->source, att->nsats, att->flags);

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
const char * X##_index_string = "CREATE INDEX IF NOT EXISTS "#X"_time_idx on " #X" s ( " #T "); \n";\
const char * X##_create_TIMESCALEDB = "SELECT create_hypertable('" #X "s', by_range('"#X"'), if_not_exists => TRUE);\n"; \
if (h->type != DB_SQLITE &&  ( h->flags & PUEO_DB_INIT_WITH_TIMESCALEDB)) { fputs(X##_create_TIMESCALEDB,f); }\
else { fputs(X##_index_string, f); }

#define DB_TIME_TYPE_PGSQL "TIMESTAMPTZ"
#define DB_TIME_TYPE_SQLITE "DATETIME"
#define DB_INDEX_DEF_PGSQL "SERIAL"
#define DB_INDEX_DEF_SQLITE "INTEGER PRIMARY KEY AUTOINCREMENT"



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
  fprintf(f, "CREATE TABLE IF NOT EXISTS nav_att (uid %s, readout_time %s NOT NULL, gps_time %s not NULL,"
             "lat REAL, lon REAL, alt REAL, heading REAL, heading_sigma REAL, pitch REAL, pitch_sigma REAL, roll REAL, roll_sigma REAL,"
             "hdop REAL, vdop REAL, source CHAR(1), nsats INTEGER, flags INTEGER);",
            h->type == DB_SQLITE  ? DB_INDEX_DEF_SQLITE : DB_INDEX_DEF_PGSQL,
            h->type == DB_SQLITE  ? DB_TIME_TYPE_SQLITE : DB_TIME_TYPE_PGSQL,
            h->type == DB_SQLITE  ? DB_TIME_TYPE_SQLITE : DB_TIME_TYPE_PGSQL);

  DB_MAKE_INDEX(nav_att, readout_time)

}


static void single_wf_init(FILE * f, pueo_db_handle_t *h)
{

  fprintf(f,"CREATE TABLE IF NOT EXISTS single_waveforms ( uid %s, time %s NOT NULL, run INTEGER, event INTEGER, channel INTEGER, max INTEGER, min INTEGER, rms REAL);\n ",
      h->type == DB_SQLITE  ? DB_INDEX_DEF_SQLITE : DB_INDEX_DEF_PGSQL,
      h->type == DB_SQLITE  ? DB_TIME_TYPE_SQLITE : DB_TIME_TYPE_PGSQL);

  DB_MAKE_INDEX(single_waveform, time)
}

static void cmd_echo_init(FILE *f, pueo_db_handle_t *h)
{
  fprintf(f, "CREATE TABLE IF NOT EXISTS cmd_echos (uid %s, time %s NOT NULL, len INTEGER, count INTEGER, data %s",
      h->type == DB_SQLITE  ? DB_INDEX_DEF_SQLITE : DB_INDEX_DEF_PGSQL,
      h->type == DB_SQLITE  ? DB_TIME_TYPE_SQLITE : DB_TIME_TYPE_PGSQL,
      h->type == DB_SQLITE  ? "TEXT" : "BYTEA" );

  DB_MAKE_INDEX(cmd_echo, time);
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
