#pragma once

#include <afx.h>
#include <vector>

class Geometry
{
public:
	static bool GetArea(const std::vector<CRect>& areas, CRect& area)
	{
		if (areas.size() == 0)
			return false;

		area = areas[0];
		for (auto index = 1; index < areas.size(); index++) {
			auto eachArea = areas[index];
			eachArea.NormalizeRect();
			area.UnionRect(area, eachArea);
		}
		return true;
	}
};
