#include "stdafx.h"

#include "easyFLAC.h"
#include "osal.h"
#include <stdlib.h>
#include <string.h>

EASYFLAC_HANDLE __CALLTYPE FLAC_openFile(const char* FileName) {
  // handle作成
  EASY_FLAC* handle = (EASY_FLAC*)malloc(sizeof(EASY_FLAC));
  memset(handle, 0x00, sizeof(EASY_FLAC));

  // libFLAC初期化
  handle->decoder = FLAC__stream_decoder_new();
  if (handle->decoder == NULL) return NULL;

  FLAC__stream_decoder_set_md5_checking(handle->decoder, true);

  // metadata callbackにvorbis commentが届くようにする
  FLAC__stream_decoder_set_metadata_respond(handle->decoder,
                                            FLAC__METADATA_TYPE_VORBIS_COMMENT);

  // openfile
  handle->init_status = osal_flacOpenFile(handle->decoder, FileName, handle);
  if (handle->init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
    FLAC_close(handle);
    return NULL;
  }

  //オーディオ情報を読み出し
  FLAC__stream_decoder_process_until_end_of_metadata(handle->decoder);
  handle->sampleSize = handle->channels * handle->bps / 8;
  if (handle->sampleSize == 0) {
    FLAC_close(handle);
    return NULL;
  }

  handle->buffer = (uint8_t*)malloc(FLAC__MAX_BLOCK_SIZE * handle->sampleSize);

  handle->renderPos  = 0;
  handle->nowSamples = 0;
  handle->resume     = FALSE;

  handle->decoder_mutex = osal_createMutex();

  return handle;
}

void __CALLTYPE FLAC_close(EASYFLAC_HANDLE handle) {
  // file close
  osal_flacCloseFile(handle);

  // release buffer
  if (handle->buffer) {
    free(handle->buffer);
    handle->buffer = NULL;
  }

  // delete metadata of vorbis-comment
  if (handle->vorbis_comment) {
    FLAC__metadata_object_delete(handle->vorbis_comment);
    handle->vorbis_comment = NULL;
  }

  // decoder delete
  FLAC__stream_decoder_delete(handle->decoder);
  handle->decoder = NULL;

  if (handle->decoder_mutex) osal_deleteMutex(handle->decoder_mutex);

  // free EASYFLAC_HANDLE
  free(handle);
  return;
}

FLAC__StreamDecoderState __CALLTYPE FLAC_render(EASYFLAC_HANDLE handle,
                                                uint8_t* buffer,
                                                uint32_t max_samples,
                                                uint32_t* used_samples) {
  uint32_t bufPos = 0;

  //ブロックサイズより出力サイズの方が小さい場合(renderPos使用)
  if (handle->resume == TRUE) {
  FR_RESUME:
    uint32_t outSamples;

    bufPos = handle->renderPos;
    if (max_samples < (handle->blockSamples - handle->renderPos)) {
      outSamples     = max_samples;
      handle->resume = TRUE;
      handle->renderPos += outSamples;
    } else {
      //残りバッファがmaxSampleより少ない
      outSamples        = handle->blockSamples - handle->renderPos;
      handle->resume    = FALSE;
      handle->renderPos = 0;
      handle->status    = FLAC__stream_decoder_get_state(handle->decoder);
    }
    memcpy(buffer,
           handle->buffer + bufPos * handle->sampleSize,
           outSamples * handle->sampleSize);
    handle->nowSamples += outSamples;
    *used_samples  = outSamples;
    handle->status = FLAC__stream_decoder_get_state(handle->decoder);
    return handle->status;
  }

  if (FLAC__stream_decoder_get_state(handle->decoder) ==
      FLAC__STREAM_DECODER_END_OF_STREAM)
    return FLAC__STREAM_DECODER_END_OF_STREAM;

  //要求量を越さない程度にデコード
  while (1) {
    osal_lockMutex(handle->decoder_mutex, -1);
    FLAC__stream_decoder_process_single(handle->decoder);
    osal_unlockMutex(handle->decoder_mutex);

    if (max_samples < handle->blockSamples) goto FR_RESUME;

    handle->status = FLAC__stream_decoder_get_state(handle->decoder);

    memcpy(buffer + bufPos * handle->sampleSize,
           handle->buffer,
           handle->blockSamples * handle->sampleSize);
    bufPos += handle->blockSamples;

    //次であふれるなら終了
    if (bufPos + handle->blockSamples > max_samples) break;

    if (handle->status == FLAC__STREAM_DECODER_END_OF_STREAM) break;
  }
  handle->nowSamples += bufPos;
  *used_samples = bufPos;
  return handle->status;
}

void __CALLTYPE FLAC_seek(EASYFLAC_HANDLE handle, uint64_t posSample) {
  if (posSample >= handle->total_samples) posSample = handle->total_samples - 1;

  handle->renderPos  = 0;
  handle->resume     = FALSE;
  handle->nowSamples = posSample;

  osal_lockMutex(handle->decoder_mutex, -1);
  {
    if (handle->status == FLAC__STREAM_DECODER_END_OF_STREAM)
      FLAC__stream_decoder_reset(handle->decoder);

    FLAC__stream_decoder_seek_absolute(handle->decoder, posSample);
  }
  osal_unlockMutex(handle->decoder_mutex);
  return;
}

void __CALLTYPE FLAC_tell(EASYFLAC_HANDLE handle, uint64_t* posSampleNum) {
  if (handle == NULL || posSampleNum == NULL) return;

  (*posSampleNum) = handle->nowSamples;
}

FLAC__StreamMetadata* __CALLTYPE
FLAC_getVorbisCommentFromHandle(EASYFLAC_HANDLE handle) {
  if (handle->vorbis_comment == NULL ||
      handle->vorbis_comment->type != FLAC__METADATA_TYPE_VORBIS_COMMENT) {
    return NULL;
  }
  return handle->vorbis_comment;
}

const char* __CALLTYPE FLAC_findComment(FLAC__StreamMetadata* handle,
                                        const char* fieldName) {
  if (handle == NULL) return NULL;
  if (handle->type != FLAC__METADATA_TYPE_VORBIS_COMMENT) return NULL;

  int offset =
      FLAC__metadata_object_vorbiscomment_find_entry_from(handle, 0, fieldName);
  if (offset == -1) return NULL;

  const char* target =
      (char*)handle->data.vorbis_comment.comments[offset].entry;
  int i = 0;
  while (1) {
    if (target[i] == '\0') break;
    if (target[i] == '=') {
      i++;
      break;
    }
    i++;
  }
  return target + i;
}

char* __CALLTYPE FLAC_makeInfomationString(EASYFLAC_HANDLE handle) {
  if (handle == NULL) return NULL;

  char* ret = (char*)malloc(INFOMATION_STRING_SIZE);
  int p;

  const size_t path_len = 256;
  char filePath[path_len];
  if (!osal_getFilePath(handle, filePath, path_len))
    strcpy_s(filePath, sizeof(filePath), "<unsupported>");

  p = sprintf_s(ret,
                INFOMATION_STRING_SIZE,
                "file  : %s\n"
                "channels     : %dch\n"
                "sample rate  : %dHz\n"
                "bit/sample   : %dbit\n"
                "total samples: %d samples\n",
                filePath,
                handle->channels,
                handle->sample_rate,
                handle->bps,
                handle->total_samples);

  FLAC__StreamMetadata* metadata = FLAC_getVorbisCommentFromHandle(handle);
  if (metadata) {
    FLAC__StreamMetadata_VorbisComment* comments =
        &metadata->data.vorbis_comment;
    if (comments) {
      p += sprintf_s(ret + p,
                     INFOMATION_STRING_SIZE - p,
                     " --- TAG META DATA ---\n"
                     "num of comments : %d\n"
                     "vender : %s\n",
                     comments->num_comments,
                     comments->vendor_string.entry);
      for (unsigned i = 0; i < comments->num_comments; i++) {
        p += sprintf_s(ret + p,
                       INFOMATION_STRING_SIZE - p,
                       "entry[%02d] %s\n",
                       i,
                       (char*)comments->comments[i].entry);
      }
    }
  }
  realloc(ret, p + 1);
  return ret;
}

void __CALLTYPE FLAC_freeInfomationString(char* InfoText) { free(InfoText); }

bool __CALLTYPE FLAC_getFileInfo(EASYFLAC_HANDLE handle,
                                 EASYFLAC_FILE_INFO* info) {
  if (handle == NULL || info == NULL) return false;

  info->sample_rate   = handle->sample_rate;
  info->channels      = handle->channels;
  info->bps           = handle->bps;
  info->sample_size   = handle->sampleSize;
  info->total_samples = handle->total_samples;
  return true;
}

void __CALLTYPE FLAC_getVersion(const char** version, const char** osal_type_in){
  *version = "easyFlac " EASYFLAC_VERSION " build: " __TIMESTAMP__;
  *osal_type_in = osal_type;
}