#pragma once

#include "Figure.h"

class Document : public CDocument
{
	CTypedPtrArray<CObArray, Figure*> figures;

public:
	using iterator = Figure**;

	//Document()
	//{
	//}

	~Document() override
	{
		for (auto figure : *this)
			delete figure;
	}

	iterator begin()
	{
		return (iterator)(figures.GetData());
		//return figures.GetCount() == 0 ? nullptr : (iterator)(figures.GetData());
	}

	iterator end()
	{
		return (iterator)figures.GetData() + figures.GetCount();
		//return figures.GetCount() == 0 ? nullptr : (iterator)figures.GetData() + figures.GetCount();
	}

	void Add(Figure* figure)
	{
		figures.Add(figure);
	}
	
	DECLARE_DYNCREATE(Document)

	afx_msg void OnEditCopy();
	DECLARE_MESSAGE_MAP()
};
