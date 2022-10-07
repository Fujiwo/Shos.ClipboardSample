#pragma once

#include "Figure.h"

class Document : public CDocument
{
    static const LONG size = 2000L;
	CArray<Figure*> figures;

public:
	using iterator = Figure**;

    const CSize GetSize() const { return CSize(size, size); }
    const CRect GetArea() const { return CRect(CPoint(), GetSize()); }

	~Document() override
	{
		RemoveAll();
	}

	iterator begin() const
	{
		return (iterator)(figures.GetData());
	}

	iterator end() const
	{
		return (iterator)figures.GetData() + figures.GetCount();
	}

	void Add(Figure* figure)
	{
		figures.Add(figure);
		SetModifiedFlag();
	}

    void Draw(CDC& dc) const
    {
		for (auto figure : *this)
			figure->Draw(dc);
	}

protected:
	virtual void Serialize(CArchive& ar)
	{
		if (ar.IsStoring()) {
			ar.WriteCount(figures.GetCount());
			for (auto figure : *this)
				ar.WriteObject(figure);
		}
		else
		{
			auto count = ar.ReadCount();
			for (DWORD_PTR counter = 0L; counter < count; counter++) {
				auto figure = STATIC_DOWNCAST(Figure, ar.ReadObject(NULL));
				if (figure != nullptr)
					figures.Add(figure);
			}
		}
	}
	
	virtual void DeleteContents()
	{
		RemoveAll();
		CDocument::DeleteContents();
	}

	afx_msg void OnFigureRandom()
	{
		const size_t count = 1000;
		AddDummyData(count);
		UpdateAllViews(nullptr);
	}

private:
	void RemoveAll()
	{
		for (auto figure : *this)
			delete figure;
		figures.RemoveAll();
	}

	void AddDummyData(size_t count)
	{
		FigureHelper::AddRandomFigures(figures, count, GetArea());
	}

	DECLARE_DYNCREATE(Document)
	DECLARE_MESSAGE_MAP()
};
