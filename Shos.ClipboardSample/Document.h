#pragma once

#include "Model.h"
#include "Command.h"

// The MFC document.
// Owns the model (the figures) and the command manager, observes the model, and passes its
// changes on to the views. Mouse input from the view is forwarded to the current command.
class Document : public CDocument, public Observer<Hint>
{
	// Background color of the drawing area.
	static constexpr COLORREF areaColor = RGB(0xff, 0xf0, 0xe0);

	// model must be declared before commandManager, which refers to it on construction.
	Model		   model;
	CommandManager commandManager;

public:
	using iterator = Model::iterator;

	// The size and the area of the drawing area in logical coordinates.
	const CSize GetSize() const { return model.GetSize(); }
	const CRect GetArea() const { return model.GetArea(); }

	Document() : commandManager(model)
	{
		model.AddObserver(*this);
	}

	// Iteration over the figures (for range-based for).
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

	// Adds the figures read from the archive (the clipboard data).
	void Paste(CArchive& ar)
	{
		model.Paste(ar);
	}

	// Draws the drawing area and all the figures (used for copying to the clipboard).
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

	// Draws the feedback of the current command (the cursor and the highlight).
	void DrawCommand(CDC& dc)
	{
		commandManager.Draw(dc);
	}

	// Mouse event handlers. Points are in logical coordinates.
	void OnClick(CPoint point)
	{
		commandManager.OnClick(point);
	}

	void OnMouseMove(CPoint point)
	{
		// Outside the drawing area, the mouse is treated as having left.
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
	// Called by the model when it has changed.
	// Marks the document as modified (except for view-only changes such as selection),
	// and passes the hint to the views so that they can redraw only the changed area.
	virtual void Update(Hint& hint) override
	{
		if (hint.type != Hint::Type::ViewOnly)
			SetModifiedFlag();
		UpdateAllViews(nullptr, 0, &hint);
	}

	// Saves/loads the figures to/from a file (also used for the clipboard data).
	virtual void Serialize(CArchive& ar) override
	{
		model.Serialize(ar);
	}

	// Called by MFC before a new document is created or a file is opened.
	virtual void DeleteContents() override
	{
		model.Clear();
		CDocument::DeleteContents();
	}

	// [Figure] - [Random] menu: adds random figures.
	afx_msg void OnFigureRandom()
	{
		constexpr size_t count = 10;
		model.AddDummyData(count);
	}

private:
	// Returns true when the point is inside the drawing area.
	bool IsValid(CPoint point) const
	{
		return GetArea().PtInRect(point) != FALSE;
	}

	DECLARE_DYNCREATE(Document)
	DECLARE_MESSAGE_MAP()
};
