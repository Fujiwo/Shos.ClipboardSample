#pragma once

#include "Model.h"
#include "Command.h"

class Document : public CDocument
{
	Model		   model;
	CommandManager commandManager;

public:
	//using iterator = Figure**;
	using iterator = Model::iterator;

	const CSize GetSize() const { return model.GetSize(); }
	const CRect GetArea() const { return model.GetArea(); }

	Document() : commandManager(model)
	{}
	
	//~Document() override
	//{
	//	RemoveAll();
	//}

	iterator begin() const
	{
		return model.begin();
		//return (iterator)(figures.GetData());
	}

	iterator end() const
	{
		return model.end();
		//return (iterator)figures.GetData() + figures.GetCount();
	}

	void Add(Figure* figure)
	{
		model.Add(figure);
		//figures.Add(figure);
		SetModifiedFlag();
	}

	void Draw(CDC& dc) const
	{
		for (auto figure : *this)
			figure->Draw(dc);
	}

	void OnMouseMove(UINT nFlags, CPoint point)
	{
		commandManager.OnMouseMove(nFlags, point);
	}

protected:
	virtual void Serialize(CArchive& ar)
	{
		model.Serialize(ar);
		//if (ar.IsStoring()) {
		//	ar.WriteCount(figures.GetCount());
		//	for (auto figure : *this)
		//		ar.WriteObject(figure);
		//}
		//else
		//{
		//	auto count = ar.ReadCount();
		//	for (DWORD_PTR counter = 0L; counter < count; counter++) {
		//		auto figure = STATIC_DOWNCAST(Figure, ar.ReadObject(NULL));
		//		if (figure != nullptr)
		//			figures.Add(figure);
		//	}
		//}
	}

	virtual void DeleteContents()
	{
		model.RemoveAll();
		CDocument::DeleteContents();
	}

	afx_msg void OnFigureRandom()
	{
		const size_t count = 1000;
		model.AddDummyData(count);
		UpdateAllViews(nullptr);
	}

private:
	//void RemoveAll()
	//{
	//	for (auto figure : *this)
	//		delete figure;
	//	figures.RemoveAll();
	//}

	//void AddDummyData(size_t count)
	//{
	//	FigureHelper::AddRandomFigures(figures, count, GetArea());
	//}

	DECLARE_DYNCREATE(Document)
	DECLARE_MESSAGE_MAP()
};
