#pragma once
#include <afx.h>
#include <random>

class NullBrushSelector
{
	CDC& dc;
	CGdiObject* const oldBrush;

public:
	NullBrushSelector(CDC& dc) : dc(dc), oldBrush(dc.SelectStockObject(NULL_BRUSH))
	{}

	~NullBrushSelector()
	{
		dc.SelectObject(oldBrush);
	}
};

class Figure : public CObject
{
public:
	virtual ~Figure() = 0 {}

	virtual void Draw(CDC& dc) {}
};

class DotFigure : public Figure
{
	const LONG radius = 10L;
	CPoint position;
		
public:
	DotFigure()
	{}

	DotFigure(const CPoint& position) : position(position)
	{}

	virtual void Draw(CDC& dc) override
	{
		class NullBrushSelector brushSelector(dc);
		const CSize size(radius, radius);
		dc.Ellipse(CRect(position - size, position + size));
	}
};

class LineFigure : public Figure
{
	CPoint start, end;

public:
	LineFigure()
	{}

	LineFigure(CPoint start, CPoint end) : start(start), end(end)
	{}

	virtual void Draw(CDC& dc) override
	{
		dc.MoveTo(start);
		dc.LineTo(end);
	}
};

class RectangleFigure : public Figure
{
	CRect position;

public:
	RectangleFigure()
	{}

	RectangleFigure(const CRect& position) : position(position)
	{}

	virtual void Draw(CDC& dc) override
	{
		class NullBrushSelector brushSelector(dc);
		dc.Rectangle(&position);
	}
};

class EllipseFigure : public Figure
{
	CRect position;

public:
	EllipseFigure()
	{}

	EllipseFigure(const CRect& position) : position(position)
	{}

	virtual void Draw(CDC& dc) override
	{
		class NullBrushSelector brushSelector(dc);
		dc.Ellipse(&position);
	}
};

class FigureHelper
{
	static std::random_device random;
	static std::mt19937		  mt;

public:
	static void AddRandomFigures(CTypedPtrArray<CObArray, Figure*>& figures, size_t count, const CRect& area)
	{
		for (size_t counter = 0; counter < count; counter++)
			figures.Add(GetRandomFigure(area));
	}

private:
	static Figure* GetRandomFigure(const CRect& area)
	{
		const long figureKindNumber = 4;

		switch (RandomValue(0, figureKindNumber - 1)) {
		case 0:
			return new DotFigure(RandomPosition(area));
		case 1:
			return new LineFigure(RandomPosition(area), RandomPosition(area));
		case 2:
			return new RectangleFigure(CRect(RandomPosition(area), RandomPosition(area)));
		case 3:
			return new EllipseFigure(CRect(RandomPosition(area), RandomPosition(area)));
		default:
			ASSERT(false);
			return nullptr;
		}
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
};
