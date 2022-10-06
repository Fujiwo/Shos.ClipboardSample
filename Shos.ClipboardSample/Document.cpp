#include "pch.h"
#include "framework.h"
#include "Document.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(Document, CDocument)

BEGIN_MESSAGE_MAP(Document, CDocument)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
END_MESSAGE_MAP()

//void Document::OnEditCopy()
//{
//	// TODO: ここにコマンド ハンドラー コードを追加します。
//}
