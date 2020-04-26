# easyFLAC
+ Wrapper library of libFLAC.
+ とりあえずWindows用のDLLとして作成

## Feature
+ Decode FLAC file
+ Seek
+ Vorbis Comment acquisition
+ Support 8/16/24bit
+ openFile, getFileInfo, render, closeの4関数でデコード可能
+ stdcall Windows DLL

## デコードのサンプル
引数のファイルを開いて"out.rwav"という名前で生サウンドデータを出力する例  
詳しくはeasyFLAC_example.c , easyFLAC_example.7zへ
```c
#include "easyFLAC.h"
#define TEST_SAMPLE_SIZE 40000

int main(int argc, char* argv[]) {
  printf("EASYFLAC_PROTOTYPE - PROGRAMMED BY RGBA_CRT rev2 2020.4.24\n");
  if (argc == 1)
    return 1;

  EASYFLAC_HANDLE handle;
  handle = FLAC_openFile((char*)argv[1]);
  if (handle == NULL)
    return 1;

  EASYFLAC_FILE_INFO info;
  FLAC_getFileInfo(handle, &info);

  uint8_t buffer[TEST_SAMPLE_SIZE * 6] = {0xFF};
  FILE* fout = fopen("out.rwav", "wb");
  if (!fout) 
    return -1;

  uint32_t decoded_samples;
  FLAC__StreamDecoderState render_ret;
  while (1) {
    render_ret = FLAC_render(handle, buffer, TEST_SAMPLE_SIZE, &decoded_samples);
    printf(".");
    fwrite(buffer, decoded_samples, handle->sampleSize, fout);
    if (render_ret == FLAC__STREAM_DECODER_END_OF_STREAM) break;
  }

  FLAC_close(handle);
  fclose(fout);
  printf("\ndone.");

  return 0;
}
```
  
