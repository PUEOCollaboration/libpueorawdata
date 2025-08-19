#include "pueo/rawio.h"
#include <stdio.h>


int main(int nargs, char **args)
{
  if (nargs <2)
  {
    fprintf(stderr,"Usage: pueo-init-db [uri]\n");
    fprintf(stderr,"uri could be sqlite://file.db,  sqldir://dirforsqlfiles, pgsql://conninfo, timescaledb://conninfo for pgsql with timescaledb\n");
    return 1;
  }

  pueo_db_handle_t * db = pueo_db_handle_open(args[1], PUEO_DB_MAYBE_INIT_TABLES);

  pueo_db_handle_close(&db);

  return 0;
}
