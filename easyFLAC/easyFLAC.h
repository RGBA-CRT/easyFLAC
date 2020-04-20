//ŠO•”ŒöŠJAPI
#ifndef EASY_FLAC_EASY_FLAC_H
#define EASY_FLAC_EASY_FLAC_H

#include "FLAC/metadata.h"
#include "FLAC/stream_decoder.h"
#include "share/compat.h"

#define __CALLTYPE __stdcall
#define INFOMATION_STRING_SIZE 1024 * 10
#define EASYFLAC_TEXT_ENCODEING_ANSI
#define EFLAC_WINDOWS

typedef struct {
  char *filePath;
  void *io_handle;
  DWORD sample_rate;
  DWORD channels;
  DWORD bps;
  FLAC__StreamDecoderState status; // decodeing state
  DWORD sampleSize;
  FLAC__uint64 nowSamples;
  FLAC__uint64 total_samples;
  DWORD blockSamples;
  BYTE *buffer;
  FLAC__StreamDecoder *decoder;
  FLAC__StreamDecoderInitStatus init_status;
  DWORD renderPos;
  BOOL resume;
  FLAC__StreamMetadata *vorbis_comment;
} EASY_FLAC;

typedef struct {
  DWORD sample_rate;
  DWORD channels;
  DWORD bps;
  DWORD sample_size;
  FLAC__uint64 total_samples;
} EASY_FLAC_FILE_INFO;

typedef EASY_FLAC *EASY_FLAC_HANDLE;
typedef unsigned char BYTE;

// audio api
EASY_FLAC_HANDLE __CALLTYPE FLAC_openFile(const char *FileName);
void __CALLTYPE FLAC_close(EASY_FLAC *handle);
FLAC__StreamDecoderState __CALLTYPE FLAC_render(EASY_FLAC_HANDLE handle,
                                                BYTE *buffer, DWORD maxSamples,
                                                DWORD *used_length);

// seek
void __CALLTYPE FLAC_seek(EASY_FLAC_HANDLE handle, uint64_t posSampleNum);
void __CALLTYPE FLAC_tell(EASY_FLAC_HANDLE handle, uint64_t* posSampleNum);

// === metadata api ===
FLAC__StreamMetadata *__CALLTYPE FLAC_getTags(const char *fileName);
void __CALLTYPE FLAC_deleteTags(FLAC__StreamMetadata *tags);
FLAC__StreamMetadata_VorbisComment *__CALLTYPE
FLAC_getVorbisCommentFromHandle(EASY_FLAC_HANDLE handle);

// note: text encoding is utf8
char *__CALLTYPE FLAC_makeInfomationString(EASY_FLAC_HANDLE handle);
void __CALLTYPE FLAC_freeInfomationString(char *InfoText);

char *__CALLTYPE FLAC_findComment(EASY_FLAC_HANDLE handle, char *fieldName);

// ======================

#endif