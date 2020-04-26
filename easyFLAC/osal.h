// os abstraction layter
// 主にファイルIO

#ifndef EASYFLAC_OSAL_H
#define EASYFLAC_OSAL_H
#include "easyFLAC.h"

extern const char* osal_type;

// open flac file
// filename's text encoding are depends your system.
// this function sets handle.decoder, .filePath, .io_handle
// this function must set flac-api's userdata to EASYFLAC_HANDLE
FLAC__StreamDecoderInitStatus osal_flacOpenFile(FLAC__StreamDecoder* decoder,
                                                const char* filename,
                                                EASYFLAC_HANDLE handle);

void osal_flacCloseFile(EASYFLAC_HANDLE handle);

bool osal_getFilePath(EASYFLAC_HANDLE handle, char* file_path,
                      size_t file_path_len);

void* osal_createMutex();
void osal_deleteMutex(void* mutex_handle);
bool osal_lockMutex(void* mutex_handle, int timeout);
bool osal_unlockMutex(void* mutex_handle);

#endif