#pragma once

#include "ClipboardHelper.h"

class View : public CScrollView
{
	DECLARE_DYNCREATE(View)

protected:
	Document* GetDocument() const
	{ return reinterpret_cast<Document*>(m_pDocument); }

	virtual void View::OnInitialUpdate() override
	{
		CScrollView::OnInitialUpdate();
		SetScrollSizes(MM_TEXT, GetDocument()->GetSize());
	}

	virtual void OnDraw(CDC* pDC) override
	{
		auto document = GetDocument();
		ASSERT_VALID(document);
		if (document == nullptr)
			return;

		DrawDocument(*pDC, *document);
	}

	//afx_msg BOOL OnEraseBkgnd(CDC* pDC)
	//{
	//	CRect clientRect;
	//	GetClientRect(&clientRect);
	//	pDC->FillSolidRect(&clientRect, ::GetSysColor(COLOR_3DFACE));
	//	return TRUE;

	//	//return CScrollView::OnEraseBkgnd(pDC);
	//}

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

private:
	void DrawDocument(CDC& dc, const Document& document)
	{
		//document.DrawArea(dc, GetSysColor(COLOR_WINDOW));
		DrawFigures(dc, document);
	}

	void DrawFigures(CDC& dc, const Document& document)
	{
		CRect clipBox;
		dc.GetClipBox(&clipBox);
		dc.DPtoLP(&clipBox);
		
		for (auto figure : document) {
			ASSERT_VALID(figure);
			if (HasIntersection(*figure, clipBox))
				continue;
			figure->Draw(dc);
		}
	}

	bool HasIntersection(const Figure& figure, const CRect& clipBox)
	{
		CRect intersection;
		return intersection.IntersectRect(&figure.GetArea(), &clipBox);
	}
	
	DECLARE_MESSAGE_MAP()
};
