#pragma once

#include <afx.h>
#include "GdiObjectSelector.h"

// Flicker-free drawing for a view, using an off-screen bitmap (the cache).
// Drawing is split into two layers:
//   - Layer 1: contents that change rarely (e.g. figures). Drawn into the cache, and redrawn
//              only when (a part of) the cache has been invalidated by Update.
//   - Layer 2: contents that change often (e.g. a cursor). Drawn directly over the cache on every paint.
class DoubleBuffer
{
    // false when the whole cache must be redrawn (it is also recreated to fit the client area).
    bool     isCacheValid    = false;
    CRect    invalidCacheArea; // the part of the cache to be redrawn (device coordinates)
    CBitmap  cache;
    COLORREF backgroundColor = ::GetSysColor(COLOR_WINDOW);
    CView&   view;

public:
    explicit DoubleBuffer(CView& view) : view(view)
    {}

    DoubleBuffer(const DoubleBuffer&) = delete;
    DoubleBuffer& operator=(const DoubleBuffer&) = delete;

    // Returns the view, checking that its window exists.
    CView& Window()
    {
        ASSERT_VALID(&view);
        ASSERT(view.GetSafeHwnd() != nullptr && ::IsWindow(view.GetSafeHwnd()));
        return view;
    }

protected:
    // Sets the color used to clear the cache before drawing layer 1.
    void SetBackgroundColor(COLORREF backgroundColor)
    {
        this->backgroundColor = backgroundColor;
    }

    // Invalidates the whole cache.
    void Update()
    {
        isCacheValid = false;
    }

    // Invalidates a part of the cache (in device coordinates).
    void Update(const CRect& deviceArea)
    {
        invalidCacheArea.UnionRect(invalidCacheArea, deviceArea);
    }

    // Paints the view: copies the cache (layer 1) to the DC, then draws layer 2 over it in logical coordinates.
    void Draw(CDC& dc)
    {
        DrawCache(dc);
        view.OnPrepareDC(&dc);
        OnDrawLayer2(dc);
    }

    // Overridden to draw each layer. The DC has already been prepared by the view's OnPrepareDC.
    virtual void OnDrawLayer1(CDC& /* dc */) {}
    virtual void OnDrawLayer2(CDC& /* dc */) {}

private:
    // Brings the cache up to date and copies it to the DC.
    void DrawCache(CDC& dc)
    {
        CRect clientRect;
        view.GetClientRect(&clientRect);
        if (clientRect.IsRectEmpty())
            return;

        CDC   memoryDC;
        memoryDC.CreateCompatibleDC(&dc);

        if (!isCacheValid) {
            // (Re)create the bitmap so that its size follows the client area.
            cache.DeleteObject();
            if (!cache.CreateCompatibleBitmap(&dc, clientRect.Width(), clientRect.Height()))
                return;
        }

        GdiObjectSelector selector(memoryDC, cache);

        // Redraw the whole cache, or only the invalidated part of it.
        if (!isCacheValid) {
            DrawToCache(memoryDC, clientRect);
            isCacheValid = true;
            invalidCacheArea.SetRectEmpty();
        } else if (!invalidCacheArea.IsRectEmpty()) {
            CRect area;
            if (area.IntersectRect(invalidCacheArea, clientRect))
                DrawToCache(memoryDC, area);
            invalidCacheArea.SetRectEmpty();
        }

        // Copy only the area that needs painting.
        CRect clipBox;
        dc.GetClipBox(&clipBox);
        dc.BitBlt(clipBox.left, clipBox.top, clipBox.Width(), clipBox.Height(), &memoryDC, clipBox.left, clipBox.top, SRCCOPY);
    }

    // Redraws layer 1 within the given area of the cache.
    // The clipping region limits drawing to the area, and SaveDC/RestoreDC undoes the mapping set by OnPrepareDC.
    void DrawToCache(CDC& memoryDC, const CRect& deviceArea)
    {
        CRgn clipRegion;
        clipRegion.CreateRectRgnIndirect(&deviceArea);
        memoryDC.SelectClipRgn(&clipRegion);

        memoryDC.FillSolidRect(&deviceArea, backgroundColor);
        const auto savedDC = memoryDC.SaveDC();
        view.OnPrepareDC(&memoryDC);
        OnDrawLayer1(memoryDC);
        memoryDC.RestoreDC(savedDC);

        memoryDC.SelectClipRgn(nullptr);
    }
};

// Mixes DoubleBuffer into an MFC view class (CView or CScrollView) and handles the messages related to painting.
template <class TView>
class DoubleBufferViewBase : public TView, protected DoubleBuffer
{
public:
    DoubleBufferViewBase() : DoubleBuffer(static_cast<TView&>(*this))
    {}

protected:
    // Called from OnPaint. Can be overridden to customize painting.
    virtual void Paint(CDC& dc)
    {
        Draw(dc);
    }

    // Drawing is done in OnPaint via the double buffer, so OnDraw is never called.
    // (Printing and print preview are not supported.)
    virtual void OnDraw(CDC* /* pDC */) override
    {
        ASSERT(false);
    }

    // The document has changed: redraw the whole cache. (A derived view may invalidate only a part instead.)
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override
    {
        Update();
        TView::OnUpdate(pSender, lHint, pHint);
    }

    afx_msg void OnPaint()
    {
        CPaintDC dc(this);
        Paint(dc);
    }

    // The whole client area is painted from the cache, so erasing the background is unnecessary (it would cause flicker).
    afx_msg BOOL OnEraseBkgnd(CDC* /* pDC */)
    {
        return TRUE;
    }

    // The cache must be recreated to fit the new size.
    afx_msg void OnSize(UINT nType, int cx, int cy)
    {
        TView::OnSize(nType, cx, cy);
        Update();
    }
};

// Double-buffered CView.
class DoubleBufferView : public DoubleBufferViewBase<CView>
{
    DECLARE_DYNCREATE(DoubleBufferView)
    DECLARE_MESSAGE_MAP()
};

// Double-buffered CScrollView.
// Scrolling changes what the client area shows, so the whole cache is redrawn after scrolling.
class DoubleBufferScrollView : public DoubleBufferViewBase<CScrollView>
{
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
    {
        DoubleBufferViewBase<CScrollView>::OnHScroll(nSBCode, nPos, pScrollBar);
        Update();
        Invalidate();
    }

    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
    {
        DoubleBufferViewBase<CScrollView>::OnVScroll(nSBCode, nPos, pScrollBar);
        Update();
        Invalidate();
    }

    afx_msg BOOL OnMouseWheel(UINT nFlags, short delta, CPoint point)
    {
        const auto result = DoubleBufferViewBase<CScrollView>::OnMouseWheel(nFlags, delta, point);
        Update();
        Invalidate();
        return result;
    }

    DECLARE_DYNCREATE(DoubleBufferScrollView)
    DECLARE_MESSAGE_MAP()
};
