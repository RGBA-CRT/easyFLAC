#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include "easyFLAC.h"

static FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);


EASY_FLAC_HANDLE __CALLTYPE FLAC_openFile(const char *FileName){
	//handle作成
	EASY_FLAC* handle = (EASY_FLAC*)malloc(sizeof(EASY_FLAC));	
	
	//ファイル名を保存しておく
	handle->filePath = (char*)malloc(strlen(FileName)+5);
	strcpy_s(handle->filePath,strlen(FileName)+1, FileName);

	//libFLAC初期化
	handle->decoder=FLAC__stream_decoder_new();
	if(handle->decoder == NULL) 
		return NULL;

	FLAC__stream_decoder_set_md5_checking(handle->decoder, true);

	handle->init_status = FLAC__stream_decoder_init_file(handle->decoder,FileName, write_callback, metadata_callback, error_callback, handle);
	if(handle->init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
		FLAC_close(handle);
		return NULL;
	}

	//オーディオ情報を読み出し
	FLAC__stream_decoder_process_until_end_of_metadata(handle->decoder);
	handle->sampleSize=handle->channels * handle->bps/8;
	if(handle->sampleSize==0){
		FLAC_close(handle);
		return NULL;
	}

	handle->buffer=(BYTE *)malloc(FLAC__MAX_BLOCK_SIZE * handle->sampleSize);
	
	handle->renderPos=0;
	handle->nowSamples=0;
	handle->resume=FALSE;

	return handle;
}

void __CALLTYPE FLAC_close(EASY_FLAC* handle){
	if(handle->buffer!=NULL) free(handle->buffer);
	if(handle->filePath!=NULL) free(handle->filePath);
	FLAC__stream_decoder_delete(handle->decoder);
	free(handle);
	return;
}

DWORD __CALLTYPE FLAC_render(EASY_FLAC_HANDLE handle, BYTE *buffer,DWORD maxSamples){
	DWORD bufPos=0;
	
	//ブロックサイズより出力サイズの方が小さい場合(renderPos使用)
	if( handle->resume == TRUE){
FR_RESUME:
		DWORD outSamples;
		
		bufPos = handle->renderPos;
		if(maxSamples < (handle->blockSamples - handle->renderPos)){
			outSamples = maxSamples;
			handle->resume=TRUE;
			handle->renderPos+=outSamples;
		}else{
			//残りバッファがmaxSampleより少ない
			outSamples = handle->blockSamples - handle->renderPos;
			handle->resume = FALSE;
			handle->renderPos=0;
			handle->ok = FLAC__stream_decoder_get_state(handle->decoder);
		}
		memcpy(buffer, handle->buffer + bufPos*handle->sampleSize , outSamples * handle->sampleSize);
		handle->nowSamples += outSamples; 
		return outSamples;
	}
	
	if(FLAC__stream_decoder_get_state(handle->decoder)==FLAC__STREAM_DECODER_END_OF_STREAM)
		return 0;

	//要求量を越さない程度にデコード
	while(1){
		FLAC__stream_decoder_process_single(handle->decoder);
		if(maxSamples < handle->blockSamples) goto FR_RESUME;

		handle->ok = FLAC__stream_decoder_get_state(handle->decoder);

		memcpy(buffer + bufPos*handle->sampleSize,handle->buffer,handle->blockSamples * handle->sampleSize);
		bufPos+=handle->blockSamples;

		//次であふれるなら終了
		if(bufPos + handle->blockSamples > maxSamples)
			break;

		if(handle->ok == FLAC__STREAM_DECODER_END_OF_STREAM) 
			break;

	}
	handle->nowSamples += bufPos;
	return bufPos;
}

void __CALLTYPE FLAC_seek(EASY_FLAC_HANDLE handle, DWORD posSample){
	if((FLAC__uint64)posSample >= handle->total_samples)
		posSample=(DWORD)handle->total_samples-1;

	handle->renderPos=0;
	handle->resume=FALSE;
	handle->nowSamples=posSample;

	if(handle->ok==FLAC__STREAM_DECODER_END_OF_STREAM)
		FLAC__stream_decoder_reset(handle->decoder);

	FLAC__stream_decoder_seek_absolute(handle->decoder,(FLAC__uint64)posSample);
	return;
}

FLAC__StreamMetadata* __CALLTYPE FLAC_getTags(const char *fileName){
	FLAC__StreamMetadata *tags;
	if( !FLAC__metadata_get_tags(fileName,&tags) ){
		//printf("this file haven't metadata.\n");
		return NULL;
	}else{
		return tags;
	}
}

void __CALLTYPE FLAC_deleteTags(FLAC__StreamMetadata *tags){
	FLAC__metadata_object_delete(tags);
}

void __CALLTYPE FLAC_freeInfomationString(char *InfoText){
	free(InfoText);
}


FLAC__StreamMetadata_VorbisComment* __CALLTYPE FLAC_getVorbisCommentFromTags(FLAC__StreamMetadata* tags){
	if(tags==NULL || tags->type!=FLAC__METADATA_TYPE_VORBIS_COMMENT)
		return NULL;
	return &tags->data.vorbis_comment;
}

int __CALLTYPE FLAC__metadata_object_vorbiscomment_find_entry_from_std(const FLAC__StreamMetadata *object, unsigned offset, const char *field_name){
	return FLAC__metadata_object_vorbiscomment_find_entry_from(object,offset,field_name);
}


char * __CALLTYPE FLAC_getTagVal(FLAC__StreamMetadata *tags,char *fieldName){
	if(tags==NULL)	return "(null)";

	int offset = FLAC__metadata_object_vorbiscomment_find_entry_from_std(tags,0,fieldName);
	if(offset==-1)
		return "(none)";

	char* target = (char*)tags->data.vorbis_comment.comments[offset].entry;
	int i=0;
	while(1){
		if(target[i]=='\0')
			break;
		if(target[i] =='='){
			i++;
			break;
		}
		i++;
	}
	return target+i;
}

bool easyFlac_Utf8ToAnsiWindows(const char* input, char* output, size_t output_length){
	//size_t input_length;
	//input_length=MultiByteToWideChar(CP_UTF8, 0, input, -1, NULL, 0);

	const size_t utf16_buf_len = 256;
	wchar_t utf16_buf[utf16_buf_len];
	// metadataでそんなに大きなテキストは来ないでしょうという判断
	// ちゃんと作るならinput_length*6ぐらいで確保すればいいはず（？）

	// 一旦UTF16に変換してからAnsi(sjis)にする
	MultiByteToWideChar(CP_UTF8, 0, input, -1, utf16_buf, utf16_buf_len);
	WideCharToMultiByte(CP_ACP, 0, utf16_buf, utf16_buf_len, output, output_length, 0, 0);

	return true;
}

bool easyFlac_Utf8ToLocalEncodeing(const char* input, char* output, size_t output_length){
#if (WINVER >= 0x0400) 
#ifdef EASYFLAC_TEXT_ENCODEING_ANSI
	return easyFlac_Utf8ToAnsiWindows(input, output, output_length);
#else
	#error Not implement
#endif
#else
	#error You must write iconv code here for your system.
#endif
}

char * __CALLTYPE FLAC_makeInfomationString(EASY_FLAC_HANDLE handle,FLAC__StreamMetadata *tags){
	if(handle==NULL) return "(null)";

	char* ret=(char*)malloc(INFOMATION_STRING_SIZE);
	int p;
	
	p = sprintf_s(ret,INFOMATION_STRING_SIZE,
		"file  : %s\n"
		"channels     : %dch\n"
		"sample rate  : %dHz\n"
		"bit/sample   : %dbit\n"
		"total samples: %d samples\n",
		handle->filePath,
		handle->channels,
		handle->sample_rate,
		handle->bps,
		handle->total_samples
		);

	if(tags!=NULL){
		FLAC__StreamMetadata_VorbisComment *comments = FLAC_getVorbisCommentFromTags(tags);
		p+=sprintf_s(ret+p,INFOMATION_STRING_SIZE-p,
			" --- TAG META DATA ---\n"
			"num of comments : %d\n"
			"vender : %s\n",
			comments->num_comments,
			comments->vendor_string.entry);
		for(unsigned i=0;i<comments->num_comments;i++){
			// VorbisCommentはUTF8なので、文字コードを変換する
			const size_t iconv_buffer_length=256;
			char iconv_buffer[iconv_buffer_length];
			easyFlac_Utf8ToLocalEncodeing((char*)comments->comments[i].entry,iconv_buffer,iconv_buffer_length);

			p+=sprintf_s(ret+p,INFOMATION_STRING_SIZE-p,
				"entry[%02d] %s\n",
				i,iconv_buffer);
		}
	}
	realloc(ret,p+1);
	return ret;
}

FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	EASY_FLAC_HANDLE handle=(EASY_FLAC_HANDLE)client_data;

	if(handle->total_samples == 0) {
		fprintf(stderr, "ERROR: this example only works for FLAC files that have a total_samples count in STREAMINFO\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	//PCM書き出し（8/16/24ビット全対応した結果↓）
	#define bits (frame->header.bits_per_sample>>3)				// bps÷8
	for(DWORD i = 0; i < frame->header.blocksize; i++) {		//SAMPLE毎
		for(DWORD c = 0 ; c < frame->header.channels ; c++){	//CHANNEL毎
			for(DWORD b=0; b<bits; b++){						//BIT毎
				handle->buffer[
					i*handle->sampleSize+c*bits+b				//1sample(ch * n bit sample)step + channel step + 8bit step
						]=buffer[c][i]>>(b<<3) & 0x000000FF;
			}
		}
	}

	if(frame->header.bits_per_sample == 8){
		for(DWORD i=0; i< frame->header.blocksize*bits*frame->header.channels; i++)
			handle->buffer[i] = (char)handle->buffer[i] + (BYTE)0x80;
	
	}
	
	handle->blockSamples = frame->header.blocksize;

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	EASY_FLAC* handle=(EASY_FLAC*)client_data;
	(void)decoder, (void)client_data;

	/* print some stats */
	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		/* save for later */
		handle->total_samples = metadata->data.stream_info.total_samples;
		handle->sample_rate = metadata->data.stream_info.sample_rate;
		handle->channels = metadata->data.stream_info.channels;
		handle->bps = metadata->data.stream_info.bits_per_sample;
	}
}

void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	(void)decoder, (void)client_data;

	fprintf(stderr, "Got error callback: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
}
