#include "pch.h"
#include "Figure.h"

IMPLEMENT_SERIAL(Figure, CObject, 1)
IMPLEMENT_SERIAL(DotFigure, Figure, 1)
IMPLEMENT_SERIAL(LineFigure, Figure, 1)
IMPLEMENT_SERIAL(RectangleFigure, Figure, 1)
IMPLEMENT_SERIAL(EllipseFigure, Figure, 1)

std::random_device FigureHelper::random;
std::mt19937 FigureHelper::mt(random());
