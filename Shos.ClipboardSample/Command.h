#pragma once
#include <afx.h>
#include <array>
#include <limits>
#include <memory>
#include "Model.h"

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
	static constexpr LONG	  searchingDistance = 100;
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

			dc.Polyline(points.data()	 , pointNumber / 2);
			dc.Polyline(points.data() + 2, pointNumber / 2);
		}
		if (const auto* highlightedFigure = GetModel().Highlight())
			highlightedFigure->DrawArea(dc);
	}

	virtual void OnClick(CPoint point) override
	{
		TRACE(_T("OnClick(x: %d, y: %d)\n"), point.x, point.y);

		if (auto* nearestFigure = GetNearestFigure(point))
			GetModel().Select(*nearestFigure);
		else
			GetModel().UnSelectAll();
	}

	virtual void OnMouseMove(CPoint point) override
	{
		cursorPosition		 = point;
		cursorPositionExists = true;
		GetModel().Highlight(GetNearestFigure(point));
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

		for (auto* figure : GetModel()) {
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

class CommandManager
{
	Model&					 model;
	std::unique_ptr<Command> currentCommand;

public:
	explicit CommandManager(Model& model) : model(model)
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
