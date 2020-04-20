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
  char* filePath;
  void* io_handle;
  DWORD sample_rate;
  DWORD channels;
  DWORD bps;
  DWORD sampleSize;
  DWORD renderPos;
  DWORD blockSamples;
  BOOL resume;
  FLAC__uint64 nowSamples;
  FLAC__uint64 total_samples;
  BYTE* buffer;
  FLAC__StreamDecoder* decoder;
  FLAC__StreamDecoderState status; // decodeing state
  FLAC__StreamMetadata* vorbis_comment;
  FLAC__StreamDecoderInitStatus init_status;
} EASY_FLAC;

typedef struct {
  DWORD sample_rate;
  DWORD channels;
  DWORD bps;
  DWORD sample_size;
  FLAC__uint64 total_samples;
} EASY_FLAC_FILE_INFO;

typedef EASY_FLAC* EASYFLAC_HANDLE;
typedef unsigned char BYTE;

// === audio api ===
EASYFLAC_HANDLE __CALLTYPE FLAC_openFile(const char* FileName);
void __CALLTYPE FLAC_close(EASY_FLAC* handle);

// render wave from flac
// [arg] buffer: wave buffer
// [arg] maxSamples: wave buffer length
// [arg] used_length: 書き込まれたサンプル数をセットするためのポインタ
FLAC__StreamDecoderState __CALLTYPE FLAC_render(EASYFLAC_HANDLE handle,
                                                BYTE* buffer, DWORD maxSamples,
                                                DWORD* used_length);

// seek
void __CALLTYPE FLAC_seek(EASYFLAC_HANDLE handle, uint64_t posSampleNum);
// get playing sample number
void __CALLTYPE FLAC_tell(EASYFLAC_HANDLE handle, uint64_t* posSampleNum);

// === metadata api ===

// get StreamMetaData from file. 
// return pointer must release using FLAC_deleteTags.
FLAC__StreamMetadata* __CALLTYPE FLAC_getTags(const char* fileName);
void __CALLTYPE FLAC_deleteTags(FLAC__StreamMetadata* tags);

// get VorbisComment from current opening file.
FLAC__StreamMetadata_VorbisComment* __CALLTYPE
FLAC_getVorbisCommentFromHandle(EASYFLAC_HANDLE handle);

// note: text encoding is utf8
char* __CALLTYPE FLAC_makeInfomationString(EASYFLAC_HANDLE handle);
void __CALLTYPE FLAC_freeInfomationString(char* InfoText);

char* __CALLTYPE FLAC_findComment(EASYFLAC_HANDLE handle, char* fieldName);

// ======================

#endif