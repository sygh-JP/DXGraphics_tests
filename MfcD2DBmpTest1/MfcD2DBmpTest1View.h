
// MfcD2DBmpTest1View.h : CMfcD2DBmpTest1View クラスのインターフェイス
//

#pragma once


class CMfcD2DBmpTest1View : public CView
{
protected: // シリアル化からのみ作成します。
	CMfcD2DBmpTest1View();
	DECLARE_DYNCREATE(CMfcD2DBmpTest1View)

// 属性
public:
	CMfcD2DBmpTest1Doc* GetDocument() const;

// 操作
public:

// オーバーライド
public:
	virtual void OnDraw(CDC* pDC);  // このビューを描画するためにオーバーライドされます。
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// 実装
public:
	virtual ~CMfcD2DBmpTest1View();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

private:
	CDCRenderTarget m_dcRenderTarget;

// 生成された、メッセージ割り当て関数
protected:
	afx_msg LRESULT OnDraw2D(WPARAM /*wParam*/, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG
inline CMfcD2DBmpTest1Doc* CMfcD2DBmpTest1View::GetDocument() const
   { return reinterpret_cast<CMfcD2DBmpTest1Doc*>(m_pDocument); }
#endif

