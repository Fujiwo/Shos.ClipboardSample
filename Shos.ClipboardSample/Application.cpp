#include "pch.h"
#include "framework.h"
#include "Application.h"
#include "MainFrame.h"
#include "Document.h"
#include "View.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(Application, CWinApp)
	ON_COMMAND(ID_FILE_NEW, &CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinApp::OnFileOpen)
END_MESSAGE_MAP()

BOOL Application::InitInstance()
{
	CWinApp::InitInstance();

	// The application takes ownership of the template.
	auto pDocTemplate = new CSingleDocTemplate(IDR_MAINFRAME, RUNTIME_CLASS(Document), RUNTIME_CLASS(MainFrame), RUNTIME_CLASS(View));
	AddDocTemplate(pDocTemplate);

	// Handles the command line (with no arguments, this creates a new empty document and the main window).
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	return TRUE;
}

// The one and only application object (required by MFC).
Application theApp;
