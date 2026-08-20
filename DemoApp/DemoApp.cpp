// DemoApp.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "DemoApp.h"
#include "DemoAppDlg.h"
#include "BatchScorer.h"
#include "Config.h"
#include "..\..\yUtils\MyWindows.h"
#include <cstdio>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDemoAppApp

BEGIN_MESSAGE_MAP(CDemoAppApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CDemoAppApp construction

CDemoAppApp::CDemoAppApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CDemoAppApp object

CDemoAppApp theApp;


// CDemoAppApp initialization

BOOL CDemoAppApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	bool bBatchMode = (__argc > 1);
	if (bBatchMode)
	{
		// IQV_tool.exe is a Windows-subsystem (GUI) app, so it has no console of its own and
		// printf output is otherwise invisible when run from a command prompt. Attach to
		// whichever console launched us so batch output actually shows up there.
		if (AttachConsole(ATTACH_PARENT_PROCESS))
		{
			FILE* pf = nullptr;
			freopen_s(&pf, "CONOUT$", "w", stdout);
			freopen_s(&pf, "CONOUT$", "w", stderr);
		}
	}

	CString sCommandLine = GetCommandLine();
    CMyWindows::SetApplicationPath (sCommandLine);
	gConfig.Init();

	if (bBatchMode)
	{
		// Batch mode: score every DICOM set found under __argv[1], headless (no dialog)
		if (!CMyWindows::IsDirectory(__argv[1]))
		{
			printf("Error: \"%s\" is not a directory\n", __argv[1]);
			return FALSE;
		}

		CBatchScorer batchScorer;
		batchScorer.RunOnDirTree(__argv[1]);
		return FALSE;
	}

	CDemoAppDlg dlg;
	m_pMainWnd = &dlg;
	CMyWindows::SetMainWindow(&dlg);
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
