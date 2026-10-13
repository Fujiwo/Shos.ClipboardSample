#pragma once
//#define SCROLL_VIEW

#include "DoubleBuffer.h"
#include "ClipboardHelper.h"

#ifdef SCROLL_VIEW
using ViewBase = DoubleBufferScrollView;
#else // SCROLL_VIEW
using ViewBase = DoubleBufferView;
#endif // SCROLL_VIEW

class View : public ViewBase
{
	bool isTrackingMouseLeave;

public:
	View() : isTrackingMouseLeave(false)
	{
		SetBackgroundColor(GetBackgroundColor());
	}

protected:
	Document& GetDocument() const
	{
		ASSERT_VALID(m_pDocument);
		return *STATIC_DOWNCAST(Document, m_pDocument);
	}

#ifdef SCROLL_VIEW
	virtual void OnInitialUpdate() override
	{
		ViewBase::OnInitialUpdate();
		SetScrollSizes(MM_TEXT, GetDocument().GetSize());
	}
#endif // SCROLL_VIEW

#ifndef SCROLL_VIEW
	virtual void OnPrepareDC(CDC* dc, CPrintInfo* pInfo = nullptr) override
	{
		ViewBase::OnPrepareDC(dc, pInfo);

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

	virtual void OnDrawLayer1(CDC& dc) override
	{
		GetDocument().DrawArea(dc);
		DrawFigures(dc, GetDocument());
	}

	virtual void OnDrawLayer2(CDC& dc) override
	{
		GetDocument().DrawCommand(dc);
	}

	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override
	{
		auto hint = DYNAMIC_DOWNCAST(Hint, pHint);
		CRect area;
		if (hint == nullptr || hint->type == Hint::Type::ViewOnly || !FigureHelper::GetArea(hint->figures, area)) {
			ViewBase::OnUpdate(pSender, lHint, pHint);
			return;
		}

		TRACE(_T("View::OnUpdate: area   (top: %d, left: %d, width: %d, height: %d)\n"), area.top, area.left, area.Width(), area.Height());
		area = LPtoDP(area);
		area.NormalizeRect();
		area.InflateRect(1, 1); // margin for rounding errors in coordinate conversion
		TRACE(_T("View::OnUpdate: clipbox(top: %d, left: %d, width: %d, height: %d)\n"), area.top, area.left, area.Width(), area.Height());

		Update(area);
		InvalidateRect(area);
	}

	afx_msg void OnEditCopy()
	{
		Copy();
	}

	afx_msg void OnEditCut()
	{
		if (Copy())
			GetDocument().RemoveAll();
	}

	afx_msg void OnEditPaste()
	{
		ClipboardHelper::Paste(*this, [&](CArchive& ar) { GetDocument().Paste(ar); });
	}

	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI)
	{
		pCmdUI->Enable(!GetDocument().IsEmpty());
	}

	afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI)
	{
		pCmdUI->Enable(ClipboardHelper::CanPaste());
	}

	afx_msg void OnLButtonUp(UINT /* nFlags */, CPoint point)
	{
		GetDocument().OnClick(DPtoLP(point));
	}

	afx_msg void OnMouseMove(UINT /* nFlags */, CPoint point)
	{
		TrackMouseLeave();
		GetDocument().OnMouseMove(DPtoLP(point));
		Invalidate();
	}

	afx_msg void OnMouseLeave()
	{
		isTrackingMouseLeave = false;
		GetDocument().OnMouseLeave();
		Invalidate();
	}

private:
	static COLORREF GetBackgroundColor()
	{
		return ::GetSysColor(COLOR_WINDOW);
	}

	bool Copy()
	{
		return ClipboardHelper::Copy(GetDocument(), *this, GetDocument().GetSize(), GetBackgroundColor(), [&](CDC& dc) { GetDocument().Draw(dc); });
	}

	void TrackMouseLeave()
	{
		if (isTrackingMouseLeave)
			return;

		TRACKMOUSEEVENT trackMouseEvent{};
		trackMouseEvent.cbSize	  = sizeof(trackMouseEvent);
		trackMouseEvent.dwFlags	  = TME_LEAVE;
		trackMouseEvent.hwndTrack = GetSafeHwnd();
		isTrackingMouseLeave	  = ::TrackMouseEvent(&trackMouseEvent) != FALSE;
	}

	void DrawFigures(CDC& dc, const Document& document)
	{
		CRect clipBox; // GetClipBox returns logical coordinates.
		auto clippingMode = dc.GetClipBox(clipBox);

		if (clippingMode == SIMPLEREGION || clippingMode == COMPLEXREGION) {
			for (auto figure : document) {
				ASSERT_VALID(figure);
				if (HasIntersection(*figure, clipBox))
					figure->Draw(dc);
			}
		}
		else {
			ASSERT(false);
		}
	}

	bool HasIntersection(const Figure& figure, const CRect& clipBox)
	{
		CRect intersection;
		return intersection.IntersectRect(figure.GetArea(), clipBox) != FALSE;
	}

	CPoint DPtoLP(CPoint point)
	{
		CClientDC dc(this);
		OnPrepareDC(&dc);
		dc.DPtoLP(&point);
		return point;
	}

	CRect LPtoDP(CRect rect)
	{
		CClientDC dc(this);
		OnPrepareDC(&dc);
		dc.LPtoDP(rect);
		return rect;
	}

	DECLARE_DYNCREATE(View)
	DECLARE_MESSAGE_MAP()
};
