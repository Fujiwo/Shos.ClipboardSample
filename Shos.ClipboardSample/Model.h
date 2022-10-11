#pragma once

#include "Observer.h"
#include "Figure.h"

struct Hint
{
	enum class Type
	{
		Added,
		Removed,
		Changed,
	};

	std::vector<Figure*> figures;
	Type				 type;

	Hint(Type type, Figure* figure) : type(type)
	{
		figures.push_back(figure);
	}

	Hint(Type type, std::vector<Figure*> figures) : type(type), figures(figures)
	{}
};

class Model : public Observable<Hint>
{
	static const LONG size = 2000L;

	std::vector<Figure*>   figures;

public:
	//using iterator = Figure**;
	using iterator = std::vector<Figure*>::const_iterator;

	const CSize GetSize() const { return CSize(size, size); }
	const CRect GetArea() const { return CRect(CPoint(), GetSize()); }

	virtual ~Model()
	{
		RemoveAll();
	}

	iterator begin() const
	{
		return figures.begin();
		//return (iterator)(figures.GetData());
	}

	iterator end() const
	{
		return figures.end();
		//return (iterator)figures.GetData() + figures.GetCount();
	}

	void Add(Figure* figure)
	{
		figures.push_back(figure);
		Update(Hint(Hint::Type::Added, figure));
	}

	bool Change(Figure* oldFigure, Figure* newFigure)
	{
		auto iterator = std::find(figures.begin(), figures.end(), oldFigure);
		if (iterator == figures.end())
			return false;

		*iterator = newFigure;
		std::vector<Figure*> changedFigures = { oldFigure, newFigure };
		Update(Hint(Hint::Type::Changed, changedFigures));
	}

	//void Draw(CDC& dc) const
	//{
	//	for (auto figure : *this)
	//		figure->Draw(dc);
	//}

	virtual void Serialize(CArchive& ar)
	{
		if (ar.IsStoring()) {
			ar.WriteCount(figures.size());
			for (auto figure : *this)
				ar.WriteObject(figure);
		}
		else
		{
			auto count = ar.ReadCount();
			for (DWORD_PTR counter = 0L; counter < count; counter++) {
				auto figure = STATIC_DOWNCAST(Figure, ar.ReadObject(NULL));
				if (figure != nullptr)
					figures.push_back(figure);
			}
		}
	}

	//virtual void DeleteContents()
	//{
	//	RemoveAll();
	//	CDocument::DeleteContents();
	//}

	void RemoveAll()
	{
		for (auto figure : *this)
			delete figure;
		figures.clear();
	}

	void AddDummyData(size_t count)
	{
		FigureHelper::AddRandomFigures(figures, count, GetArea());
	}
};
