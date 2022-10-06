#pragma once
#include <afx.h>
#include <random>
using namespace std;

class FigureHelper
{
	static random_device random;
	static mt19937		 mt;

public:
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

class Figure : public CObject
{
public:
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
		const CSize size(radius, radius);
		dc.Ellipse(CRect(position - size, position + size));
	}
};
