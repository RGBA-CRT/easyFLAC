// this osal uses default file io functions(fopen).
// in windows, text encoding will ansi.

#include "common_callback.h"
#include "easyFLAC.h"
#include <pthread.h>

const char* osal_type = "posix";

FLAC__StreamDecoderInitStatus osal_flacOpenFile(FLAC__StreamDecoder* decoder,
                                                const char* filename,
                                                EASYFLAC_HANDLE handle) {

  // save filename
  size_t filename_len = strlen(filename) + 1;
  handle->filePath    = (char*)malloc(filename_len);
  strcpy_s(handle->filePath, filename_len, filename);

  return FLAC__stream_decoder_init_file(decoder,
                                        filename,
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
}

bool osal_getFilePath(EASYFLAC_HANDLE handle, char* file_path,
                      size_t file_path_len) {
  strcpy_s(file_path, file_path_len, handle->filePath);
  return true;
}

void* osal_createMutex() {
  pthread_mutex_t* mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex, NULL);
  return (void*)mutex;
}
void osal_deleteMutex(void* mutex_handle) {
  pthread_mutex_t* mutex = (pthread_mutex_t*)mutex_handle;
  pthread_mutex_destroy(mutex);
  free(mutex);
}
bool osal_lockMutex(void* mutex_handle, int timeout) {
  pthread_mutex_t* mutex = (pthread_mutex_t*)mutex_handle;
  pthread_mutex_lock(mutex);
  return true;
}
bool osal_unlockMutex(void* mutex_handle) {
  pthread_mutex_t* mutex = (pthread_mutex_t*)mutex_handle;
  pthread_mutex_unlock(mutex);
  return true;
}