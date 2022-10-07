#pragma once

#include <afx.h>
#include <random>
#include "GdiObjectSelector.h"

struct FigureAttribute
{
	COLORREF color;
	int      penWidth;

	FigureAttribute() : color(RGB(0x00, 0x00, 0x00)), penWidth(0)
	{}

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
	void Draw(CDC& dc) const
	{
		StockObjectSelector stockObjectSelector(dc, NULL_BRUSH);
		CPen pen(PS_SOLID, attribute.penWidth, attribute.color);
		GdiObjectSelector gdiObjectSelector(dc, pen);

		DrawShape(dc);
	}

	CRect GetArea() const
	{
		auto area = GetShapeArea();
		area.InflateRect(attribute.penWidth, attribute.penWidth);
		return area;
	}

	virtual void Serialize(CArchive& ar) override
	{
		CObject::Serialize(ar);
		attribute.Serialize(ar);
	}

protected:
	virtual void DrawShape(CDC& dc) const
	{}

	virtual CRect GetShapeArea() const
	{
		return CRect();
	}
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
	virtual void DrawShape(CDC& dc) const override
	{
		dc.Ellipse(GetShapeArea());
	}

	virtual CRect GetShapeArea() const override
	{
		const CSize size(radius, radius);
		return CRect(position - size, position + size);
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
	virtual void DrawShape(CDC& dc) const override
	{
		dc.MoveTo(start);
		dc.LineTo(end);
	}

	virtual CRect GetShapeArea() const override
	{
		CRect area(start, end);
		area.NormalizeRect();
		return area;
	}
};

class RectangleFigureBase : public Figure
{
	DECLARE_SERIAL(RectangleFigureBase)

	CRect position;

public:
	RectangleFigureBase()
	{}

	RectangleFigureBase(const CRect& position) : position(position)
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
	virtual CRect GetShapeArea() const override
	{
		return position;
	}
};

class RectangleFigure : public RectangleFigureBase
{
	DECLARE_SERIAL(RectangleFigure)

public:
	RectangleFigure()
	{}

	RectangleFigure(const CRect& position) : RectangleFigureBase(position)
	{}

protected:
	virtual void DrawShape(CDC& dc) const override
	{
		dc.Rectangle(&position);
	}
};

class EllipseFigure : public RectangleFigureBase
{
	DECLARE_SERIAL(EllipseFigure)

public:
	EllipseFigure()
	{}

	EllipseFigure(const CRect& position) : RectangleFigureBase(position)
	{}

protected:
	virtual void DrawShape(CDC& dc) const override
	{
		dc.Ellipse(&position);
	}
};

class FigureHelper
{
	static const long		  figureKindNumber = 4;

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
