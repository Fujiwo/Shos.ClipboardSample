#pragma once

#include "ClipboardHelper.h"

class View : public CView
{
	DECLARE_DYNCREATE(View)

public:
	Document* GetDocument() const
	{ return reinterpret_cast<Document*>(m_pDocument); }

	virtual void OnDraw(CDC* pDC) override
	{
		auto document = GetDocument();
		ASSERT_VALID(document);
		if (document == nullptr)
			return;

		document->Draw(*pDC);
	}
	

	afx_msg void OnEditCopy()
	{
		auto document = GetDocument();
		ASSERT_VALID(document);
		if (document == nullptr)
			return;

		ClipboardHelper::OnEditCopy(*document, *this, document->GetSize(), [&](CDC& dc) { document->Draw(dc); });
	}

	afx_msg void OnEditPaste()
	{
		auto document = GetDocument();
		ASSERT_VALID(document);
		if (document == nullptr)
			return;

		ClipboardHelper::OnEditPaste(*document, *this);
	}

	afx_msg void OnDestroyClipboard()
	{
		ClipboardHelper::OnDestroyClipboard();
	}
	
	DECLARE_MESSAGE_MAP()
};
