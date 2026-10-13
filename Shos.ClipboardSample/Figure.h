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

struct FigureAttribute
{
	COLORREF color	  = RGB(0x00, 0x00, 0x00);
	int      penWidth = 0;

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
	Figure(const Figure& another) : attribute(another.attribute), selected(another.selected)
	{}

	void Select(bool selected)
	{
		this->selected = selected;
	}

	virtual Figure* Clone() const
	{
		return new Figure(*this);
	}

	void Draw(CDC& dc) const
	{
		StockObjectSelector stockObjectSelector(dc, NULL_BRUSH);
		CPen				pen(PS_SOLID, attribute.penWidth, attribute.color);
		GdiObjectSelector   penSelector(dc, pen);

		DrawShape(dc);
		if (selected)
			DrawSelector(dc);
	}

	void DrawArea(CDC& dc) const
	{
		StockObjectSelector stockObjectSelector(dc, NULL_BRUSH);
		CPen				pen(PS_SOLID, 3, areaColor);
		GdiObjectSelector	penSelector(dc, pen);

		dc.Rectangle(GetArea());
	}

	CRect GetArea() const
	{
		auto area   = GetShapeArea();
		auto margin = attribute.penWidth + selectorSize + selectorPenWidth;
		area.InflateRect(margin, margin);
		return area;
	}

	virtual long GetDistanceFrom(CPoint /* point */) const
	{
		return (std::numeric_limits<long>::max)();
	}

	virtual void Serialize(CArchive& ar) override
	{
		CObject::Serialize(ar);
		attribute.Serialize(ar);
	}

protected:
	virtual void DrawShape(CDC& /* dc */) const
	{}

	virtual CRect GetShapeArea() const
	{
		return CRect();
	}

	virtual std::vector<CPoint> GetPoints() const
	{
		return {};
	}

private:
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

class FigureHelper
{
	static constexpr long figureKindNumber = 4;

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

	static CPoint RandomPosition(const CRect& area)
	{
		return { RandomValue(area.left, area.right), RandomValue(area.top, area.bottom) };
	}

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
