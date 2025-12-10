#include "pueo/rawdata.h"
#include "pueo/rawio.h"

#define PUEO_MAX_FILE_DOWNLOAD_LENGTH 65000

struct image
{
  uint8_t bytes[PUEO_MAX_FILE_DOWNLOAD_LENGTH]
};

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


  struct image f;
  for(int i=0; i<PUEO_MAX_FILE_DOWNLOAD_LENGTH; i++)
  {
    f.bytes[i] = im.bytes[i];
  }

  if (outfile)
  {
    FILE* file;
    file = fopen(outfile, "wb");

    fwrite(&f, im.len, 1, file);

    fclose(file);
  }

  return 0; 
}