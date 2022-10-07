#pragma once

#include "Figure.h"

class Document : public CDocument
{
    DECLARE_DYNCREATE(Document)
        
    CArray<Figure*> figures;

    const LONG size = 2000L;

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

	void RemoveAll()
	{
		for (auto figure : *this)
			delete figure;
		figures.RemoveAll();
	}

    void Draw(CDC& dc) const
    {
		//DrawArea(dc, GetSysColor(COLOR_WINDOW));
		DrawFigures(dc);
    }

	//void DrawArea(CDC& dc, COLORREF color) const
	//{
	//	dc.FillSolidRect(GetArea(), color);
	//}

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
				Figure* figure = (Figure*)ar.ReadObject(NULL);
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
	void AddDummyData(size_t count)
	{
		FigureHelper::AddRandomFigures(figures, count, GetArea());
	}

	void DrawFigures(CDC& dc) const
	{
		for (auto figure : *this)
			figure->Draw(dc);
	}

    DECLARE_MESSAGE_MAP()
};
