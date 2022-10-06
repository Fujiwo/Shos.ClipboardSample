#pragma once

class View : public CView
{
	DECLARE_DYNCREATE(View)

public:
	Document* GetDocument() const
	{ return reinterpret_cast<Document*>(m_pDocument); }

	virtual void OnDraw(CDC* pDC) override
	{
		auto document = GetDocument();
		ASSERT_VALID(document);
		if (document == nullptr)
			return;

		AddDummyData();

		for (auto figure : *document)
			figure->Draw(*pDC);
	}

private:
	void AddDummyData()
	{
		auto document = GetDocument();
		ASSERT_VALID(document);
		if (document == nullptr)
			return;
		
		const size_t figureCount = 10;

		CRect clientRect;
		GetClientRect(&clientRect);

		for (auto count = 0; count < figureCount; count++)
			document->Add(new DotFigure(FigureHelper::RandomPosition(clientRect)));
	}
	
	DECLARE_MESSAGE_MAP()
};
