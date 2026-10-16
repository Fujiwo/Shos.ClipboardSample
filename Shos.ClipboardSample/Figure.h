#pragma once

#include <afx.h>
#include <algorithm>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <span>
#include <vector>
#include "GdiObjectSelector.h"
#include "Geometry.h"

// Drawing attributes common to all the figures.
struct FigureAttribute
{
	COLORREF color	  = RGB(0x00, 0x00, 0x00);
	// Pen width in logical units (0 means a line one pixel wide).
	int      penWidth = 0;

	void Serialize(CArchive& ar)
	{
		if (ar.IsStoring())
			ar << color << penWidth;
		else
			ar >> color >> penWidth;
	}
};

// Base class of all the figures. Coordinates are logical (document) coordinates.
// Figures are serializable (DECLARE_SERIAL), so they can be saved to a file and copied to the clipboard.
// Derived classes override DrawShape, GetShapeArea, GetPoints and GetDistanceFrom.
class Figure : public CObject
{
	// Appearance of the selection handles and of the highlight outline.
	static constexpr long	  selectorSize	   = 10L;
	static constexpr long	  selectorPenWidth = 5;
	static constexpr COLORREF selectedColor	   = RGB(0x80, 0x00, 0x40);
	static constexpr COLORREF areaColor		   = RGB(0x00, 0xa0, 0xff);

	FigureAttribute attribute;
	bool            selected = false;

public:
	const FigureAttribute& Attribute() const
	{
		return attribute;
	}

	FigureAttribute& Attribute()
	{
		return attribute;
	}

	Figure() = default;

	// CObject is not copyable, so the copy constructor must be written explicitly.
	// Clone() relies on the copy constructors of the derived classes.
	Figure(const Figure& another) : attribute(another.attribute), selected(another.selected)
	{}

	// Selected figures are drawn with handles at their characteristic points.
	void Select(bool selected)
	{
		this->selected = selected;
	}

	// Returns a copy allocated with new. The caller takes ownership.
	virtual Figure* Clone() const
	{
		return new Figure(*this);
	}

	// Draws the outline of the shape (not filled) with the attribute, and the handles when selected.
	void Draw(CDC& dc) const
	{
		StockObjectSelector stockObjectSelector(dc, NULL_BRUSH);
		CPen				pen(PS_SOLID, attribute.penWidth, attribute.color);
		GdiObjectSelector   penSelector(dc, pen);

		DrawShape(dc);
		if (selected)
			DrawSelector(dc);
	}

	// Outlines the area of the figure (used to highlight it).
	void DrawArea(CDC& dc) const
	{
		StockObjectSelector stockObjectSelector(dc, NULL_BRUSH);
		CPen				pen(PS_SOLID, 3, areaColor);
		GdiObjectSelector	penSelector(dc, pen);

		dc.Rectangle(GetArea());
	}

	// Returns the bounding rectangle of everything drawn for the figure,
	// i.e. the shape plus the pen width and the selection handles. Used for redrawing and hit testing.
	CRect GetArea() const
	{
		auto area   = GetShapeArea();
		auto margin = attribute.penWidth + selectorSize + selectorPenWidth;
		area.InflateRect(margin, margin);
		return area;
	}

	// Returns the distance from the point to the figure (used to find the figure nearest to the mouse).
	virtual long GetDistanceFrom(CPoint /* point */) const
	{
		return (std::numeric_limits<long>::max)();
	}

	virtual void Serialize(CArchive& ar) override
	{
		// Derived classes call this first, then serialize their own data.
		CObject::Serialize(ar);
		attribute.Serialize(ar);
	}

protected:
	// Draws the shape itself with the pen and the brush currently selected into the DC.
	virtual void DrawShape(CDC& /* dc */) const
	{}

	// Returns the bounding rectangle of the shape itself.
	virtual CRect GetShapeArea() const
	{
		return CRect();
	}

	// Returns the characteristic points of the shape, where the selection handles are drawn.
	virtual std::vector<CPoint> GetPoints() const
	{
		return {};
	}

private:
	// Draws a square handle at each characteristic point.
	void DrawSelector(CDC& dc) const
	{
		StockObjectSelector stockObjectSelector(dc, NULL_BRUSH);
		CPen				pen(PS_SOLID, selectorPenWidth, selectedColor);
		GdiObjectSelector	penSelector(dc, pen);

		for (const auto& point : GetPoints())
			DrawSelector(dc, point);
	}

	void DrawSelector(CDC& dc, CPoint point) const
	{
		CRect rect(point, point);
		rect.InflateRect(selectorSize, selectorSize);
		dc.Rectangle(rect);
	}

	DECLARE_SERIAL(Figure)
};

// A dot: a small circle of a fixed radius around a position.
class DotFigure : public Figure
{
	static constexpr long radius = 10L;

	CPoint position;

public:
	DotFigure() = default;
	DotFigure(const DotFigure& another) = default;

	explicit DotFigure(const CPoint& position) : position(position)
	{}

	virtual Figure* Clone() const override
	{
		return new DotFigure(*this);
	}

	virtual long GetDistanceFrom(CPoint point) const override
	{
		// The distance from the circle (0 inside it).
		return (std::max)(Geometry::GetDistance(point, position) - radius, 0L);
	}

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

	virtual std::vector<CPoint> GetPoints() const override
	{
		return { position };
	}

	DECLARE_SERIAL(DotFigure)
};

// A line segment from start to end.
class LineFigure : public Figure
{
	CPoint start, end;

public:
	LineFigure() = default;
	LineFigure(const LineFigure& another) = default;

	LineFigure(CPoint start, CPoint end) : start(start), end(end)
	{}

	virtual Figure* Clone() const override
	{
		return new LineFigure(*this);
	}

	virtual long GetDistanceFrom(CPoint point) const override
	{
		return Geometry::GetDistanceToLineSegment(point, start, end);
	}

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

	virtual std::vector<CPoint> GetPoints() const override
	{
		return { start, end };
	}

	DECLARE_SERIAL(LineFigure)
};

// Common base of the figures defined by a bounding rectangle.
// The rectangle is normalized (left <= right, top <= bottom) on construction.
class RectangleFigureBase : public Figure
{
protected:
	CRect position;

public:
	RectangleFigureBase() = default;
	RectangleFigureBase(const RectangleFigureBase& another) = default;

	explicit RectangleFigureBase(const CRect& position) : position(position)
	{
		this->position.NormalizeRect();
	}

	virtual Figure* Clone() const override
	{
		return new RectangleFigureBase(*this);
	}

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

	virtual std::vector<CPoint> GetPoints() const override
	{
		return Geometry::ToPoints(position);
	}

	DECLARE_SERIAL(RectangleFigureBase)
};

// A rectangle.
class RectangleFigure : public RectangleFigureBase
{
public:
	RectangleFigure() = default;

	explicit RectangleFigure(const CRect& position) : RectangleFigureBase(position)
	{}

	virtual Figure* Clone() const override
	{
		return new RectangleFigure(*this);
	}

	virtual long GetDistanceFrom(CPoint point) const override
	{
		return Geometry::GetDistance(point, position);
	}

protected:
	virtual void DrawShape(CDC& dc) const override
	{
		dc.Rectangle(&position);
	}

	DECLARE_SERIAL(RectangleFigure)
};

// An ellipse inscribed in the rectangle.
class EllipseFigure : public RectangleFigureBase
{
public:
	EllipseFigure() = default;

	explicit EllipseFigure(const CRect& position) : RectangleFigureBase(position)
	{}

	virtual Figure* Clone() const override
	{
		return new EllipseFigure(*this);
	}

	virtual long GetDistanceFrom(CPoint point) const override
	{
		return Geometry::GetDistanceToEllipse(point, position);
	}

protected:
	virtual void DrawShape(CDC& dc) const override
	{
		dc.Ellipse(&position);
	}

	DECLARE_SERIAL(EllipseFigure)
};

// Utilities for collections of figures, including random figures for test data.
class FigureHelper
{
	// The number of figure types GetRandomFigure can create (the cases of its switch).
	static constexpr long figureKindNumber = 4;

	// The random number engine, seeded once per process.
	inline static std::mt19937 mt{ std::random_device{}() };

public:
	// The caller takes ownership of the returned figures.
	static std::vector<Figure*> GetRandomFigures(size_t count, const CRect& area)
	{
		std::vector<Figure*> figures;
		figures.reserve(count);
		for (size_t counter = 0; counter < count; counter++) {
			if (auto figure = GetRandomFigure(area))
				figures.push_back(figure.release());
		}
		return figures;
	}

	// Returns the union of the areas of the figures, or std::nullopt when there is no figure.
	static std::optional<CRect> GetArea(std::span<Figure* const> figures)
	{
		std::vector<CRect> areas;
		areas.reserve(figures.size());
		std::ranges::transform(figures, std::back_inserter(areas), &Figure::GetArea);
		return Geometry::GetArea(areas);
	}

private:
	// Creates a figure of a random type at random positions within the area, with random attributes.
	static std::unique_ptr<Figure> GetRandomFigure(const CRect& area)
	{
		std::unique_ptr<Figure> figure;

		switch (RandomValue(0, figureKindNumber - 1)) {
		case 0:
			figure = std::make_unique<DotFigure>(RandomPosition(area));
			break;
		case 1:
			figure = std::make_unique<LineFigure>(RandomPosition(area), RandomPosition(area));
			break;
		case 2:
			figure = std::make_unique<RectangleFigure>(CRect(RandomPosition(area), RandomPosition(area)));
			break;
		case 3:
			figure = std::make_unique<EllipseFigure>(CRect(RandomPosition(area), RandomPosition(area)));
			break;
		default:
			ASSERT(false);
			return nullptr;
		}
		figure->Attribute() = { .color = RandomColor(), .penWidth = static_cast<int>(RandomValue(0, 5)) };
		return figure;
	}

	// Returns a random point within the area (including its edges).
	static CPoint RandomPosition(const CRect& area)
	{
		return { RandomValue(area.left, area.right), RandomValue(area.top, area.bottom) };
	}

	// Returns a uniformly distributed random value in [minimum, maximum].
	static long RandomValue(long minimum, long maximum)
	{
		std::uniform_int_distribution<long> distribution(minimum, maximum);
		return distribution(mt);
	}

	static COLORREF RandomColor()
	{
		return RGB(RandomValue(0, 255), RandomValue(0, 255), RandomValue(0, 255));
	}
};
