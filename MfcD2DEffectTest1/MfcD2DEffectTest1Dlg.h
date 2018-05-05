
// MfcD2DEffectTest1Dlg.h : ヘッダー ファイル
//

#pragma once
#include <afxwin.h>
#include "LightningLines.hpp"

using Microsoft::WRL::ComPtr;


// CMfcD2DEffectTest1Dlg ダイアログ
class CMfcD2DEffectTest1Dlg : public CDialogEx
{
	// コンストラクション
public:
	explicit CMfcD2DEffectTest1Dlg(CWnd* pParent = NULL); // 標準コンストラクター

	// ダイアログ データ
	enum { IDD = IDD_MFCD2DEFFECTTEST1_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV サポート


	// 実装
private:
	HICON m_hIcon;

	// Direct3D 11.1, Direct2D 1.1 は、Vista では利用できない。
	// Windows 7 の場合 SP1 と Platform Update が適用されていれば利用できる。
	// Windows 8 以降は無印でも利用可能。
	// なお、Direct2D 1.1 は Direct3D 11.1, DXGI 1.2 上に構築されているが、
	// ユーザープログラムでは必ずしも Direct3D 11.1, DXGI 1.2 のインターフェイスを作成する必要はなく、
	// Direct3D 11.0, DXGI 1.1 までのインターフェイスでも対応可能らしい。
private:
	ComPtr<ID3D11Device> m_d3dDevice;
	ComPtr<ID3D11DeviceContext> m_d3dDeviceContext;
	ComPtr<IDXGISwapChain> m_dxgiSwapChain;
	ComPtr<ID2D1Factory1> m_d2dFactory;
	ComPtr<ID2D1Device> m_d2dDevice;
	ComPtr<ID2D1DeviceContext> m_d2dDeviceContext;
	ComPtr<ID2D1SolidColorBrush> m_d2dLightningBrush;
	ComPtr<ID2D1Bitmap1> m_d2dBackBufferBitmap;
	ComPtr<ID2D1Bitmap1> m_d2dWorkBitmap;
	ComPtr<ID2D1Effect> m_d2dGaussianBlurEffect;

	std::unique_ptr<MyLightningGenerator> m_lightningGen;

private:
	CStatic m_ddxcPicture1;
private:
	CButton m_ddxcCheckBloom;

private:
	bool InitDirectX(CWnd& target);
	void DrawByDirectX();

	void InvalidatePicture1()
	{
		CRect targetRect;
		m_ddxcPicture1.GetWindowRect(targetRect);
		this->ScreenToClient(targetRect);
		this->InvalidateRect(targetRect);
	}

protected:
	// 生成された、メッセージ割り当て関数
	virtual BOOL OnInitDialog() override;
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonUpdate();
	afx_msg void OnBnClickedCheckBloom();
};
