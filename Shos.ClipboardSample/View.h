#pragma once
// Define SCROLL_VIEW to show the document at actual size with scroll bars (MM_TEXT),
// instead of scaling it to fit the window (MM_ISOTROPIC).
//#define SCROLL_VIEW

#include "DoubleBuffer.h"
#include "ClipboardHelper.h"

#ifdef SCROLL_VIEW
using ViewBase = DoubleBufferScrollView;
#else // SCROLL_VIEW
using ViewBase = DoubleBufferView;
#endif // SCROLL_VIEW

// The view. Draws the document with double buffering, handles the clipboard commands,
// and forwards mouse input to the document in logical coordinates.
class View : public ViewBase
{
	bool isTrackingMouseLeave = false;

public:
	View()
	{
		SetBackgroundColor(GetBackgroundColor());
	}

protected:
	// Returns the document as the application's Document type.
	Document& GetDocument() const
	{
		ASSERT_VALID(m_pDocument);
		return *STATIC_DOWNCAST(Document, m_pDocument);
	}

#ifdef SCROLL_VIEW
	virtual void OnInitialUpdate() override
	{
		ViewBase::OnInitialUpdate();
		// The scroll range is the size of the document.
		SetScrollSizes(MM_TEXT, GetDocument().GetSize());
	}
#endif // SCROLL_VIEW

#ifndef SCROLL_VIEW
	// Scales the drawing area to fit the client area, keeping the aspect ratio (MM_ISOTROPIC),
	// and centers it by mapping the center of the drawing area to the center of the client area.
	virtual void OnPrepareDC(CDC* dc, CPrintInfo* pInfo = nullptr) override
	{
		ViewBase::OnPrepareDC(dc, pInfo);

		dc->SetMapMode(MM_ISOTROPIC);

		const auto documentArea = GetDocument().GetArea();
		dc->SetWindowOrg(documentArea.CenterPoint());
		dc->SetWindowExt(documentArea.Size());

		CRect clientRect;
		GetClientRect(&clientRect);
		dc->SetViewportOrg(clientRect.CenterPoint());
		dc->SetViewportExt(clientRect.Size());
	}
#endif // SCROLL_VIEW

	// Layer 1 (cached): the drawing area and the figures.
	virtual void OnDrawLayer1(CDC& dc) override
	{
		GetDocument().DrawArea(dc);
		DrawFigures(dc, GetDocument());
	}

	// Layer 2 (drawn on every paint): the feedback of the command, such as the cursor and the highlight.
	virtual void OnDrawLayer2(CDC& dc) override
	{
		GetDocument().DrawCommand(dc);
	}

	// When the hint tells which figures changed, redraws only their area; otherwise redraws everything.
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override
	{
		const auto* hint = DYNAMIC_DOWNCAST(Hint, pHint);
		if (hint == nullptr || hint->type == Hint::Type::ViewOnly) {
			ViewBase::OnUpdate(pSender, lHint, pHint);
			return;
		}
		const auto figureArea = FigureHelper::GetArea(hint->figures);
		if (!figureArea) {
			ViewBase::OnUpdate(pSender, lHint, pHint);
			return;
		}

		auto area = *figureArea;
		TRACE(_T("View::OnUpdate: area   (top: %d, left: %d, width: %d, height: %d)\n"), area.top, area.left, area.Width(), area.Height());
		// The cache is invalidated in device coordinates.
		area = LPtoDP(area);
		area.NormalizeRect();
		area.InflateRect(1, 1); // margin for rounding errors in coordinate conversion
		TRACE(_T("View::OnUpdate: clipbox(top: %d, left: %d, width: %d, height: %d)\n"), area.top, area.left, area.Width(), area.Height());

		Update(area);
		InvalidateRect(area);
	}

	// [Edit] menu commands. Copy and Cut work on the whole document.
	afx_msg void OnEditCopy()
	{
		Copy();
	}

	afx_msg void OnEditCut()
	{
		// Remove the figures only when they have been copied.
		if (Copy())
			GetDocument().RemoveAll();
	}

	afx_msg void OnEditPaste()
	{
		ClipboardHelper::Paste(*this, [this](CArchive& ar) { GetDocument().Paste(ar); });
	}

	// Copy and Cut are enabled only when there are figures.
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI)
	{
		pCmdUI->Enable(!GetDocument().IsEmpty());
	}

	// Paste is enabled only when the clipboard holds data of this application.
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
		// Repaint without invalidating the cache, so that only layer 2 (the cursor and the highlight) is redrawn.
		Invalidate();
	}

	afx_msg void OnMouseLeave()
	{
		isTrackingMouseLeave = false;
		GetDocument().OnMouseLeave();
		Invalidate();
	}

private:
	// The background color outside the drawing area (also used for the bitmap copied to the clipboard).
	static COLORREF GetBackgroundColor()
	{
		return ::GetSysColor(COLOR_WINDOW);
	}

	// Copies the whole document to the clipboard. Returns false on failure.
	bool Copy()
	{
		return ClipboardHelper::Copy(GetDocument(), *this, GetDocument().GetSize(), GetBackgroundColor(), [this](CDC& dc) { GetDocument().Draw(dc); });
	}

	// Requests a WM_MOUSELEAVE message. The request ends when the message is sent,
	// so it is made again on the next mouse move.
	void TrackMouseLeave()
	{
		if (isTrackingMouseLeave)
			return;

		TRACKMOUSEEVENT trackMouseEvent{
			.cbSize	   = sizeof(TRACKMOUSEEVENT),
			.dwFlags   = TME_LEAVE,
			.hwndTrack = GetSafeHwnd()
		};
		isTrackingMouseLeave = ::TrackMouseEvent(&trackMouseEvent) != FALSE;
	}

	// Draws only the figures that intersect the clip box, to save time.
	void DrawFigures(CDC& dc, const Document& document)
	{
		CRect clipBox; // GetClipBox returns logical coordinates.
		const auto clippingMode = dc.GetClipBox(clipBox);

		if (clippingMode == SIMPLEREGION || clippingMode == COMPLEXREGION) {
			for (auto* figure : document) {
				ASSERT_VALID(figure);
				if (HasIntersection(*figure, clipBox))
					figure->Draw(dc);
			}
		}
		else {
			ASSERT(false);
		}
	}

	// Returns true when the area of the figure intersects the clip box (both in logical coordinates).
	static bool HasIntersection(const Figure& figure, const CRect& clipBox)
	{
		CRect intersection;
		return intersection.IntersectRect(figure.GetArea(), clipBox) != FALSE;
	}

	// Coordinate conversions between device and logical coordinates, using the same mapping as for drawing.
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
