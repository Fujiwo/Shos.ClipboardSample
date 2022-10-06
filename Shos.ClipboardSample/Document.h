#pragma once

#include "Figure.h"
#include "ClipboardHelper.h"

class Document : public CDocument
{
    DECLARE_DYNCREATE(Document)
        
    CTypedPtrArray<CObArray, Figure*> figures;

    const LONG size = 2000L;

public:
	using iterator = Figure**;

    const CSize GetSize() const { return CSize(size, size); }
    const CRect GetArea() const { return CRect(CPoint(), GetSize()); }

	~Document() override
	{
		for (auto figure : *this)
			delete figure;
	}

	iterator begin()
	{
		return (iterator)(figures.GetData());
	}

	iterator end()
	{
		return (iterator)figures.GetData() + figures.GetCount();
	}

	void Add(Figure* figure)
	{
		figures.Add(figure);
	}

    void Draw(CDC& dc)
    {
        for (auto figure : *this)
            figure->Draw(dc);
    }

    void AddDummyData(size_t count)
    {
		FigureHelper::AddRandomFigures(figures, count, GetArea());
    }

    afx_msg void OnEditCopy()
    {
        ClipboardHelper::CopyToClipboard(GetSize(), [&](CDC& dc) { Draw(dc); });
    }

    DECLARE_MESSAGE_MAP()
};
