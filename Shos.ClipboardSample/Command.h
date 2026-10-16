#pragma once
#include <afx.h>
#include <array>
#include <limits>
#include <memory>
#include "Model.h"

// Base class of the commands (editing tools).
// The current command decides how mouse input on the view is handled, and draws its own
// feedback (such as a cursor) on top of the figures.
class Command : public CObject
{
	Model* model = nullptr;

protected:
	Model& GetModel() const
	{
		ASSERT_POINTER(model, Model);
		return *model;
	}

public:
	Command() = default;

	// Binds the command to the model it works on.
	void Set(Model& model)
	{
		this->model = &model;
	}

	// Draws the feedback of the command on top of the figures.
	virtual void Draw(CDC& /* dc */)
	{}

	// Mouse event handlers. Points are in logical (document) coordinates.
	virtual void OnClick(CPoint /* point */)
	{}

	virtual void OnMouseMove(CPoint /* point */)
	{}

	virtual void OnMouseLeave()
	{}

	DECLARE_DYNCREATE(Command)
};

// Highlights the figure nearest to the mouse cursor, and selects it when clicked.
class SelectCommand : public Command
{
	// Maximum distance (in logical units) at which a figure can be picked.
	static constexpr LONG	  searchingDistance = 100;
	// Appearance of the crosshair cursor. The length (of each half of a line) is in device units.
	static constexpr LONG	  cursorLength		= 100;
	static constexpr long	  cursorPenWidth	= 3;
	static constexpr COLORREF cursorColor		= RGB(0xff, 0x00, 0xff);

	CPoint cursorPosition;
	bool   cursorPositionExists = false;

public:
	SelectCommand() = default;

	virtual void Draw(CDC& dc) override
	{
		if (cursorPositionExists) {
			// Draw a crosshair cursor. Its size is given in device units so that it looks the same at any
			// zoom level: the end points are calculated in device coordinates and then converted back to
			// logical coordinates for drawing.
			CPen pen(PS_SOLID, cursorPenWidth, cursorColor);
			GdiObjectSelector penSelector(dc, pen);

			auto deviceCursorPosition = cursorPosition;
			dc.LPtoDP(&deviceCursorPosition);

			std::array<CPoint, 4> points = {{
				{ deviceCursorPosition.x - cursorLength, deviceCursorPosition.y },
				{ deviceCursorPosition.x + cursorLength, deviceCursorPosition.y },
				{ deviceCursorPosition.x, deviceCursorPosition.y - cursorLength },
				{ deviceCursorPosition.x, deviceCursorPosition.y + cursorLength }
			}};
			constexpr int pointNumber = static_cast<int>(points.size());
			dc.DPtoLP(points.data(), pointNumber);

			// The horizontal line, then the vertical line.
			dc.Polyline(points.data()	 , pointNumber / 2);
			dc.Polyline(points.data() + 2, pointNumber / 2);
		}
		// Outline the figure under the cursor.
		if (const auto* highlightedFigure = GetModel().Highlight())
			highlightedFigure->DrawArea(dc);
	}

	// Selects the figure nearest to the clicked point, or clears the selection when there is none.
	virtual void OnClick(CPoint point) override
	{
		TRACE(_T("OnClick(x: %d, y: %d)\n"), point.x, point.y);

		if (auto* nearestFigure = GetNearestFigure(point))
			GetModel().Select(*nearestFigure);
		else
			GetModel().UnSelectAll();
	}

	// Moves the cursor and highlights the figure nearest to it.
	virtual void OnMouseMove(CPoint point) override
	{
		cursorPosition		 = point;
		cursorPositionExists = true;
		GetModel().Highlight(GetNearestFigure(point));
	}

	// Hides the cursor and the highlight.
	virtual void OnMouseLeave() override
	{
		cursorPositionExists = false;
		GetModel().Highlight(nullptr);
	}

private:
	// Returns the figure nearest to the point within searchingDistance, or nullptr when there is none.
	Figure* GetNearestFigure(CPoint point)
	{
		auto	distance	  = (std::numeric_limits<long>::max)();
		Figure* nearestFigure = nullptr;

		CRect searchingRect(point, point);
		searchingRect.InflateRect(searchingDistance, searchingDistance);

		for (auto* figure : GetModel()) {
			// Do a cheap bounding-box test first, and calculate the exact distance only for the figures near enough.
			if (CRect rect; rect.IntersectRect(figure->GetArea(), searchingRect)) {
				const auto figureDistance = figure->GetDistanceFrom(point);
				if (figureDistance <= searchingDistance && figureDistance < distance) {
					distance	  = figureDistance;
					nearestFigure = figure;
				}
			}
		}
		return nearestFigure;
	}

	DECLARE_DYNCREATE(SelectCommand)
};

// Owns the current command and forwards drawing and mouse events to it.
class CommandManager
{
	Model&					 model;
	std::unique_ptr<Command> currentCommand;

public:
	explicit CommandManager(Model& model) : model(model)
	{
		// The select command is the initial (and currently the only) command.
		SetCommand(std::make_unique<SelectCommand>());
	}

	// Replaces the current command (the previous one is destroyed) and binds the new one to the model.
	void SetCommand(std::unique_ptr<Command> command)
	{
		currentCommand = std::move(command);
		if (currentCommand != nullptr)
			currentCommand->Set(model);
	}

	void Draw(CDC& dc)
	{
		if (auto* command = GetCurrentCommand())
			command->Draw(dc);
	}

	void OnClick(CPoint point)
	{
		if (auto* command = GetCurrentCommand())
			command->OnClick(point);
	}

	void OnMouseMove(CPoint point)
	{
		if (auto* command = GetCurrentCommand())
			command->OnMouseMove(point);
	}

	void OnMouseLeave()
	{
		if (auto* command = GetCurrentCommand())
			command->OnMouseLeave();
	}

private:
	Command* GetCurrentCommand() const
	{
		if (currentCommand == nullptr)
			return nullptr;

		ASSERT_VALID(currentCommand.get());
		return currentCommand.get();
	}

	CommandManager(const CommandManager& other) = delete;
	CommandManager& operator=(const CommandManager& other) = delete;
};
