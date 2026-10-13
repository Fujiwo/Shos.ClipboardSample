#pragma once

#include <afx.h>
#include "GdiObjectSelector.h"

class DoubleBuffer
{
    bool     isCacheValid    = false;
    CRect    invalidCacheArea; // device coordinates
    CBitmap  cache;
    COLORREF backgroundColor = ::GetSysColor(COLOR_WINDOW);
    CView&   view;

public:
    explicit DoubleBuffer(CView& view) : view(view)
    {}

    DoubleBuffer(const DoubleBuffer&) = delete;
    DoubleBuffer& operator=(const DoubleBuffer&) = delete;

    CView& Window()
    {
        ASSERT_VALID(&view);
        ASSERT(view.GetSafeHwnd() != nullptr && ::IsWindow(view.GetSafeHwnd()));
        return view;
    }

protected:
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

    void Draw(CDC& dc)
    {
        DrawCache(dc);
        view.OnPrepareDC(&dc);
        OnDrawLayer2(dc);
    }

    virtual void OnDrawLayer1(CDC& /* dc */) {}
    virtual void OnDrawLayer2(CDC& /* dc */) {}

private:
    void DrawCache(CDC& dc)
    {
        CRect clientRect;
        view.GetClientRect(&clientRect);
        if (clientRect.IsRectEmpty())
            return;

        CDC   memoryDC;
        memoryDC.CreateCompatibleDC(&dc);

        if (!isCacheValid) {
            cache.DeleteObject();
            if (!cache.CreateCompatibleBitmap(&dc, clientRect.Width(), clientRect.Height()))
                return;
        }

        GdiObjectSelector selector(memoryDC, cache);

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

        CRect clipBox;
        dc.GetClipBox(&clipBox);
        dc.BitBlt(clipBox.left, clipBox.top, clipBox.Width(), clipBox.Height(), &memoryDC, clipBox.left, clipBox.top, SRCCOPY);
    }

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

template <class TView>
class DoubleBufferViewBase : public TView, protected DoubleBuffer
{
public:
    DoubleBufferViewBase() : DoubleBuffer(static_cast<TView&>(*this))
    {}

protected:
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

    afx_msg BOOL OnEraseBkgnd(CDC* /* pDC */)
    {
        return TRUE;
    }

    afx_msg void OnSize(UINT nType, int cx, int cy)
    {
        TView::OnSize(nType, cx, cy);
        Update();
    }
};

class DoubleBufferView : public DoubleBufferViewBase<CView>
{
    DECLARE_DYNCREATE(DoubleBufferView)
    DECLARE_MESSAGE_MAP()
};

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
