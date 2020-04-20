// dllmain.cpp : DLL アプリケーションのエントリ ポイントを定義します、E
#include "easyFLAC.h"
#include "stdafx.h"


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call,
                      LPVOID lpReserved) {
  switch (ul_reason_for_call) {
  case DLL_PROCESS_ATTACH:
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
  case DLL_PROCESS_DETACH: break;
  }
  return TRUE;
}

EASY_FLAC_HANDLE __CALLTYPE FLAC_openFileW(const char *FileName) {
  return FLAC_openFile(FileName);
}

/*
 * todo: 斁E��コードがファイルパスとタグで混在してる�Eで直ぁE

*/