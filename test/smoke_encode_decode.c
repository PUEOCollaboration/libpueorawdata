#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <pueo/rawdata.h>
#include <pueo/rawio.h>

/* Small smoke test that constructs a waveform, encodes and decodes it with
   compression flags 0 (none), 1 (zlib), and 2 (zstd) and verifies round-trip equality. */

static void fill_waveform(pueo_single_waveform_t *wf, int nsamples)
{
    memset(wf, 0, sizeof(*wf));
    wf->wf.length = nsamples;
    wf->run = 123;
    wf->event = 456;
    wf->wf.channel_id = 5;
    for (int i = 0; i < nsamples; ++i) {
      wf->wf.data[i] = (uint16_t)(i & 0xffff);
    }
}

static int compare_waveforms(const pueo_single_waveform_t *a, const pueo_single_waveform_t *b)
{
    if (a->run != b->run) return 0;
    if (a->event != b->event) return 0;
    if (a->wf.length != b->wf.length) return 0;
    if (a->wf.channel_id != b->wf.channel_id) return 0;
    for (int i = 0; i < a->wf.length; ++i) {
        if (a->wf.data[i] != b->wf.data[i]) return 0;
    }
    return 1;
}

int main(void)
{
    pueo_single_waveform_t in;
    pueo_single_waveform_t out;
    pueo_encoded_waveform_t enc;

    fill_waveform(&in, 512);

    int flags[] = {0,1,2};
    const char *names[] = {"none","zlib","zstd"};

    int failed = 0;

    for (int i = 0; i < 3; ++i) {
        memset(&enc, 0, sizeof(enc));
        memset(&out, 0, sizeof(out));

        int flag = flags[i];
        int r = pueo_encode_waveform(&in, &enc, flag);
        if (r != 0) {
            printf("encode(%s) failed (r=%d)\n", names[i], r);
            failed++;
            continue;
        }
        r = pueo_decode_waveform(&enc, &out);
        if (r != 0) {
            printf("decode(%s) failed (r=%d)\n", names[i], r);
            failed++;
            continue;
        }
        if (!compare_waveforms(&in, &out)) {
            printf("mismatch after round-trip for %s\n", names[i]);
            failed++;
            continue;
        }
        printf("round-trip %s: OK\n", names[i]);
    }

    return failed ? 1 : 0;
}
