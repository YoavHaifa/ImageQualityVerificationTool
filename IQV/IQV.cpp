// IQV.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "IQV.h"
#include "IQVDlg.h"
#include "BatchScorer.h"
#include "Optimizer.h"
#include "Config.h"
#include "..\..\yUtils\MyWindows.h"
#include <cstdio>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CIQVApp

BEGIN_MESSAGE_MAP(CIQVApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CIQVApp construction

CIQVApp::CIQVApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CIQVApp object

CIQVApp theApp;


// CIQVApp initialization

BOOL CIQVApp::InitInstance()
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
		CMyWindows::AttachToParentConsole();

	CString sCommandLine = GetCommandLine();
    CMyWindows::SetApplicationPath (sCommandLine);
	gConfig.Init();

	if (bBatchMode)
	{
		// Dev-only debug commands (not documented for end users): run the same Optimize menu
		// operations headlessly, against gConfig.msTrainingSetRoot, so their behavior can be
		// inspected from a console/debugger without going through the GUI.
		CString sArg1(__argv[1]);
		if (sArg1.CompareNoCase("-score-training-data") == 0)
		{
			COptimizer optimizer;
			int nScored = optimizer.RunOnTrainingSet(gConfig.msTrainingSetRoot.c_str());
			printf("Scored %d labeled case(s). Reports written to: %s\n",
				nScored, (LPCTSTR)optimizer.GetReportDir());
			return FALSE;
		}
		if (sArg1.CompareNoCase("-optimize-weights") == 0)
		{
			COptimizer optimizer;
			int nScored = optimizer.OptimizeWeights(gConfig.msTrainingSetRoot.c_str());
			printf("Optimized scorer weights from %d labeled case(s). Reports written to: %s\n",
				nScored, (LPCTSTR)optimizer.GetReportDir());
			return FALSE;
		}

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

	CIQVDlg dlg;
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
