#pragma once

#include "ClipboardHelper.h"

class View : public CScrollView
{
	DECLARE_DYNCREATE(View)

protected:
	Document& GetDocument() const
	{
		ASSERT_VALID(m_pDocument);
		return reinterpret_cast<Document&>(*m_pDocument);
	}

	virtual void View::OnInitialUpdate() override
	{
		CScrollView::OnInitialUpdate();
		SetScrollSizes(MM_TEXT, GetDocument().GetSize());
	}

	virtual void OnDraw(CDC* pDC) override
	{
		DrawFigures(*pDC, GetDocument());
	}

	afx_msg void OnEditCopy()
	{
		ClipboardHelper::OnEditCopy(GetDocument(), *this, GetDocument().GetSize(), ::GetSysColor(COLOR_WINDOW), [&](CDC& dc) { GetDocument().Draw(dc); });
	}

	afx_msg void OnEditCut()
	{
		ClipboardHelper::OnEditCut(GetDocument(), *this, GetDocument().GetSize(), ::GetSysColor(COLOR_WINDOW), [&](CDC& dc) { GetDocument().Draw(dc); });
	}

	afx_msg void OnEditPaste()
	{
		ClipboardHelper::OnEditPaste(GetDocument(), *this);
	}

	afx_msg void OnDestroyClipboard()
	{
		ClipboardHelper::OnDestroyClipboard();
	}

private:
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
