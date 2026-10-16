#pragma once

#include <afx.h>

// RAII helpers that select a GDI object into a DC, and select the previous object back on destruction.
// Usage: GdiObjectSelector penSelector(dc, pen); // pen is selected until the end of the scope
class GdiObjectSelectorBase
{
	CDC&			  dc;
	CGdiObject* const oldGdiObject;

public:
	GdiObjectSelectorBase(CDC& dc, CGdiObject* oldGdiObject) : dc(dc), oldGdiObject(oldGdiObject)
	{}

	// Restores the object that was selected before.
	virtual ~GdiObjectSelectorBase()
	{
		dc.SelectObject(oldGdiObject);
	}

	GdiObjectSelectorBase(const GdiObjectSelectorBase&) = delete;
	GdiObjectSelectorBase& operator=(const GdiObjectSelectorBase&) = delete;
};

// Selects a stock object (e.g. NULL_BRUSH).
class StockObjectSelector : public GdiObjectSelectorBase
{
public:
	StockObjectSelector(CDC& dc, int stockObjectIndex) : GdiObjectSelectorBase(dc, dc.SelectStockObject(stockObjectIndex))
	{}
};

// Selects the given GDI object (a pen, a brush, a bitmap, ...). The object must outlive the selector.
class GdiObjectSelector : public GdiObjectSelectorBase
{
public:
	GdiObjectSelector(CDC& dc, CGdiObject& gdiObject) : GdiObjectSelectorBase(dc, dc.SelectObject(&gdiObject))
	{}
};
