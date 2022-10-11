#pragma once
#include <afx.h>
#include <limits>
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

	virtual void OnMouseMove(UINT nFlags, CPoint point)
	{}
	
	DECLARE_DYNCREATE(Command)
};

class SelectCommand : public Command
{
public:
	virtual void OnMouseMove(UINT nFlags, CPoint point) override
	{
		TRACE(_T("OnMouseMove(%d, %d)\n"), point.x, point.y);

		//long distance = 
		for (auto figure : GetModel()) {
		}
	}

	DECLARE_DYNCREATE(SelectCommand)
};

class CommandManager
{
	Model&	  model;
	Command*  currentCommand;

public:
	CommandManager(Model& model) : model(model), currentCommand(nullptr)
	{
		SetCommand(new SelectCommand());
	}

	virtual ~CommandManager()
	{
		delete currentCommand;
	}
	
	void SetCommand(Command* command)
	{
		delete currentCommand;
		currentCommand = command;
		currentCommand->Set(model);
	}

	void OnMouseMove(UINT nFlags, CPoint point)
	{
		if (currentCommand != nullptr)
			currentCommand->OnMouseMove(nFlags, point);
	}
};