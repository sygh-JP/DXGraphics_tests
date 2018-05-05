
// MfcD2DEffectTest1.h : PROJECT_NAME アプリケーションのメイン ヘッダー ファイルです。
//

#pragma once

#ifndef __AFXWIN_H__
#error "PCH に対してこのファイルをインクルードする前に 'stdafx.h' をインクルードしてください"
#endif

#include "resource.h" // メイン シンボル


// CMfcD2DEffectTest1App:
// このクラスの実装については、MfcD2DEffectTest1.cpp を参照してください。
//

class CMfcD2DEffectTest1App : public CWinApp
{
public:
	CMfcD2DEffectTest1App();

	// オーバーライド
public:
	virtual BOOL InitInstance();

	// 実装

	DECLARE_MESSAGE_MAP()
};

extern CMfcD2DEffectTest1App theApp;
