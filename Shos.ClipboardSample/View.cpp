#include "pch.h"
#include "framework.h"
#include "Document.h"
#include "View.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(View, CScrollView)

BEGIN_MESSAGE_MAP(View, CScrollView)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_WM_DESTROYCLIPBOARD()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()
