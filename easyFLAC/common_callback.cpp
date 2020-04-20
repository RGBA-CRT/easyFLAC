
#include "common_callback.h"

FLAC__StreamDecoderWriteStatus
write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame,
               const FLAC__int32 *const buffer[], void *client_data) {
  EASYFLAC_HANDLE handle = (EASYFLAC_HANDLE)client_data;

  if (handle->total_samples == 0) {
    fprintf(stderr,
            "ERROR: this example only works for FLAC files that have a "
            "total_samples count in STREAMINFO\n");
    return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
  }

  // PCM書き出し（8/16/24ビット全対応した結果↓）
  DWORD sample_bytes = (frame->header.bits_per_sample >> 3);  // bps÷8
  for (DWORD idx = 0; idx < frame->header.blocksize; idx++) { // SAMPLE毎
    for (DWORD ch = 0; ch < frame->header.channels; ch++) {   // CHANNEL毎
      for (DWORD byte_idx = 0; byte_idx < sample_bytes; byte_idx++) { // BIT毎
        handle
            ->buffer[idx * handle->sampleSize + ch * sample_bytes + byte_idx] =
            buffer[ch][idx] >> (byte_idx << 3) & 0x000000FF;
      }
    }
  }

  if (frame->header.bits_per_sample == 8) {
    for (DWORD i = 0;
         i < frame->header.blocksize * sample_bytes * frame->header.channels;
         i++)
      handle->buffer[i] = (char)handle->buffer[i] + (BYTE)0x80;
  }

  handle->blockSamples = frame->header.blocksize;

  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void metadata_callback(const FLAC__StreamDecoder *decoder,
                       const FLAC__StreamMetadata *metadata,
                       void *client_data) {
  EASY_FLAC *handle = (EASY_FLAC *)client_data;
  (void)decoder, (void)client_data;

  /* print some stats */
  if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
    /* save for later */
    handle->total_samples = metadata->data.stream_info.total_samples;
    handle->sample_rate   = metadata->data.stream_info.sample_rate;
    handle->channels      = metadata->data.stream_info.channels;
    handle->bps           = metadata->data.stream_info.bits_per_sample;

  } else if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
    handle->vorbis_comment = FLAC__metadata_object_clone(metadata);
  }
}

void error_callback(const FLAC__StreamDecoder *decoder,
                    FLAC__StreamDecoderErrorStatus status, void *client_data) {
  (void)decoder, (void)client_data;

  fprintf(stderr,
          "Got error callback: %s\n",
          FLAC__StreamDecoderErrorStatusString[status]);
}
