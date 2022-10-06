#pragma once
#include <afx.h>
#include <random>

class GdiObjectSelectorBase
{
	CDC& dc;
	CGdiObject* const oldGdiObject;

public:
	GdiObjectSelectorBase(CDC& dc, CGdiObject* oldGdiObject) : dc(dc), oldGdiObject(oldGdiObject)
	{}

	virtual ~GdiObjectSelectorBase()
	{
		dc.SelectObject(oldGdiObject);
	}
};

class StockObjectSelector : public GdiObjectSelectorBase
{
public:
	StockObjectSelector(CDC& dc, int stockObjectIndex) : GdiObjectSelectorBase(dc, dc.SelectStockObject(stockObjectIndex))
	{}
};

class GdiObjectSelector : public GdiObjectSelectorBase
{
public:
	GdiObjectSelector(CDC& dc, CGdiObject& gdiObject) : GdiObjectSelectorBase(dc, dc.SelectObject(&gdiObject))
	{}
};

struct FigureAttribute
{
	COLORREF color;
	int      penWidth;

	void Serialize(CArchive& ar)
	{
		if (ar.IsStoring())
			ar << color << penWidth;
		else
			ar >> color >> penWidth;
	}
};

class Figure : public CObject
{
	DECLARE_SERIAL(Figure)

	FigureAttribute attribute;

public:
	void Draw(CDC& dc)
	{
		StockObjectSelector stockObjectSelector(dc, NULL_BRUSH);
		CPen pen(PS_SOLID, attribute.penWidth, attribute.color);
		GdiObjectSelector gdiObjectSelector(dc, pen);

		DrawShape(dc);
	}

	virtual void Serialize(CArchive& ar) override
	{
		CObject::Serialize(ar);
		attribute.Serialize(ar);
	}

protected:
	virtual void DrawShape(CDC& dc)
	{}
};

class DotFigure : public Figure
{
	DECLARE_SERIAL(DotFigure)

	const LONG radius = 10L;
	CPoint position;
		
public:
	DotFigure()
	{}

	DotFigure(const CPoint& position) : position(position)
	{}

	virtual void Serialize(CArchive& ar) override
	{
		Figure::Serialize(ar);

		if (ar.IsStoring())
			ar << position;
		else
			ar >> position;
	}

protected:
	virtual void DrawShape(CDC& dc)
	{
		const CSize size(radius, radius);
		dc.Ellipse(CRect(position - size, position + size));
	}
};

class LineFigure : public Figure
{
	DECLARE_SERIAL(LineFigure)

	CPoint start, end;

public:
	LineFigure()
	{}

	LineFigure(CPoint start, CPoint end) : start(start), end(end)
	{}

	virtual void Serialize(CArchive& ar) override
	{
		Figure::Serialize(ar);

		if (ar.IsStoring())
			ar << start << end;
		else
			ar >> start >> end;
	}

protected:
	virtual void DrawShape(CDC& dc)
	{
		dc.MoveTo(start);
		dc.LineTo(end);
	}
};

class RectangleFigure : public Figure
{
	DECLARE_SERIAL(RectangleFigure)

	CRect position;

public:
	RectangleFigure()
	{}

	RectangleFigure(const CRect& position) : position(position)
	{}

	virtual void Serialize(CArchive& ar) override
	{
		Figure::Serialize(ar);

		if (ar.IsStoring())
			ar << position;
		else
			ar >> position;
	}

protected:
	virtual void DrawShape(CDC& dc)
	{
		dc.Rectangle(&position);
	}
};

class EllipseFigure : public Figure
{
	DECLARE_SERIAL(EllipseFigure)
	
	CRect position;

public:
	EllipseFigure()
	{}

	EllipseFigure(const CRect& position) : position(position)
	{}

	virtual void Serialize(CArchive& ar) override
	{
		Figure::Serialize(ar);

		if (ar.IsStoring())
			ar << position;
		else
			ar >> position;
	}

protected:
	virtual void DrawShape(CDC& dc)
	{
		dc.Ellipse(&position);
	}
};

class FigureHelper
{
	static std::random_device random;
	static std::mt19937		  mt;

public:
	static void AddRandomFigures(CArray<Figure*>& figures, size_t count, const CRect& area)
	{
		for (size_t counter = 0; counter < count; counter++)
			figures.Add(GetRandomFigure(area));
	}

private:
	static Figure* GetRandomFigure(const CRect& area)
	{
		const long figureKindNumber = 4;

		Figure* figure = nullptr;
		
		switch (RandomValue(0, figureKindNumber - 1)) {
		case 0:
			figure = new DotFigure(RandomPosition(area));
			break;
		case 1:
			figure = new LineFigure(RandomPosition(area), RandomPosition(area));
			break;
		case 2:
			figure = new RectangleFigure(CRect(RandomPosition(area), RandomPosition(area)));
			break;
		case 3:
			figure = new EllipseFigure(CRect(RandomPosition(area), RandomPosition(area)));
			break;
		default:
			ASSERT(false);
			figure = nullptr;
			break;
		}
		figure->attribute.color    = RandomColor();
		figure->attribute.penWidth = RandomValue(0, 3);
		return figure;
	}

	static CPoint RandomPosition(const CRect& area)
	{
		POINT randomPosition = { RandomValue(area.left, area.right), RandomValue(area.top, area.bottom) };
		return randomPosition;
	}

	static LONG RandomValue(long minimum, long maxmum)
	{
		return minimum + mt() % (maxmum - minimum + 1);
	};

	static COLORREF RandomColor()
	{
		return RGB(RandomValue(0, 255), RandomValue(0, 255), RandomValue(0, 255));
	}
};
