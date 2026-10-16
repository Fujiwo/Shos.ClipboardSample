#pragma once

#include <afx.h>
#include <algorithm>
#include <utility>
#include <vector>
#include "Observer.h"
#include "Figure.h"

// A change notification sent from the model to its observers, and then to the views
// (as the pHint of CView::OnUpdate, which is why it derives from CObject).
struct Hint : public CObject
{
	enum class Type
	{
		// Figures were added.
		Added   ,
		// Figures were removed.
		Removed ,
		// figures[0] was replaced with figures[1].
		Changed ,
		// Only the appearance changed (e.g. the selection). The document is not modified.
		ViewOnly
	};

	// The figures affected by the change, used to calculate the area to redraw.
	// Valid only during the notification: removed or replaced figures are deleted right after it.
	std::vector<Figure*> figures;
	Type				 type;

	Hint(const Hint& other) = delete;

	explicit Hint(Type type) : type(type)
	{}

	Hint(Type type, Figure* figure) : type(type)
	{
		figures.push_back(figure);
	}

	Hint(Type type, std::vector<Figure*> figures) : figures(std::move(figures)), type(type)
	{}

	Hint& operator =(const Hint& other) = delete;

	DECLARE_DYNAMIC(Hint)
};

// The data model: owns the figures, and notifies the observers (the document) of changes with a Hint.
class Model : public Observable<Hint>
{
	// The width and the height of the drawing area in logical units.
	static constexpr LONG size = 2000L;

	// The owned figures, in drawing order.
	std::vector<Figure*> figures;
	// The figure under the mouse cursor (not owned), or nullptr.
	const Figure*		 highlightedFigure = nullptr;

public:
	using iterator = std::vector<Figure*>::const_iterator;

	const CSize GetSize() const { return CSize(size, size); }
	const CRect GetArea() const { return CRect(CPoint(), GetSize()); }

	Model() = default;

	virtual ~Model()
	{
		Clear();
	}

	iterator begin() const
	{
		return figures.begin();
	}

	iterator end() const
	{
		return figures.end();
	}

	bool IsEmpty() const
	{
		return figures.empty();
	}

	// Adds the figure. The model takes ownership.
	void Add(Figure* figure)
	{
		figures.push_back(figure);
		Hint hint(Hint::Type::Added, figure);
		Update(hint);
	}

	// Replaces oldFigure with newFigure. The model takes ownership of newFigure and deletes oldFigure.
	bool Change(Figure* oldFigure, Figure* newFigure)
	{
		auto iterator = std::ranges::find(figures, oldFigure);
		if (iterator == figures.end())
			return false;

		*iterator = newFigure;
		if (highlightedFigure == oldFigure)
			highlightedFigure = nullptr;

		Hint hint(Hint::Type::Changed, { oldFigure, newFigure });
		Update(hint);

		delete oldFigure;
		return true;
	}

	// Removes and deletes all the figures.
	// They are deleted after the notification, so that the observers can still use them (e.g. for the area to redraw).
	void RemoveAll()
	{
		if (figures.empty())
			return;

		auto removedFigures = figures;
		figures.clear();
		highlightedFigure = nullptr;

		Hint hint(Hint::Type::Removed, removedFigures);
		Update(hint);

		for (auto* figure : removedFigures)
			delete figure;
	}

	// Adds the figures read from the archive (e.g. the clipboard).
	void Paste(CArchive& ar)
	{
		auto newFigures = Load(ar);
		if (newFigures.empty())
			return;

		figures.insert(figures.end(), newFigures.begin(), newFigures.end());
		Hint hint(Hint::Type::Added, newFigures);
		Update(hint);
	}

	// Selects only the given figure.
	void Select(Figure& figure)
	{
		ClearSelected();
		figure.Select(true);
		Hint hint(Hint::Type::ViewOnly);
		Update(hint);
	}

	// Sets the figure under the mouse cursor. No notification is sent, since the view repaints on every mouse move.
	void Highlight(const Figure* figure)
	{
		highlightedFigure = figure;
	}

	// Returns the figure under the mouse cursor, or nullptr.
	const Figure* Highlight() const
	{
		return highlightedFigure;
	}

	// Clears the selection.
	void UnSelectAll()
	{
		ClearSelected();
		Hint hint(Hint::Type::ViewOnly);
		Update(hint);
	}

	// Stores all the figures, or loads figures and appends them.
	// No notification is sent on loading, since MFC redraws the views after opening a file.
	virtual void Serialize(CArchive& ar)
	{
		if (ar.IsStoring()) {
			ar.WriteCount(figures.size());
			for (auto* figure : *this)
				ar.WriteObject(figure);
		} else {
			auto newFigures = Load(ar);
			figures.insert(figures.end(), newFigures.begin(), newFigures.end());
		}
	}

	// Deletes all the figures without notifying the observers (used when MFC resets the document).
	void Clear()
	{
		for (auto* figure : *this)
			delete figure;
		figures.clear();
		highlightedFigure = nullptr;
	}

	// Adds 'count' random figures within the drawing area.
	void AddDummyData(size_t count)
	{
		auto newFigures = FigureHelper::GetRandomFigures(count, GetArea());
		figures.insert(figures.end(), newFigures.begin(), newFigures.end());

		Hint hint(Hint::Type::Added, newFigures);
		Update(hint);
	}

private:
	// Reads the number of figures followed by the figures, and returns them (the caller takes ownership).
	// If reading fails, the figures read so far are deleted and the exception is rethrown.
	static std::vector<Figure*> Load(CArchive& ar)
	{
		std::vector<Figure*> loadedFigures;
		try {
			const auto count = ar.ReadCount();
			for (DWORD_PTR counter = 0; counter < count; counter++) {
				if (auto* figure = static_cast<Figure*>(ar.ReadObject(RUNTIME_CLASS(Figure))))
					loadedFigures.push_back(figure);
			}
		} catch (CException*) {
			for (auto* figure : loadedFigures)
				delete figure;
			throw;
		}
		return loadedFigures;
	}

	// Unselects all the figures without notifying the observers.
	void ClearSelected()
	{
		for (auto* figure : figures)
			figure->Select(false);
	}

	Model(const Model& other) = delete;
	Model& operator=(const Model& other) = delete;
};
