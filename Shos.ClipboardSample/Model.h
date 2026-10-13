#pragma once

#include <afx.h>
#include <algorithm>
#include <utility>
#include <vector>
#include "Observer.h"
#include "Figure.h"

struct Hint : public CObject
{
	enum class Type
	{
		Added   ,
		Removed ,
		Changed ,
		ViewOnly
	};

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

class Model : public Observable<Hint>
{
	static constexpr LONG size = 2000L;

	std::vector<Figure*> figures;
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

	void Select(Figure& figure)
	{
		ClearSelected();
		figure.Select(true);
		Hint hint(Hint::Type::ViewOnly);
		Update(hint);
	}

	void Highlight(const Figure* figure)
	{
		highlightedFigure = figure;
	}

	const Figure* Highlight() const
	{
		return highlightedFigure;
	}

	void UnSelectAll()
	{
		ClearSelected();
		Hint hint(Hint::Type::ViewOnly);
		Update(hint);
	}

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

	void Clear()
	{
		for (auto* figure : *this)
			delete figure;
		figures.clear();
		highlightedFigure = nullptr;
	}

	void AddDummyData(size_t count)
	{
		auto newFigures = FigureHelper::GetRandomFigures(count, GetArea());
		figures.insert(figures.end(), newFigures.begin(), newFigures.end());

		Hint hint(Hint::Type::Added, newFigures);
		Update(hint);
	}

private:
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

	void ClearSelected()
	{
		for (auto* figure : figures)
			figure->Select(false);
	}

	Model(const Model& other) = delete;
	Model& operator=(const Model& other) = delete;
};
