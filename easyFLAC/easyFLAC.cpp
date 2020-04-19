#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include "easyFLAC.h"
#include "osal.h"

EASY_FLAC_HANDLE __CALLTYPE FLAC_openFile(const char *FileName){
	//handle作成
	EASY_FLAC* handle = (EASY_FLAC*)malloc(sizeof(EASY_FLAC));	
	memset(handle, 0x00, sizeof(EASY_FLAC));
	
	//libFLAC初期化
	handle->decoder=FLAC__stream_decoder_new();
	if(handle->decoder == NULL) 
		return NULL;

	FLAC__stream_decoder_set_md5_checking(handle->decoder, true);

	// metadata callbackにvorbis commentが届くようにする
	FLAC__stream_decoder_set_metadata_respond(handle->decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT);

	handle->init_status = osal_flacOpenFile(handle->decoder, FileName, handle);
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

void __CALLTYPE FLAC_close(EASY_FLAC_HANDLE handle){
	// file close
	osal_flacCloseFile(handle);
	
	// release buffer
	if(handle->buffer){
		free(handle->buffer);
		handle->buffer=NULL
	}

	// delete metadata of vorbis-comment 
	if(handle->vorbis_comment){
		FLAC__metadata_object_delete(handle->vorbis_comment);
		handle->vorbis_comment = NULL;
	}

	// decoder delete
	FLAC__stream_decoder_delete(handle->decoder);

	// free EASY_FLAC_HANDLE
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


char * __CALLTYPE FLAC_makeInfomationString(EASY_FLAC_HANDLE handle,FLAC__StreamMetadata *tags){
	if(handle==NULL) return "(null)";

	char* ret=(char*)malloc(INFOMATION_STRING_SIZE);
	int p;

	const size_t path_len=256;
	char filePath[path_len];
	if(!osal_getFilePath(handle, filePath, path_len))
		strcpy_s(filePath, sizeof(filePath), "<unsupported>");
	
	p = sprintf_s(ret,INFOMATION_STRING_SIZE,
		"file  : %s\n"
		"channels     : %dch\n"
		"sample rate  : %dHz\n"
		"bit/sample   : %dbit\n"
		"total samples: %d samples\n",
		filePath,
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
			//const size_t iconv_buffer_length=256;
			//char iconv_buffer[iconv_buffer_length];
			//easyFlac_Utf8ToLocalEncodeing((char*)comments->comments[i].entry,iconv_buffer,iconv_buffer_length);

			p+=sprintf_s(ret+p,INFOMATION_STRING_SIZE-p,
				"entry[%02d] %s\n",
				i,(char*)comments->comments[i].entry);
		}
	}
	realloc(ret,p+1);
	return ret;
}