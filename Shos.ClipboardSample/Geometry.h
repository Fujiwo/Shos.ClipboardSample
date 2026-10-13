#pragma once

#include <afx.h>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <numbers>
#include <vector>

class Geometry
{
public:
    template <class T>
    static T Square(T x)
    {
        return x * x;
    }

    static long Round(double x)
    {
        return std::lround(x);
    }

    // absolute value
    static long Abs(CPoint point)
    {
        return Round(std::sqrt(Square(static_cast<double>(point.x)) + Square(static_cast<double>(point.y))));
    }

    static long Abs(CSize size)
    {
        return Round(std::sqrt(Square(static_cast<double>(size.cx)) + Square(static_cast<double>(size.cy))));
    }

    static long GetDistance(CPoint point1, CPoint point2)
    {
        return Abs(point2 - point1);
    }

    static long GetDistanceToLineSegment(CPoint point, CPoint start, CPoint end)
    {
        auto d      = end - start;
        auto length = Abs(d);
        if (length == 0)
            return GetDistance(point, start);

        auto d1 = start - point;
        auto d2 = end   - point;

        auto f0 = Mul(d, d1);
        auto f1 = Mul(d, d2);

        if (f0 > 0)
            return Abs(d1);
        if (f1 < 0)
            return Abs(d2);

        // outer product / length
        const auto outerProduct = static_cast<long long>(d.cy) * d1.cx - static_cast<long long>(d.cx) * d1.cy;
        return static_cast<long>(std::llabs(outerProduct) / length);
    }

    static long GetDistance(CPoint point, const CRect& rect)
    {
        return GetDistanceToPolygon(point, ToPoints(rect));
    }

    // Approximates the ellipse with a polygon and returns the distance from its outline.
    static long GetDistanceToEllipse(CPoint point, const CRect& rect)
    {
        if (rect.Width() == 0 || rect.Height() == 0)
            return GetDistanceToLineSegment(point, rect.TopLeft(), rect.BottomRight());

        constexpr int divisionNumber = 72;

        const auto   center  = rect.CenterPoint();
        const double radiusX = std::abs(rect.Width ()) / 2.0;
        const double radiusY = std::abs(rect.Height()) / 2.0;

        std::vector<CPoint> points;
        points.reserve(divisionNumber);
        for (int index = 0; index < divisionNumber; index++) {
            const double angle = 2.0 * std::numbers::pi * index / divisionNumber;
            points.emplace_back(center.x + Round(radiusX * std::cos(angle)), center.y + Round(radiusY * std::sin(angle)));
        }
        return GetDistanceToPolygon(point, points);
    }

    static long GetDistanceToPolygon(CPoint point, const std::vector<CPoint>& points)
    {
        auto distance = (std::numeric_limits<long>::max)();

        for (size_t index = 0; index < points.size(); index++) {
            auto eachDistance = GetDistanceToLineSegment(point, points[index], points[(index + 1) % points.size()]);
            if (eachDistance < distance)
                distance = eachDistance;
        }
        return distance;
    }

    // inner product
    static long long Mul(CPoint point1, CPoint point2)
    {
        return static_cast<long long>(point1.x) * point2.x + static_cast<long long>(point1.y) * point2.y;
    }

    static long long Mul(CSize size1, CSize size2)
    {
        return static_cast<long long>(size1.cx) * size2.cx + static_cast<long long>(size1.cy) * size2.cy;
    }

    static std::vector<CPoint> ToPoints(const CRect& rect)
    {
        return { rect.TopLeft(), CPoint(rect.right, rect.top), rect.BottomRight(), CPoint(rect.left, rect.bottom) };
    }

    static bool GetArea(const std::vector<CRect>& areas, CRect& area)
    {
        if (areas.empty())
            return false;

        area = areas[0];
        area.NormalizeRect();
        for (size_t index = 1; index < areas.size(); index++) {
            auto eachArea = areas[index];
            eachArea.NormalizeRect();
            area.UnionRect(area, eachArea);
        }
        return true;
    }
};
