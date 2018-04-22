
// MyMfcSimpleD3D12Test1.h : PROJECT_NAME アプリケーションのメイン ヘッダー ファイルです。
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH に対してこのファイルをインクルードする前に 'stdafx.h' をインクルードしてください"
#endif

#include "resource.h"		// メイン シンボル


// CMyMfcSimpleD3D12Test1App:
// このクラスの実装については、MyMfcSimpleD3D12Test1.cpp を参照してください。
//

class CMyMfcSimpleD3D12Test1App : public CWinApp
{
public:
	CMyMfcSimpleD3D12Test1App();

// オーバーライド
public:
	virtual BOOL InitInstance();

// 実装

	DECLARE_MESSAGE_MAP()
};

extern CMyMfcSimpleD3D12Test1App theApp;