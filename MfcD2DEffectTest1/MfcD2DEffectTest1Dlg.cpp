
// MfcD2DEffectTest1Dlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "MfcD2DEffectTest1.h"
#include "MfcD2DEffectTest1Dlg.h"
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
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

// 実装
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMfcD2DEffectTest1Dlg ダイアログ



CMfcD2DEffectTest1Dlg::CMfcD2DEffectTest1Dlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMfcD2DEffectTest1Dlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_lightningGen = std::make_unique<MyLightningGenerator>();
}

void CMfcD2DEffectTest1Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PICTURE1, m_ddxcPicture1);
	DDX_Control(pDX, IDC_CHECK_BLOOM, m_ddxcCheckBloom);
}

BEGIN_MESSAGE_MAP(CMfcD2DEffectTest1Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_UPDATE, &CMfcD2DEffectTest1Dlg::OnBnClickedButtonUpdate)
	ON_BN_CLICKED(IDC_CHECK_BLOOM, &CMfcD2DEffectTest1Dlg::OnBnClickedCheckBloom)
END_MESSAGE_MAP()


bool CMfcD2DEffectTest1Dlg::InitDirectX(CWnd& target)
{
	CRect winRect(0,0,0,0);
	target.GetWindowRect(winRect);

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferDesc.Width = winRect.Width();
	swapChainDesc.BufferDesc.Height = winRect.Height();
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_CENTERED;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.OutputWindow = target.GetSafeHwnd();
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	auto featureLevel = D3D_FEATURE_LEVEL();

#ifdef _DEBUG
	const UINT createFlags = D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#else
	const UINT createFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#endif

	HRESULT hr = ::D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
		createFlags,
		nullptr, 0, D3D11_SDK_VERSION,
		&swapChainDesc, m_dxgiSwapChain.GetAddressOf(),
		m_d3dDevice.GetAddressOf(), &featureLevel, m_d3dDeviceContext.GetAddressOf());
	if (FAILED(hr))
	{
		AfxMessageBox(_T("Failed to initialize Direct3D!!"));
		return false;
	}

	ComPtr<IDXGIDevice1> dxgiDevice;
	hr = m_d3dDevice.As(&dxgiDevice);
	if (FAILED(hr) || !dxgiDevice)
	{
		AfxMessageBox(_T("Failed to get DXGI device!!"));
		return false;
	}

	// Direct2D 1.1 ファクトリ。
	D2D1_FACTORY_OPTIONS d2dOptions = {};
#ifdef _DEBUG
	d2dOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
	hr = D2D1CreateFactory<ID2D1Factory1>(
		D2D1_FACTORY_TYPE_SINGLE_THREADED,
		d2dOptions,
		m_d2dFactory.GetAddressOf());
	if (FAILED(hr) || !m_d2dFactory)
	{
		AfxMessageBox(_T("Failed to create D2D factory!!"));
		return false;
	}

	// ID2D1DeviceContext は Direct2D 1.0 以前より存在する ID2D1RenderTarget を継承しているので、
	// 基本的な描画機能はそのままに新たな機能が追加された形になっているが、インターフェイスの作成方法が異なる。
	// Direct2D 1.0 において下記の方法で比較的簡単に作成できていたインターフェイス経由では Direct2D 1.1 の新しい機能は使えない模様。
	// ・ID2D1Factory::CreateDxgiSurfaceRenderTarget() で作成できる ID2D1RenderTarget
	// ・ID2D1Factory::CreateDCRenderTarget() で作成できる ID2D1DCRenderTarget
	// ・ID2D1Factory::CreateHwndRenderTarget() で作成できる ID2D1HwndRenderTarget
	// 特に DC と Hwnd は Direct3D 10.1 や DXGI を意識することなく気軽に Win32/MFC アプリから使えるのがメリットだったが、
	// Direct2D 1.1 以降（の新しい機能を使う場合）は必ず Direct3D 11.x と DXGI の明示的初期化が必要になる模様。

	hr = m_d2dFactory->CreateDevice(dxgiDevice.Get(), m_d2dDevice.GetAddressOf());
	if (FAILED(hr) || !m_d2dDevice)
	{
		AfxMessageBox(_T("Failed to create D2D device!!"));
		return false;
	}

	hr = m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, m_d2dDeviceContext.GetAddressOf());
	if (FAILED(hr) || !m_d2dDeviceContext)
	{
		AfxMessageBox(_T("Failed to create D2D device context!!"));
		return false;
	}

	hr = m_d2dDeviceContext->CreateSolidColorBrush(
		//D2D1::ColorF(D2D1::ColorF::DodgerBlue),
		//D2D1::ColorF(D2D1::ColorF::HotPink),
		D2D1::ColorF(D2D1::ColorF::Violet),
		m_d2dLightningBrush.GetAddressOf()
		);
	if (FAILED(hr) || !m_d2dLightningBrush)
	{
		AfxMessageBox(_T("Failed to create D2D brush!!"));
		return false;
	}

	const D2D1_BITMAP_PROPERTIES1 backBufferBitmapProps =
		D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, // RTV として使うが、SRV としては使わない。
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
		);

	Microsoft::WRL::ComPtr<IDXGISurface> dxgiBackBufferSurface;
	hr = m_dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(dxgiBackBufferSurface.GetAddressOf()));
	if (FAILED(hr))
	{
		return false;
	}

	// DXGI バックバッファのサーフェイスから、
	// Direct2D ビットマップのインターフェイスを作成する。
	// Direct3D テクスチャのインターフェイスを作成するときと同じように、
	// ピクセル リソースは共有される。
	hr = m_d2dDeviceContext->CreateBitmapFromDxgiSurface(
		dxgiBackBufferSurface.Get(),
		&backBufferBitmapProps,
		m_d2dBackBufferBitmap.ReleaseAndGetAddressOf()
		);
	if (FAILED(hr))
	{
		return false;
	}

	const D2D1_BITMAP_PROPERTIES1 workBitmapProps =
		D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET, // RTV, SRV として使う。
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
		);

	const auto backBufferSize = m_d2dBackBufferBitmap->GetPixelSize();

	// バックバッファと同じサイズの作業用ビットマップを作成する。
	const uint32_t strideSizeInBytes = backBufferSize.width * 4;
	hr = m_d2dDeviceContext->CreateBitmap(backBufferSize, nullptr, strideSizeInBytes, &workBitmapProps, m_d2dWorkBitmap.ReleaseAndGetAddressOf());
	if (FAILED(hr))
	{
		return false;
	}

	// Direct2D 1.1 で追加されたエフェクトに関しては下記を参考に。
	// HLSL で書いたシェーダーを使ってカスタム エフェクトを作ることもできるらしい。
	// http://msdn.microsoft.com/en-us/library/windows/desktop/hh973240.aspx

	hr = m_d2dDeviceContext->CreateEffect(CLSID_D2D1GaussianBlur, &m_d2dGaussianBlurEffect);
	if (FAILED(hr))
	{
		return false;
	}

	return SUCCEEDED(hr);
}

void CMfcD2DEffectTest1Dlg::DrawByDirectX()
{
	// まず作業用オフスクリーン ビットマップにレンダリング。
	m_d2dDeviceContext->SetTarget(m_d2dWorkBitmap.Get());

	m_d2dDeviceContext->BeginDraw();
	m_d2dDeviceContext->Clear(D2D1::ColorF(0, 0, 0, 0));

	const auto& verticesArraysList = m_lightningGen->GetVerticesArraysList();
	const auto& branchExistenceList = m_lightningGen->GetBranchExistenceList();
	for (size_t a = 0; a < verticesArraysList.size(); ++a)
	{
		if (branchExistenceList[a])
		{
			const auto& verticesArray = verticesArraysList[a];
			auto penSize = (a == 0) ? 2.0f : 1.0f;
			for (size_t i = 0; i < verticesArray.size() - 1; ++i)
			{
				const auto vStart = verticesArray[i];
				const auto vEnd = verticesArray[i + 1];
				m_d2dDeviceContext->DrawLine(vStart.ToPoint2F(), vEnd.ToPoint2F(), m_d2dLightningBrush.Get(), penSize);
			}
		}
	}

	m_d2dDeviceContext->EndDraw();

	// DXGI バックバッファにレンダリング。
	m_d2dDeviceContext->SetTarget(m_d2dBackBufferBitmap.Get());

	m_d2dDeviceContext->BeginDraw();

	// 背景色は暗めのほうが、ブルームの効果がわかりやすい。

	//m_d2dDeviceContext->Clear(D2D1::ColorF(D2D1::ColorF::CornflowerBlue));
	//m_d2dDeviceContext->Clear(D2D1::ColorF(D2D1::ColorF::WhiteSmoke));
	m_d2dDeviceContext->Clear(D2D1::ColorF(0.1f, 0.1f, 0.1f));

	// いったんビットマップにレンダリングしておいたものを普通に描画。
	m_d2dDeviceContext->DrawImage(m_d2dWorkBitmap.Get());

	if (m_ddxcCheckBloom.GetCheck())
	{
		// ガウスぼかしエフェクトを適用したものを加算合成。
		m_d2dGaussianBlurEffect->SetInput(0, m_d2dWorkBitmap.Get(), true);
		m_d2dGaussianBlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, 6.0f);
		m_d2dDeviceContext->DrawImage(m_d2dGaussianBlurEffect.Get(), D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR, D2D1_COMPOSITE_MODE_PLUS);
		// SRV スロットをリセット。
		m_d2dGaussianBlurEffect->SetInput(0, nullptr, true);
	}

	m_d2dDeviceContext->EndDraw();

	// RTV スロットをリセット。
	m_d2dDeviceContext->SetTarget(nullptr);

	// DXGI スワップチェーンをフリップする。
	m_dxgiSwapChain->Present(0, 0);
}

// CMfcD2DEffectTest1Dlg メッセージ ハンドラー

BOOL CMfcD2DEffectTest1Dlg::OnInitDialog()
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

	m_ddxcPicture1.SetWindowPos(nullptr, 0, 0, 700, 500, SWP_NOMOVE | SWP_NOZORDER);
	m_ddxcCheckBloom.SetCheck(BST_CHECKED);

	if (!this->InitDirectX(m_ddxcPicture1))
	{
		this->SendMessage(WM_CLOSE);
	}

	return TRUE;  // フォーカスをコントロールに設定した場合を除き、TRUE を返します。
}

void CMfcD2DEffectTest1Dlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CMfcD2DEffectTest1Dlg::OnPaint()
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

		this->DrawByDirectX();
	}
}

// ユーザーが最小化したウィンドウをドラッグしているときに表示するカーソルを取得するために、
//  システムがこの関数を呼び出します。
HCURSOR CMfcD2DEffectTest1Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CMfcD2DEffectTest1Dlg::OnBnClickedButtonUpdate()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	m_lightningGen->Update();

	this->InvalidatePicture1();
}


void CMfcD2DEffectTest1Dlg::OnBnClickedCheckBloom()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	this->InvalidatePicture1();
}
