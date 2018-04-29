
// MfcD2DBmpTest1Doc.h : CMfcD2DBmpTest1Doc クラスのインターフェイス
//


#pragma once


class CMfcD2DBmpTest1Doc : public CDocument
{
protected: // シリアル化からのみ作成します。
	CMfcD2DBmpTest1Doc();
	DECLARE_DYNCREATE(CMfcD2DBmpTest1Doc)

// 属性
public:

// 操作
public:

// オーバーライド
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// 実装
public:
	virtual ~CMfcD2DBmpTest1Doc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// 生成された、メッセージ割り当て関数
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// 検索ハンドラーの検索コンテンツを設定するヘルパー関数
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
};
