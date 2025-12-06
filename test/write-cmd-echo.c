#include "pueo/rawdata.h"
#include "pueo/rawio.h"

#include <time.h>
#include <stdlib.h>


int main(int nargs, char ** args)
{

  pueo_cmd_echo_t echo  =  {.when = time(NULL), .len_m1 = nargs-2, .count = time(NULL) & 0xffffff };

  for (int i = 1; i < nargs; i++) echo.data[i-1] = atoi(args[i]);

  pueo_handle_t h;
  pueo_handle_init(&h, "cmd-echo.dat","w");
  pueo_write_cmd_echo(&h, &echo);
  printf("{");
  pueo_dump_cmd_echo(stdout, &echo);
  printf("}\n");
  return 0;
}

