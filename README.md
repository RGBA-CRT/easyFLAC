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

## example
+ filename: [easyFLAC_example.c](easyFLAC_example.c)
+ usage: easyFLAC_example.exe input.flac
+ output: out.rwav
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

---
Programmed by RGBA_CRT 2020  
Project url: https://github.com/RGBA-CRT/easyFLAC/
