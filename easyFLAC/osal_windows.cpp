// for Windows(Unicode,UTF16)
// CreateFileWを使う

#include "common_callback.h"
#include "easyFLAC.h"
#include <Windows.h>

const char* osal_type = "windows(unicode)";

typedef struct {
  HANDLE file_handle;
  uint64_t file_len;
} osal_win_data;

// ==================
// callbacks
// ==================
FLAC__StreamDecoderReadStatus
read_callback_win(const FLAC__StreamDecoder* decoder, FLAC__byte buffer[],
                  size_t* bytes, void* client_data) {
  EASYFLAC_HANDLE ef_handle = (EASYFLAC_HANDLE)client_data;
  osal_win_data* osal_data  = (osal_win_data*)ef_handle->io_handle;
  DWORD dwAB;

  if (!ReadFile(osal_data->file_handle,
                buffer,
                (*bytes) * sizeof(FLAC__byte),
                &dwAB,
                NULL)) {
    *bytes = 0;
    return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
  }
  *bytes = dwAB;

  if (*bytes == 0) { return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM; }

  return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

FLAC__StreamDecoderSeekStatus
seek_callback_win(const FLAC__StreamDecoder* decoder,
                  FLAC__uint64 absolute_byte_offset, void* client_data) {
  EASYFLAC_HANDLE ef_handle = (EASYFLAC_HANDLE)client_data;
  osal_win_data* osal_data  = (osal_win_data*)ef_handle->io_handle;

  LARGE_INTEGER li;
  li.QuadPart = absolute_byte_offset;
  li.LowPart  = SetFilePointer(
      osal_data->file_handle, li.LowPart, &li.HighPart, FILE_BEGIN);

  if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
    return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
  }

  return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus
tell_callback_win(const FLAC__StreamDecoder* decoder,
                  FLAC__uint64* absolute_byte_offset, void* client_data) {
  EASYFLAC_HANDLE ef_handle = (EASYFLAC_HANDLE)client_data;
  osal_win_data* osal_data  = (osal_win_data*)ef_handle->io_handle;

  LARGE_INTEGER li;
  li.QuadPart = 0;
  li.LowPart  = SetFilePointer(
      osal_data->file_handle, li.LowPart, &li.HighPart, FILE_CURRENT);

  if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
    return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
  }

  *absolute_byte_offset = li.QuadPart;

  return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus
length_callback_win(const FLAC__StreamDecoder* decoder,
                    FLAC__uint64* stream_length, void* client_data) {
  EASYFLAC_HANDLE ef_handle = (EASYFLAC_HANDLE)client_data;
  osal_win_data* osal_data  = (osal_win_data*)ef_handle->io_handle;

  LARGE_INTEGER li;
  li.LowPart = GetFileSize(osal_data->file_handle, (LPDWORD)&li.HighPart);

  if ((li.LowPart == INVALID_FILE_SIZE) && (GetLastError() != NO_ERROR))
    return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
  else {
    *stream_length = (FLAC__uint64)li.QuadPart;
    return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
  }
}

FLAC__bool eof_callback_win(const FLAC__StreamDecoder* decoder,
                            void* client_data) {
  EASYFLAC_HANDLE ef_handle = (EASYFLAC_HANDLE)client_data;
  osal_win_data* osal_data  = (osal_win_data*)ef_handle->io_handle;

  FLAC__uint64 file_ptr;
  if (tell_callback_win(decoder, &file_ptr, client_data) !=
      FLAC__STREAM_DECODER_TELL_STATUS_OK)
    return true;

  return (osal_data->file_len <= file_ptr);
}

// ==================
// osal impl
// ==================
FLAC__StreamDecoderInitStatus osal_flacOpenFile(FLAC__StreamDecoder* decoder,
                                                const char* filename,
                                                EASYFLAC_HANDLE handle) {

  // init osal_data
  handle->io_handle        = malloc(sizeof(osal_win_data));
  osal_win_data* osal_data = (osal_win_data*)handle->io_handle;
  memset(osal_data, 0x00, sizeof(osal_win_data));

  // openfile
  LPCWSTR filename_utf16 = (LPCWSTR)filename;
  osal_data->file_handle =
      CreateFileW(filename_utf16,
                  GENERIC_READ,
                  FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                  NULL,
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL,
                  NULL);
  if (osal_data->file_handle == INVALID_HANDLE_VALUE) {
    osal_data->file_handle = NULL;
    int gle                = GetLastError();
    fprintf(stderr, "[easyFLAC] GLE:%d\r\n", gle);
    return FLAC__STREAM_DECODER_INIT_STATUS_ERROR_OPENING_FILE;
  }

  // set file length
  length_callback_win(NULL, &osal_data->file_len, handle);

  //ファイル名を保存しておく
  size_t filename_len = lstrlenW(filename_utf16) + 1;
  handle->filePath    = (char*)malloc(filename_len * sizeof(wchar_t));
  lstrcpynW((LPWSTR)handle->filePath, filename_utf16, filename_len);

  return FLAC__stream_decoder_init_stream(decoder,
                                          read_callback_win,
                                          seek_callback_win /*seek*/,
                                          tell_callback_win /*tell*/,
                                          length_callback_win /*length*/,
                                          eof_callback_win /*eof*/,
                                          write_callback,
                                          metadata_callback,
                                          error_callback,
                                          handle);
}

void osal_flacCloseFile(EASYFLAC_HANDLE handle) {
  if (handle->filePath != NULL) {
    free(handle->filePath);
    handle->filePath = NULL;
  }

  // release osal datas
  osal_win_data* osal_data = (osal_win_data*)handle->io_handle;

  CloseHandle(osal_data->file_handle);
  osal_data->file_len = 0;

  memset(osal_data, 0x00, sizeof(osal_win_data));
  free(osal_data);

  handle->io_handle = NULL;
}

bool utf16_to_utf8(const wchar_t* input, char* output, size_t output_length) {
  int iconv_ret =
      WideCharToMultiByte(CP_UTF8, 0, input, -1, output, output_length, 0, 0);
  return iconv_ret != 0;
}

bool osal_getFilePath(EASYFLAC_HANDLE handle, char* file_path,
                      size_t file_path_len) {
  return utf16_to_utf8((wchar_t*)handle->filePath, file_path, file_path_len);
}

//----------

void* osal_createMutex() {
  LPCRITICAL_SECTION ret = (LPCRITICAL_SECTION)malloc(sizeof(CRITICAL_SECTION));
  InitializeCriticalSection(ret);
  return (void*)ret;
}
void osal_deleteMutex(void* mutex_handle) {
  DeleteCriticalSection((LPCRITICAL_SECTION)mutex_handle);
  free(mutex_handle);
}
bool osal_lockMutex(void* mutex_handle, int timeout) {
  EnterCriticalSection((LPCRITICAL_SECTION)mutex_handle);
  return true;
}
bool osal_unlockMutex(void* mutex_handle) {
  LeaveCriticalSection((LPCRITICAL_SECTION)mutex_handle);
  return true;
}

//==============
// bool easyFlac_Utf8ToAnsiWindows(const char* input, char* output, size_t
// output_length){
// 	//size_t input_length;
// 	//input_length=MultiByteToWideChar(CP_UTF8, 0, input, -1, NULL, 0);

// 	const size_t utf16_buf_len = 256;
// 	wchar_t utf16_buf[utf16_buf_len];
// 	// metadataでそんなに大きなテキストは来ないでしょうという判断
// 	// ちゃんと作るならinput_length*6ぐらいで確保すればいいはず（？）

// 	// 一旦UTF16に変換してからAnsi(sjis)にする
// 	MultiByteToWideChar(CP_UTF8, 0, input, -1, utf16_buf, utf16_buf_len);
// 	WideCharToMultiByte(CP_ACP, 0, utf16_buf, utf16_buf_len, output,
// output_length, 0, 0);

// 	return true;
// }

// bool easyFlac_Utf8ToLocalEncodeing(const char* input, char* output, size_t
// output_length){
// #if (WINVER >= 0x0400)
// #ifdef EASYFLAC_TEXT_ENCODEING_ANSI
// 	return easyFlac_Utf8ToAnsiWindows(input, output, output_length);
// #else
// 	#error Not implement
// #endif
// #else
// 	#error You must write iconv code here for your system.
// #endif
// }