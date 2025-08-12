/* here we have DB Insertion scripts for each datatype we want to insert into the database
 **/
#include <stdio.h>
#include "pueo/rawio.h"
#include "pueo/rawdata.h"
#include "pueo/sensor_ids.h"
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>


#ifdef PGSQL_ENABLED
#include <libpq-fe.h>
#else
typedef void PGconn;
#endif



struct pueo_db_handle
{
  enum
  {
    DB_PGSQL,
    DB_SQLDIR
  } type;

  char * description;

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

    struct
    {
      PGconn * psql;
      FILE * memstream;
      char * buf;
      size_t bufN;
    } psql;

  } backend;

  struct timespec txn_begin;
  struct timespec txn_end;
};


pueo_db_handle_t * pueo_db_handle_open_sqlfiles_dir(const char * dir)
{
  // Attempt to make the directory (it might already exist though)
  errno = 0;
  if (mkdir(dir, 00644))
  {
    if (errno != EEXIST)
    {
      fprintf(stderr,"Could not create %s and it doesn't already exist\n", dir);
      return NULL;
    }
  }

  //now try to open it
  int dirfd = open(dir, O_DIRECTORY | O_RDWR);
  if (dirfd < 0)
  {
    fprintf(stderr,"Could not open %s as directory or don't have permissions to write\n", dir);
    return NULL;
  }

  pueo_db_handle_t * h = calloc(1, sizeof(pueo_db_handle_t));
  h->type = DB_SQLDIR;
  h->backend.sqldir.dirfd = dirfd;
  h->state = DB_READY;
  return h;
}

pueo_db_handle_t * pueo_db_handle_open_pgsql(const char * conninfo)
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
    sprintf(fname,"%02d-%02d-%04dT%02d.%02d.%02d.%03d.sql",
        current_tm.tm_mon+1, current_tm.tm_mday,
        current_tm.tm_year+1900, current_tm.tm_hour,
        current_tm.tm_min, current_tm.tm_sec, h->txn_begin.tv_nsec / 1e6);


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
  else if (h->type == DB_PGSQL)
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

  if (h->type = DB_SQLDIR)
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

  h->state = DB_FAIL;
  return -1;
}




int pueo_db_insert_sensors_telem(pueo_db_handle_t * h, const pueo_sensors_telem_t * t)
{

  //helper local functions

#define DB_INSERT_TEMPLATE(X)\
    const char * X##_insert_string = "INSERT INTO " #X "s (time, device, sensor, "#X ") VALUES (TO_TIME(%u), '%s', '%s',"

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
        return NULL;
    }
  }

  int telem_sensor_print_val(FILE * f, pueo_sensor_telem_t t)
  {
    char sensor_type = pueo_sensor_id_get_type_tag(t.sensor_id);

    if (sensor_type == 'F')
    {
      return fprintf(f,"%f", (double) t.val.fval);
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
     const char * sensor_name = pueo_sensor_id_get_name(t->sensors[i].sensor_id);
     const char * sensor_subsystem = pueo_sensor_id_get_subsystem(t->sensors[i].sensor_id);
     char sensor_kind = pueo_sensor_id_get_kind(t->sensors[i].sensor_id);

     fprintf(f, get_insert_string(sensor_kind), when, sensor_subsystem, sensor_name);
     telem_sensor_print_val(f,t->sensors[i]);
     fprintf(f,"); ");
 }

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

  free(h);
}


