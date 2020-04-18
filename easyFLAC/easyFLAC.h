//外部公開API

#include "share/compat.h"
#include "FLAC/stream_decoder.h"
#include "FLAC/metadata.h"

#define __CALLTYPE __stdcall
#define INFOMATION_STRING_SIZE 1024*10
#define EASYFLAC_TEXT_ENCODEING_ANSI

typedef struct{
	char* filePath;
	DWORD sample_rate;
	DWORD channels;
	DWORD bps;	
	FLAC__StreamDecoderState ok;	//decodeing state
	DWORD sampleSize;
	DWORD nowSamples;
	FLAC__uint64 total_samples;
	DWORD blockSamples;
	BYTE *buffer;
	FLAC__StreamDecoder *decoder;
	FLAC__StreamDecoderInitStatus init_status;
	DWORD renderPos;
	BOOL  resume;
}EASY_FLAC;

typedef EASY_FLAC* EASY_FLAC_HANDLE;
typedef unsigned char BYTE;

//audio api
EASY_FLAC_HANDLE __CALLTYPE FLAC_openFile(const char *FileName);
void __CALLTYPE FLAC_close(EASY_FLAC* handle);
DWORD __CALLTYPE FLAC_render(EASY_FLAC_HANDLE handle, BYTE *buffer,DWORD maxSamples);

//seek
void __CALLTYPE FLAC_seek(EASY_FLAC_HANDLE handle, DWORD posSampleNum);

//metadata api
FLAC__StreamMetadata* __CALLTYPE FLAC_getTags(const char *fileName);
void __CALLTYPE FLAC_deleteTags(FLAC__StreamMetadata *tags);
FLAC__StreamMetadata_VorbisComment* __CALLTYPE FLAC_getVorbisCommentFromTags(FLAC__StreamMetadata* tags);
int __CALLTYPE FLAC__metadata_object_vorbiscomment_find_entry_from_std(const FLAC__StreamMetadata *object, unsigned offset, const char *field_name);
char * __CALLTYPE FLAC_getTagVal(FLAC__StreamMetadata *tags,char *fieldName);
char * __CALLTYPE FLAC_makeInfomationString(EASY_FLAC_HANDLE handle,FLAC__StreamMetadata *tags);
void __CALLTYPE FLAC_freeInfomationString(char *InfoText);

/* 引数解説
	FLAC_openFile(ファイル名(ANSI)) ->　ファイル情報構造体兼ハンドルが返る
	FLAC_render(ハンドル,サウンド用バッファ,要求サンプル数)　-> 書き込んだサンプル数が返る
		デコード終了を検知するには、ハンドル内のokがFLAC__STREAM_DECODER_END_OF_STREAMかをチェック
	FLAC_close(ハンドル)　-> ハンドルを閉じる

	FLAC_seek(ハンドル,サンプル位置) -> サンプル位置はTotalSample以内
*/

/* メタデータ(VorbisComment)の扱い方
 *	FLAC_getTags()でメタデータ取得。以後の戻り値はすべてこのデータ由来。
 *	FLAC_getVorbisCommentFromTags()でVorbisCommentセクションをtagsから取り出す。
		開放不要
 *	FLAC__metadata_object_vorbiscomment_find_entry_from_std()
		で指定タグのインデックスを検索できる。
 *	FLAC_getTagVal()で指定タグの右辺を取得できる。ないなら(none)が帰る。
 *	FLAC_makeInfomationString()でファイル情報の文字列を作成。
		おわったらFLAC_freeInfomationString()で開放
 *	いろいろ終わったらFLAC_deleteTags()で開放。おわり*/
