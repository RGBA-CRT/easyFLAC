//テストプログラム

#include "easyFLAC.h"

#define TEST_SAMPLE_SIZE 40000

int main(int argc, char* argv[]) {
  printf("EASYFLAC_PROTOTYPE - PROGRAMMED BY RGBA_CRT rev2 2020.4.24\n");

  // version
  const char *libver, *osal;
  FLAC_getVersion(&libver, &osal);
  printf("%s, osal-type:%s\n", libver, osal);

  if (argc == 1) {
    printf("usege: easyFLAC_test.exe filename\n");
    return 1;
  }

  //--------------------------------------
  EASYFLAC_HANDLE handle;

  handle = FLAC_openFile((char*)argv[1]);
  if (handle == NULL) {
    printf("flac open error!\n");
    return 1;
  }

  // basic sound info
  EASYFLAC_FILE_INFO info;
  FLAC_getFileInfo(handle, &info);
  printf("sample rate    : %u Hz\n", info.sample_rate);
  printf("channels       : %u\n", info.channels);
  printf("bits per sample: %u\n", info.bps);
  printf("total samples  : %ld\n", info.total_samples);
  printf("sample uint8_ts   : %ld\n", info.sample_size);

  // info string
  char* info_str = FLAC_makeInfomationString(handle);
  printf(" --- info string ---\n%s\n", info_str);
  FLAC_freeInfomationString(info_str);

  // vorbis comment
  const char* vc_field_name  = "TITLE";
  FLAC__StreamMetadata* tags = FLAC_getVorbisCommentFromHandle(handle);
  const char* comment_utf8   = FLAC_findComment(tags, vc_field_name);
  if (comment_utf8) { printf("%s = %s\n\n", vc_field_name, comment_utf8); }

  // decode
  uint8_t buffer[TEST_SAMPLE_SIZE * 6] = {0xFF};
  FILE* fout;
  fout = fopen("out.rwav", "wb");
  if (!fout) {
    printf("output file open err\n");
    return -1;
  }

  uint32_t decoded_samples;
  FLAC__StreamDecoderState render_ret;
  while (1) {
    render_ret =
        FLAC_render(handle, buffer, TEST_SAMPLE_SIZE, &decoded_samples);
    printf(".");
    fwrite(buffer, decoded_samples, handle->sampleSize, fout);
    if (render_ret == FLAC__STREAM_DECODER_END_OF_STREAM) break;
  }

  FLAC_close(handle);

  fclose(fout);

  printf("\ndone.");

  return 0;
}