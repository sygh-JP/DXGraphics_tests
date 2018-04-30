
// MfcD3D11ParticleTest1.h : PROJECT_NAME アプリケーションのメイン ヘッダー ファイルです。
//

#pragma once

#ifndef __AFXWIN_H__
#error "PCH に対してこのファイルをインクルードする前に 'stdafx.h' をインクルードしてください"
#endif

#include "resource.h" // メイン シンボル


// CMfcD3D11ParticleTest1App:
// このクラスの実装については、MfcD3D11ParticleTest1.cpp を参照してください。
//

class CMfcD3D11ParticleTest1App : public CWinApp
{
public:
	CMfcD3D11ParticleTest1App();

	// オーバーライド
public:
	virtual BOOL InitInstance();

	// 実装

	DECLARE_MESSAGE_MAP()
};

extern CMfcD3D11ParticleTest1App theApp;
