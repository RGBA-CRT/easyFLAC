//テストプログラム

#include "easyFLAC.h"

#define TEST_SAMPLE_SIZE 40000

int _tmain(int argc, _TCHAR* argv[])
{
	printf("EASY_FLAC_PROTOTYPE - PROGRAMMED BY RGBA_CRT v2017.2.25\n");
	if(argc==1){
		printf("usege: easyFLAC_test.exe filename\n");
		return 1;
	}

	printf("size of EASY_FLAC : %d\n",sizeof(EASY_FLAC));

	FLAC__StreamMetadata *tags=FLAC_getTags(argv[1]);
	
	if( tags==NULL ){
		printf("this file haven't metadata.\n");
	}else{
		FLAC__StreamMetadata_VorbisComment *comments = FLAC_getVorbisCommentFromTags(tags);
		printf("num of comments : %d\nvender : %s\n",comments->num_comments,comments->vendor_string.entry);
		for(unsigned i=0;i<comments->num_comments;i++)
			printf("[%02d] %s\n",i,comments->comments[i].entry);
		printf("title = %d , %s\n",FLAC__metadata_object_vorbiscomment_find_entry_from_std(tags,0,"TITLE"),FLAC_getTagVal(tags,"TITLE"));

	}

	//--------------------------------------
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

	BYTE buffer[TEST_SAMPLE_SIZE*6]={0xFF};
	FILE *fout;
	fopen_s(&fout,"out.rwav","wb");

	DWORD samples;
	while(1){
		samples=FLAC_render(handle,buffer,TEST_SAMPLE_SIZE);
		printf(".");
		fwrite(buffer,samples,handle->sampleSize,fout);
		if(handle->ok==FLAC__STREAM_DECODER_END_OF_STREAM)
			break;
	}

	
	
	printf(FLAC_makeInfomationString(handle,tags));

	FLAC_close(handle);

	fclose(fout);

	printf("\ndone.");

	return 0;
}