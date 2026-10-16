#pragma once

#ifndef __AFXWIN_H__
	#error "Include 'pch.h' before including this file for PCH"
#endif

// The application object.
// Registers a single-document (SDI) template that ties Document, MainFrame and View together.
class Application : public CWinApp
{
public:
	// Called by MFC at startup. Creates the document template and the main window.
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};
