#pragma once
#include <afx.h>
#include <iterator>
#include <limits>
#include <memory>
#include "Model.h"

class Command : public CObject
{
	Model* model;

protected:
	Model& GetModel() const
	{
		ASSERT_POINTER(model, Model);
		return *model;
	}

public:
	Command() : model(nullptr)
	{}

	void Set(Model& model)
	{
		this->model = &model;
	}

	virtual void Draw(CDC& /* dc */)
	{}

	virtual void OnClick(CPoint /* point */)
	{}

	virtual void OnMouseMove(CPoint /* point */)
	{}

	virtual void OnMouseLeave()
	{}

	DECLARE_DYNCREATE(Command)
};

class SelectCommand : public Command
{
	static const LONG	  searchingDistance = 100;
	static const long	  cursorPenWidth	= 3;
	static const COLORREF cursorColor		= RGB(0xff, 0x00, 0xff);

	CPoint  cursorPosition;
	bool    cursorPositionExists;

public:
	SelectCommand() : cursorPositionExists(false)
	{}

	virtual void Draw(CDC& dc) override
	{
		if (cursorPositionExists) {
			CPen pen(PS_SOLID, cursorPenWidth, cursorColor);
			GdiObjectSelector penSelector(dc, pen);

			auto deviceCursorPosition = cursorPosition;
			dc.LPtoDP(&deviceCursorPosition);

			const LONG cursorLength = 100;

			CPoint points[] = {
				{ deviceCursorPosition.x - cursorLength, deviceCursorPosition.y },
				{ deviceCursorPosition.x + cursorLength, deviceCursorPosition.y },
				{ deviceCursorPosition.x, deviceCursorPosition.y - cursorLength },
				{ deviceCursorPosition.x, deviceCursorPosition.y + cursorLength }
			};
			constexpr int pointNumber = static_cast<int>(std::size(points));
			dc.DPtoLP(points, pointNumber);

			dc.Polyline(points	  , pointNumber / 2);
			dc.Polyline(points + 2, pointNumber / 2);
		}
		if (GetModel().Highlight() != nullptr)
			GetModel().Highlight()->DrawArea(dc);
	}

	virtual void OnClick(CPoint point) override
	{
		TRACE(_T("OnClick(x: %d, y: %d)\n"), point.x, point.y);

		auto nearestFigure = GetNearestFigure(point);
		if (nearestFigure == nullptr)
			GetModel().UnSelectAll();
		else
			GetModel().Select(*nearestFigure);
	}

	virtual void OnMouseMove(CPoint point) override
	{
		cursorPosition		 = point;
		cursorPositionExists = true;
		auto nearestFigure = GetNearestFigure(point);
		GetModel().Highlight(nearestFigure);
	}

	virtual void OnMouseLeave() override
	{
		cursorPositionExists = false;
		GetModel().Highlight(nullptr);
	}

private:
	Figure* GetNearestFigure(CPoint point)
	{
		auto	distance	  = (std::numeric_limits<long>::max)();
		Figure* nearestFigure = nullptr;

		CRect searchingRect(point, point);
		searchingRect.InflateRect(searchingDistance, searchingDistance);

		for (auto figure : GetModel()) {
			CRect rect;
			if (rect.IntersectRect(figure->GetArea(), searchingRect)) {
				auto figureDistance = figure->GetDistanceFrom(point);
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

class CommandManager
{
	Model&					 model;
	std::unique_ptr<Command> currentCommand;

public:
	CommandManager(Model& model) : model(model)
	{
		SetCommand(std::make_unique<SelectCommand>());
	}

	void SetCommand(std::unique_ptr<Command> command)
	{
		currentCommand = std::move(command);
		if (currentCommand != nullptr)
			currentCommand->Set(model);
	}

	void Draw(CDC& dc)
	{
		if (GetCurrentCommand() != nullptr)
			GetCurrentCommand()->Draw(dc);
	}

	void OnClick(CPoint point)
	{
		if (GetCurrentCommand() != nullptr)
			GetCurrentCommand()->OnClick(point);
	}

	void OnMouseMove(CPoint point)
	{
		if (GetCurrentCommand() != nullptr)
			GetCurrentCommand()->OnMouseMove(point);
	}

	void OnMouseLeave()
	{
		if (GetCurrentCommand() != nullptr)
			GetCurrentCommand()->OnMouseLeave();
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
