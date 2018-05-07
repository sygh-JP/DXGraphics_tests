
// MfcD3D10Test1Dlg.h : ヘッダー ファイル
//

#pragma once
#include <afxwin.h>

#include "MyD3DManager.h"


// CMfcD3D10Test1Dlg ダイアログ
class CMfcD3D10Test1Dlg : public CDialogEx
{
	// コンストラクション
public:
	explicit CMfcD3D10Test1Dlg(CWnd* pParent = nullptr); // 標準コンストラクター

	// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MFCD3D10TEST1_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV サポート


// 実装
protected:
	HICON m_hIcon;

	// 生成された、メッセージ割り当て関数
	virtual BOOL OnInitDialog() override;
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
private:
	CStatic m_ddxcStaticPict1;
private:
	std::unique_ptr<MyD3DManager> m_d3dManager;
public:
	afx_msg void OnDestroy();
};
