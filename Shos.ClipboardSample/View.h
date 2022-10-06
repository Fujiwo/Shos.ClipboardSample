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

		const size_t figureCount = 10;
		document->AddDummyData(figureCount);
		document->Draw(*pDC);
	}
	
	DECLARE_MESSAGE_MAP()
};
