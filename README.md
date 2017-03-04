# easyFLAC
簡易FLACデコーダ。コールバックとか作らなくていい。8/16/24bit対応

## できること

+ FLACファイルのデコード
+ シーク
+ VorbisCommentの取得
 
## デコードのサンプル
引数のファイルを開いて"out.rwav"という名前で生サウンドデータを出力する例  
詳しくはeasyFLAC_example.c , easyFLAC_example.7zへ
```c
#define TEST_SAMPLE_SIZE 40000
int _tmain(int argc, _TCHAR* argv[]){
	EASY_FLAC_HANDLE handle;

	handle=FLAC_openFile((char*)argv[1]);
	if(handle==NULL){
		printf("flac open error!\n");
		return 1;
	}

	fprintf(stderr, "sample rate    : %u Hz\n", handle->sample_rate);
	fprintf(stderr, "channels       : %u\n", handle->channels);
	fprintf(stderr, "bits per sample: %u\n", handle->bps);
	fprintf(stderr, "total samples  : %ld\n", handle->total_samples);
	fprintf(stderr, "sample bytes   : %ld\n", handle->sampleSize);

	BYTE buffer[TEST_SAMPLE_SIZE*6]={0xFF};	//6は24bit * 2chの意
  FILE *fout;
	fopen_s(&fout,"out.rwav","wb");

	DWORD samples;
	while(1){
		samples=FLAC_render(handle,buffer,TEST_SAMPLE_SIZE);
		printf(".");
		fwrite(buffer,samples,handle->sampleSize,fout);
		if(handle->ok==FLAC__STREAM_DECODER_END_OF_STREAM)  //終了判定はこのようにする
      break;
	}	
	
	FLAC_close(handle);
	fclose(fout);
	printf("\ndone.");
}
```
  
