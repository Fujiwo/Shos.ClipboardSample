#pragma once

#include "Model.h"
#include "Command.h"

class Document : public CDocument, public Observer<Hint>
{
	static constexpr COLORREF areaColor = RGB(0xff, 0xf0, 0xe0);

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

	bool IsEmpty() const
	{
		return model.IsEmpty();
	}

	void RemoveAll()
	{
		model.RemoveAll();
	}

	void Paste(CArchive& ar)
	{
		model.Paste(ar);
	}

	void Draw(CDC& dc) const
	{
		DrawArea(dc);
		for (auto* figure : *this)
			figure->Draw(dc);
	}

	void DrawArea(CDC& dc) const
	{
		dc.FillSolidRect(GetArea(), areaColor);
	}

	void DrawCommand(CDC& dc)
	{
		commandManager.Draw(dc);
	}

	void OnClick(CPoint point)
	{
		commandManager.OnClick(point);
	}

	void OnMouseMove(CPoint point)
	{
		if (IsValid(point))
			commandManager.OnMouseMove(point);
		else
			commandManager.OnMouseLeave();
	}

	void OnMouseLeave()
	{
		commandManager.OnMouseLeave();
	}

protected:
	virtual void Update(Hint& hint) override
	{
		if (hint.type != Hint::Type::ViewOnly)
			SetModifiedFlag();
		UpdateAllViews(nullptr, 0, &hint);
	}

	virtual void Serialize(CArchive& ar) override
	{
		model.Serialize(ar);
	}

	virtual void DeleteContents() override
	{
		model.Clear();
		CDocument::DeleteContents();
	}

	afx_msg void OnFigureRandom()
	{
		constexpr size_t count = 10;
		model.AddDummyData(count);
	}

private:
	bool IsValid(CPoint point) const
	{
		return GetArea().PtInRect(point) != FALSE;
	}

	DECLARE_DYNCREATE(Document)
	DECLARE_MESSAGE_MAP()
};
