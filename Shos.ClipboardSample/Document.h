#pragma once

#include "Model.h"
#include "Command.h"

class Document : public CDocument, public Observer<Hint>
{
	Model		   model;
	CommandManager commandManager;

public:
	using iterator = Model::iterator;

	const CSize GetSize() const { return model.GetSize(); }
	const CRect GetArea() const { return model.GetArea(); }

	Document() : commandManager(model)
	{
		model.AddObserver(*this);
	}

	iterator begin() const
	{
		return model.begin();
	}

	iterator end() const
	{
		return model.end();
	}

	void Add(Figure* figure)
	{
		model.Add(figure);
		SetModifiedFlag();
	}

	void Draw(CDC& dc) const
	{
		for (auto figure : *this)
			figure->Draw(dc);
	}

	void OnClick(CPoint point)
	{
		if (GetArea().PtInRect(point))
			commandManager.OnClick(point);
	}

protected:
	virtual void Update(Hint& hint) override
	{
		SetModifiedFlag();
		UpdateAllViews(nullptr, 0, &hint);
	}

	virtual void Serialize(CArchive& ar)
	{
		model.Serialize(ar);
	}

	virtual void DeleteContents()
	{
		model.Clear();
		CDocument::DeleteContents();
	}

	afx_msg void OnFigureRandom()
	{
		const size_t count = 10;
		model.AddDummyData(count);
	}

	DECLARE_DYNCREATE(Document)
	DECLARE_MESSAGE_MAP()
};
