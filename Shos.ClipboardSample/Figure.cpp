#include "pch.h"
#include "Figure.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_SERIAL(Figure, CObject, 1)
IMPLEMENT_SERIAL(DotFigure, Figure, 1)
IMPLEMENT_SERIAL(LineFigure, Figure, 1)
IMPLEMENT_SERIAL(RectangleFigureBase, Figure, 1)
IMPLEMENT_SERIAL(RectangleFigure, RectangleFigureBase, 1)
IMPLEMENT_SERIAL(EllipseFigure, RectangleFigureBase, 1)
