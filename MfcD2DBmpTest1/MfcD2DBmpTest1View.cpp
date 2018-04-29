
// MfcD2DBmpTest1View.cpp : CMfcD2DBmpTest1View クラスの実装
//

#include "stdafx.h"
// SHARED_HANDLERS は、プレビュー、縮小版、および検索フィルター ハンドラーを実装している ATL プロジェクトで定義でき、
// そのプロジェクトとのドキュメント コードの共有を可能にします。
#ifndef SHARED_HANDLERS
#include "MfcD2DBmpTest1.h"
#endif

#include "MfcD2DBmpTest1Doc.h"
#include "MfcD2DBmpTest1View.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//#define USE_D2D_DC_INTEROP

// CMfcD2DBmpTest1View

IMPLEMENT_DYNCREATE(CMfcD2DBmpTest1View, CView)

BEGIN_MESSAGE_MAP(CMfcD2DBmpTest1View, CView)
	// 標準印刷コマンド
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
#ifndef USE_D2D_DC_INTEROP
	ON_REGISTERED_MESSAGE(AFX_WM_DRAW2D, &CMfcD2DBmpTest1View::OnDraw2D)
#endif
END_MESSAGE_MAP()

namespace
{
	void RenderBitmap(CRenderTarget& rt, CD2DBitmap& bmp, CD2DRectF rectDest, float x, float y)
	{
		using namespace D2D1;

		ATLASSERT(bmp.IsValid());
#if 1
		rt.SetTransform(Matrix3x2F::Translation(x, y));
#else
		rectDest.left = x;
		rectDest.top = y;
		rectDest.right += x;
		rectDest.bottom += y;
#endif
		rt.DrawBitmap(&bmp, rectDest);
	};

	void CreateAndRenderBitmap(CRenderTarget& rt, CD2DBitmap& bmp, CD2DRectF rectDest, float x, float y)
	{
		const HRESULT hr = bmp.Create(&rt);
		if (FAILED(hr))
		{
			ATLTRACE("Failed to create D2D bitmap!! (0x%08lX)\n", hr);
		}
		else
		{
			RenderBitmap(rt, bmp, rectDest, x, y);
		}
	};

	void RenderObjects(CRenderTarget& rt)
	{
		using namespace D2D1;

		//rt.Clear(ColorF(ColorF::White));
		rt.Clear(ColorF(ColorF::LightGray));

		CD2DRectF rc(0, 0, 48, 48); // ビットマップのサイズ。

		// 通常は描画フレームごとにリソースの作成をするようなことはしないが、簡潔な例示のため。
		// D2D レンダーターゲット、D2D ビットマップ、D2D ブラシなどはデバイス依存のリソースオブジェクトで、
		// GPU 側にリソースを確保する関係上 GDI/GDI+ のペンやブラシと比べて生成負荷が高いため、
		// 実際は描画のたびに生成せずに事前生成しておくと効率が良くなる。詳細はチュートリアルを参照。

		float y = 0;
		// カスタム リソースからの読み込み。
		{
			// BMP の場合、32bit BGRA であっても透過はできない模様。
			CD2DBitmap bmp1(&rt, ID_MY_TEST_IMAGE, _T("MYRESTYPE_BITMAP"));
			CreateAndRenderBitmap(rt, bmp1, rc, 0, y);
			y += 48;
			// PNG の透過は可能。
			CD2DBitmap bmp2(&rt, ID_MY_TEST_IMAGE, _T("MYRESTYPE_PNG"));
			CreateAndRenderBitmap(rt, bmp2, rc, 0, y);
			y += 48;
			// 96 DPI 環境かつ .ico ファイルが 48x48 の画像を含んでいてもぼやける。
			CD2DBitmap bmp3(&rt, ID_MY_TEST_IMAGE, _T("MYRESTYPE_ICON"));
			CreateAndRenderBitmap(rt, bmp3, rc, 0, y);
			y += 48;
		}

		// HBITMAP を経由すれば、BITMAP タイプ (RT_BITMAP) で登録した 32bit BGRA BMP の透過が可能。
		{
			CBitmap mfcBmp;
			const BOOL isLoaded = mfcBmp.LoadBitmap(ID_MY_TEST_IMAGE);
			BITMAP bmpInfo = {};
			mfcBmp.GetBitmap(&bmpInfo);
			CD2DBitmap d2dBmp(&rt, static_cast<HBITMAP>(mfcBmp));
			RenderBitmap(rt, d2dBmp, rc, 0, y);
		}
	}
}

// CMfcD2DBmpTest1View コンストラクション/デストラクション

CMfcD2DBmpTest1View::CMfcD2DBmpTest1View()
{
	// TODO: 構築コードをここに追加します。

	this->EnableD2DSupport();

#ifdef USE_D2D_DC_INTEROP
	const auto props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
		0,
		0,
		D2D1_RENDER_TARGET_USAGE_NONE,
		D2D1_FEATURE_LEVEL_DEFAULT
	);

	ATLVERIFY(m_dcRenderTarget.Create(props));
#endif
}

CMfcD2DBmpTest1View::~CMfcD2DBmpTest1View()
{
}

BOOL CMfcD2DBmpTest1View::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: この位置で CREATESTRUCT cs を修正して Window クラスまたはスタイルを
	//  修正してください。

	return CView::PreCreateWindow(cs);
}

// CMfcD2DBmpTest1View 描画

void CMfcD2DBmpTest1View::OnDraw(CDC* pDC)
{
	CMfcD2DBmpTest1Doc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: この場所にネイティブ データ用の描画コードを追加します。

	// GDI デバイス コンテキストを経由するので、そのまま印刷機能が使える模様。
#ifdef USE_D2D_DC_INTEROP
	CRect clientRect;
	this->GetClientRect(&clientRect);

	if (m_dcRenderTarget.BindDC(*pDC, clientRect))
	{
		m_dcRenderTarget.BeginDraw();

		RenderObjects(m_dcRenderTarget);

		const HRESULT hr = m_dcRenderTarget.EndDraw();

		if (FAILED(hr))
		{
			ATLTRACE("Failed to present D2D!! (0x%08lX)\n", hr);
			if (hr == D2DERR_RECREATE_TARGET)
			{
				// TODO: デバイス依存リソースの再作成が必要。
			}
		}
	}
	else
	{
		ATLASSERT(false);
	}
#endif
}


// CMfcD2DBmpTest1View 印刷

BOOL CMfcD2DBmpTest1View::OnPreparePrinting(CPrintInfo* pInfo)
{
	// 既定の印刷準備
	return DoPreparePrinting(pInfo);
}

void CMfcD2DBmpTest1View::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: 印刷前の特別な初期化処理を追加してください。
}

void CMfcD2DBmpTest1View::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: 印刷後の後処理を追加してください。
}


// CMfcD2DBmpTest1View 診断

#ifdef _DEBUG
void CMfcD2DBmpTest1View::AssertValid() const
{
	CView::AssertValid();
}

void CMfcD2DBmpTest1View::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CMfcD2DBmpTest1Doc* CMfcD2DBmpTest1View::GetDocument() const
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMfcD2DBmpTest1Doc)));
	return (CMfcD2DBmpTest1Doc*)m_pDocument;
}
#endif


// CMfcD2DBmpTest1View メッセージ ハンドラー

LRESULT CMfcD2DBmpTest1View::OnDraw2D(WPARAM /*wParam*/, LPARAM lParam)
{
	// CDCRenderTarget ではない。
	CHwndRenderTarget* pRenderTarget = reinterpret_cast<CHwndRenderTarget*>(lParam);
	ASSERT_VALID(pRenderTarget);

	CHwndRenderTarget& rt = *pRenderTarget;

	RenderObjects(rt);

	// AFX_WM_DRAW2D を使う方法の場合、レンダーターゲットの生成や BeginDraw()/EndDraw() が不要になる。
	// ただし、設定したトランスフォーム行列などのステートが保存されて次回のレンダリングにも使われることに注意が必要。

	return TRUE;
}
