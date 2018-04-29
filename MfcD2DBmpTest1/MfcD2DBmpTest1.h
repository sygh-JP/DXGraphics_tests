
// MfcD2DBmpTest1.h : MfcD2DBmpTest1 アプリケーションのメイン ヘッダー ファイル
//
#pragma once

#ifndef __AFXWIN_H__
	#error "PCH に対してこのファイルをインクルードする前に 'stdafx.h' をインクルードしてください"
#endif

#include "resource.h"       // メイン シンボル


// CMfcD2DBmpTest1App:
// このクラスの実装については、MfcD2DBmpTest1.cpp を参照してください。
//

class CMfcD2DBmpTest1App : public CWinApp
{
public:
	CMfcD2DBmpTest1App();


// オーバーライド
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// 実装
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CMfcD2DBmpTest1App theApp;
