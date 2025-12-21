#define _GNU_SOURCE
#include "pueo/rawdata.h"
#include "pueo/rawio.h"
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>


int ith = 0; 
int main(int nargs, char ** args)
{


  for (int i = 1; i < nargs; i++)
  {
    if (args[i][0]=='-')
    {
      ith= atoi(args[i]+1);
      continue;
    }

    pueo_handle_t h;
    pueo_handle_init(&h, args[i], "r");

    pueo_packet_t * packet = 0;
    int i = 0;
    while (i <= ith)
    {
      int read = pueo_ll_read_realloc(&h, &packet);
      if (read <= 0) break;

      if (i == ith)
      {
        printf("%s ", pueo_packet_name(packet));
      }
      free(packet);
      packet = 0;
      i++;
    }

  }
  printf("\n");

  return 0;

}

