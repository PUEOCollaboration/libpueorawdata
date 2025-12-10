#include "pueo/rawdata.h"
#include "pueo/rawio.h"
#include <stdint.h>

#define PUEO_MAX_FILE_DOWNLOAD_LENGTH 65000


int main(int nargs, char ** args)
{
  char* infile = args[1];
  char* outfile = args[2];

  pueo_handle_t h;
  pueo_handle_init(&h, infile,"r");

  pueo_file_download_t im;

  if (pueo_read_file_download(&h, &im) <= 0)
  {
    printf("Hmm... didn't get any bytes?\n");
    return 1;
  }


  if (outfile)
  {
    FILE* file;
    file = fopen(outfile, "wb");

    fwrite(im.bytes, im.len, 1, file);

    fclose(file);
  }

  return 0; 
}
