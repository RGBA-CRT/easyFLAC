//�O�����JAPI

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

/* �������
	FLAC_openFile(�t�@�C����(ANSI)) ->�@�t�@�C�����\���̌��n���h�����Ԃ�
	FLAC_render(�n���h��,�T�E���h�p�o�b�t�@,�v���T���v����)�@-> �������񂾃T���v�������Ԃ�
		�f�R�[�h�I�������m����ɂ́A�n���h������ok��FLAC__STREAM_DECODER_END_OF_STREAM�����`�F�b�N
	FLAC_close(�n���h��)�@-> �n���h�������

	FLAC_seek(�n���h��,�T���v���ʒu) -> �T���v���ʒu��TotalSample�ȓ�
*/

/* ���^�f�[�^(VorbisComment)�̈�����
 *	FLAC_getTags()�Ń��^�f�[�^�擾�B�Ȍ�̖߂�l�͂��ׂĂ��̃f�[�^�R���B
 *	FLAC_getVorbisCommentFromTags()��VorbisComment�Z�N�V������tags������o���B
		�J���s�v
 *	FLAC__metadata_object_vorbiscomment_find_entry_from_std()
		�Ŏw��^�O�̃C���f�b�N�X�������ł���B
 *	FLAC_getTagVal()�Ŏw��^�O�̉E�ӂ��擾�ł���B�Ȃ��Ȃ�(none)���A��B
 *	FLAC_makeInfomationString()�Ńt�@�C�����̕�������쐬�B
		���������FLAC_freeInfomationString()�ŊJ��
 *	���낢��I�������FLAC_deleteTags()�ŊJ���B�����*/
