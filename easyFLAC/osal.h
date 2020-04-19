// os abstraction layter
// 主にファイルIO

#ifndef EASYFLAC_OSAL_H
#define EASYFLAC_OSAL_H
#include "easyFLAC.h"

// open flac file
// filename's text encoding are depends your system.
// this function sets handle.decoder, .filePath, .io_handle
// this function must set flac-api's userdata to EASY_FLAC_HANDLE
FLAC__StreamDecoderInitStatus osal_flacOpenFile(FLAC__StreamDecoder *decoder,
                                                const char *filename,
                                                EASY_FLAC_HANDLE handle);

void osal_flacCloseFile(EASY_FLAC_HANDLE handle);

bool osal_getFilePath(EASY_FLAC_HANDLE handle, char *file_path,
                      size_t file_path_len);

#endif