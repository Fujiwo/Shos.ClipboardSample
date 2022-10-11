#pragma once
//#define SCROLL_VIEW

#include "ClipboardHelper.h"

class View : public
#ifdef SCROLL_VIEW
	CScrollView
#else // SCROLL_VIEW
	CView
#endif // SCROLL_VIEW 
{
protected:
	Document& GetDocument() const
	{
		ASSERT_VALID(m_pDocument);
		return reinterpret_cast<Document&>(*m_pDocument);
	}

#ifdef SCROLL_VIEW
	virtual void View::OnInitialUpdate() override
	{
		CScrollView::OnInitialUpdate();
		SetScrollSizes(MM_TEXT, GetDocument().GetSize());
	}
#endif // SCROLL_VIEW 

#ifndef SCROLL_VIEW
	virtual void OnPrepareDC(CDC* dc, CPrintInfo* pInfo = nullptr) override
	{
		CView::OnPrepareDC(dc, pInfo);

		dc->SetMapMode(MM_ISOTROPIC);

		auto documentArea = GetDocument().GetArea();
		dc->SetWindowOrg(documentArea.CenterPoint());
		dc->SetWindowExt(documentArea.Size());

		CRect clientRect;
		GetClientRect(&clientRect);
		dc->SetViewportOrg(clientRect.CenterPoint());
		dc->SetViewportExt(clientRect.Size());
	}
#endif // SCROLL_VIEW

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

	afx_msg void OnMouseMove(UINT nFlags, CPoint point)
	{
		GetDocument().OnMouseMove(nFlags, DPtoLP(point));
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

	CPoint DPtoLP(CPoint point)
	{
		CClientDC dc(this);
		OnPrepareDC(&dc);
		dc.DPtoLP(&point);
		return point;
	}
	
	DECLARE_DYNCREATE(View)
	DECLARE_MESSAGE_MAP()
};
