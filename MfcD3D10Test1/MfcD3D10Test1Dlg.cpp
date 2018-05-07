
// MfcD3D10Test1Dlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "MfcD3D10Test1.h"
#include "MfcD3D10Test1Dlg.h"
#include <afxdialogex.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// アプリケーションのバージョン情報に使われる CAboutDlg ダイアログ

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV サポート

// 実装
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMfcD3D10Test1Dlg ダイアログ



CMfcD3D10Test1Dlg::CMfcD3D10Test1Dlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_MFCD3D10TEST1_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_d3dManager = std::make_unique<MyD3DManager>();
}

void CMfcD3D10Test1Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_PICT1, m_ddxcStaticPict1);
}

BEGIN_MESSAGE_MAP(CMfcD3D10Test1Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CMfcD3D10Test1Dlg メッセージ ハンドラー

BOOL CMfcD3D10Test1Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// "バージョン情報..." メニューをシステム メニューに追加します。

	// IDM_ABOUTBOX は、システム コマンドの範囲内になければなりません。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// このダイアログのアイコンを設定します。アプリケーションのメイン ウィンドウがダイアログでない場合、
	//  Framework は、この設定を自動的に行います。
	SetIcon(m_hIcon, TRUE);			// 大きいアイコンの設定
	SetIcon(m_hIcon, FALSE);		// 小さいアイコンの設定

	// TODO: 初期化をここに追加します。

	m_ddxcStaticPict1.SetWindowPos(nullptr, 0, 0, 640, 480, SWP_NOZORDER | SWP_NOMOVE);

	HWND hWnd = m_ddxcStaticPict1.GetSafeHwnd();
	CRect clientRect;
	m_ddxcStaticPict1.GetClientRect(clientRect);
	const UINT width = clientRect.Width();
	const UINT height = clientRect.Height();
	try
	{
		_ASSERTE(m_d3dManager);
		m_d3dManager->Init(hWnd, width, height);
	}
	catch (const MyComException& ex)
	{
		CString strErr;
		strErr.Format(_T("hr = 0x%lx, %s"), ex.Code, ex.Message);
		ATLTRACE(_T("%s\n"), strErr);
		AfxMessageBox(strErr);
		//this->OnCancel();
		this->EndDialog(IDCANCEL);
		return TRUE;
	}

	return TRUE;  // フォーカスをコントロールに設定した場合を除き、TRUE を返します。
}

void CMfcD3D10Test1Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// ダイアログに最小化ボタンを追加する場合、アイコンを描画するための
//  下のコードが必要です。ドキュメント/ビュー モデルを使う MFC アプリケーションの場合、
//  これは、Framework によって自動的に設定されます。

void CMfcD3D10Test1Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 描画のデバイス コンテキスト

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// クライアントの四角形領域内の中央
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// アイコンの描画
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();

		if (m_d3dManager)
		{
			try
			{
				m_d3dManager->Render();
			}
			catch (const MyComException& ex)
			{
				CString strErr;
				strErr.Format(_T("hr = 0x%lx, %s"), ex.Code, ex.Message);
				ATLTRACE(_T("%s\n"), strErr);
			}
		}
	}
}

// ユーザーが最小化したウィンドウをドラッグしているときに表示するカーソルを取得するために、
//  システムがこの関数を呼び出します。
HCURSOR CMfcD3D10Test1Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CMfcD3D10Test1Dlg::OnDestroy()
{
	m_d3dManager.reset();

	CDialogEx::OnDestroy();

	// TODO: ここにメッセージ ハンドラー コードを追加します。
}
