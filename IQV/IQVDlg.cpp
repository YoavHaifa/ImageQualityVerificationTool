// IQVDlg.cpp : implementation file
//

#include "stdafx.h"
#include "IQV.h"
#include "IQVDlg.h"
#include "ArinetaImages.h"
#include "RingsScorer.h"
#include "IQVManager.h"
#include "BatchScorer.h"
#include "BatchCompleteDlg.h"
#include "DataDownloader.h"
#include "CaseReviewer.h"
#include "BatchReviewer.h"
#include "ImageScore.h"
#include "Config.h"

#include "..\..\yUtils\MyMath.h"
#include "..\..\yUtils\MyFileDialog.h"
#include "..\..\yUtils\MyFolderDialog.h"
#include "..\..\yUtils\FilesList.h"

#include "..\..\ImageRLib\ImageRIF.h"
#include "..\..\ImageRLib\DataRoi.h"
#include "..\..\ImageRLib\Rle1read.h"
#include "..\..\ImageRLib\DataFiles.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CIQVDlg* gpDlg = nullptr;

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CIQVDlg dialog


CIQVDlg::CIQVDlg(CWnd* pParent /*=NULL*/)
	: CMyDialogEx(CIQVDlg::IDD, pParent)
	, mpImageRIF(NULL)
	, mpSharedVolume(NULL)
	, mnImagesInRow(10)
	, mnImageRows(16)
	, mnImageLines(512)
	, mnImageCols(640)
	, miPos(0)
	, miPos2d(0)
	, mbDisplayReadyImages(false)
	, mpImages(NULL)
	, mpIQVManager(NULL)
	, mpCaseReviewer(NULL)
	, mpBatchReviewer(NULL)
	, mpDataFiles(NULL)
	, mpColors(NULL)
	, mMyViewerOffsetX(100)
	, mMyViewerOffsetY(250)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	gfLog.Log("<CIQVDlg::CIQVDlg>");
}
CIQVDlg::~CIQVDlg(void)
{
	gfLog.Log("<CIQVDlg::~CIQVDlg>");
	if (mpIQVManager)
		delete mpIQVManager;
	if (mpCaseReviewer)
		delete mpCaseReviewer;
	if (mpBatchReviewer)
		delete mpBatchReviewer;
	if (mpImageRIF)
		delete mpImageRIF;
	if (mpSharedVolume)
		delete mpSharedVolume;
	if (mpColors)
		delete mpColors;

	while (!mImages.IsEmpty())
	{
		CArchivesImages* pImages = mImages.GetTail();
		mImages.RemoveTail();
		if (pImages != mpImages) // mpImages is owned and deleted by mpIQVManager/mpCaseReviewer/mpBatchReviewer
			delete pImages;
	}
}
void CIQVDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CIQVDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_SHARED, &CIQVDlg::OnBnClickedButtonShared)
	ON_BN_CLICKED(IDC_BUTTON_ADD_ROI, &CIQVDlg::OnBnClickedButtonAddRoi)
	ON_BN_CLICKED(IDC_BUTTON_UP_POS, &CIQVDlg::OnBnClickedButtonUpPos)
	ON_COMMAND(ID_FILE_OPEN32771, &CIQVDlg::OnFileOpen32771)
	ON_COMMAND(ID_FILE_BATCHSCORING, &CIQVDlg::OnFileBatchscoring)
	ON_COMMAND(ID_FILE_OPENCASESCORING, &CIQVDlg::OnFileOpencasescoring)
	ON_COMMAND(ID_FILE_OPENBATCHSCORING, &CIQVDlg::OnFileOpenbatchscoring)
	ON_COMMAND(ID_TEST_FINDDICOMSETS, &CIQVDlg::OnTestFinddicomsets)
	ON_COMMAND(ID_UTILS_DOWNLOADDATA, &CIQVDlg::OnUtilsDownloaddata)
	ON_COMMAND(ID_FILE_EXIT, &CIQVDlg::OnFileExit)
	ON_BN_CLICKED(IDC_BUTTON_ADD_COLORS, &CIQVDlg::OnBnClickedButtonAddColors)
	ON_BN_CLICKED(IDC_OK, &CIQVDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_CANCEL, &CIQVDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_MAX, &CIQVDlg::OnBnClickedButtonMax)
	ON_BN_CLICKED(IDC_BUTTON_NEXT, &CIQVDlg::OnBnClickedButtonNext)
	ON_BN_CLICKED(IDC_BUTTON_PREV, &CIQVDlg::OnBnClickedButtonPrev)
	ON_CBN_SELCHANGE(IDC_COMBO_SCORE_TYPE, &CIQVDlg::OnCbnSelchangeComboScoreType)
	ON_BN_CLICKED(IDC_BUTTON_WORST_CASE, &CIQVDlg::OnBnClickedButtonWorstCase)
	ON_BN_CLICKED(IDC_BUTTON_NEXT_CASE, &CIQVDlg::OnBnClickedButtonNextCase)
	ON_BN_CLICKED(IDC_BUTTON_PREV_CASE, &CIQVDlg::OnBnClickedButtonPrevCase)
END_MESSAGE_MAP()


// CIQVDlg message handlers

BOOL CIQVDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString sTitle;
	sTitle.Format("Image Quality Verification App v%s", gConfig.msVersion.c_str());
	SetWindowText(sTitle);

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	CWnd *pWnd = GetDlgItem(IDC_STATIC_STATUS);
	if (pWnd)
		CMyWindows::SetStatusWindow(pWnd);
	CWnd *pBatchStatusWnd = GetDlgItem(IDC_STATIC_BATCH_STATUS);
	if (pBatchStatusWnd)
		CMyWindows::SetStatusWindow1(pBatchStatusWnd);

	CComboBox* pComboScoreType = (CComboBox*)GetDlgItem(IDC_COMBO_SCORE_TYPE);
	if (pComboScoreType)
	{
		for (int i = 0; i < (int)EScoreType::N_SCORE_TYPES; i++)
			pComboScoreType->AddString(ScoreTypeName((EScoreType)i));
		pComboScoreType->SetCurSel((int)gConfig.mScoreType);
	}

	// TODO: Add extra initialization here
	//DisplayPos();

	if (!gConfig.mbDeveloperMode)
	{
		CMenu* pMenu = GetMenu();
		if (pMenu)
		{
			for (int i = pMenu->GetMenuItemCount() - 1; i >= 0; i--)
			{
				CString sLabel;
				pMenu->GetMenuString(i, sLabel, MF_BYPOSITION);
				if (sLabel != "File" && sLabel != "Utils")
					pMenu->RemoveMenu(i, MF_BYPOSITION);
			}
			DrawMenuBar();
		}
	}

	gpDlg = this;
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CIQVDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CIQVDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}
LRESULT CIQVDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	char zBuf[128];
	if (mpImageRIF)
    {
		CGraphicElement *pGE = NULL;
		bool bChange = false;
		if (mpImageRIF->GetGraphicMessage(message, wParam, lParam, bChange))
		{
			//gCardiac.OnGraphicUpdate();
			sprintf_s (zBuf,sizeof(zBuf), "<WindowProc> Graphic Messgae %d %d", (int)wParam, (int)lParam);
			gConfig.PrintStatus(zBuf);
		    return 0;
		}
		else if (mpImageRIF->GetGraphicActiveMessage(message, &pGE))
		{
			//gCardiac.OnGraphicUpdate();
			sprintf_s (zBuf,sizeof(zBuf), "<WindowProc> Graphic Active Messgae %d %d - %s", 
				(int)wParam, (int)lParam, (const char *)pGE->Name());
			gConfig.PrintStatus(zBuf);
		    return 0;
		}
        else if (message == mpImageRIF->mImageRKeyboardMsg)
	    {
			sprintf_s (zBuf,sizeof(zBuf), "<WindowProc> Keyboard Messgae %c", 
				(int)wParam);
			gConfig.PrintStatus(zBuf);
		    return 0;
		}
        else if (message == mpImageRIF->mImageRSaveDoneMsg)
	    {
			//gCardiac.OnViewerState();
		    return 0;
	    }
        else if (message == mpImageRIF->mImageRWindowMsg)
	    {
			char zBuf[128];
			sprintf_s(zBuf,sizeof(zBuf),"Window center %d width %d", (int)wParam, (int)lParam);
			CMyWindows::MessBox(zBuf,"Window from MyViewer");
		    return 0;
	    }
        else if (mpImageRIF->GetPositionMessage(message, wParam, lParam, bChange))
        {
			if (bChange)
			{
				//gfLog.Printf("<WindowProc> message %u, wParam %d, lParam %d", message, (int)wParam, (int)lParam);
				if (message == CPosition::umaPositionMsg[0])
					gfLog.Printf("<WindowProc> PositionMsg 0 ID %d pos %d",  (int)lParam, (int)wParam);
				else if (message == CPosition::umaPositionMsg[1])
					gfLog.Printf("<WindowProc> PositionMsg 1 ID %d pos %d", (int)lParam, (int)wParam);
				gfLog.Printf("<WindowProc> Pos %3d\n", miPos);

				//DisplayPos();
				sprintf_s (zBuf,sizeof(zBuf), "<WindowProc> Position Messgae %d %d", (int)wParam, (int)lParam);
				gConfig.PrintStatus(zBuf);
				if (mpImages)
				{
					CPosition *pPosition = mpImages->GetPosition();
					if (miPos >= pPosition->miFirst && miPos <= pPosition->miLast)
						mpImages->SetCurrent(miPos);
				}

				if (mpRingsScorer)
					DisplayScore();
			}
		    return 0;
        }
		else if (message == mpImageRIF->mImageRCursorLocationMsg)
		{
			sprintf_s (zBuf,sizeof(zBuf), "<WindowProc> Cursor Messgae %d %d", (int)wParam, (int)lParam);
			gConfig.PrintStatus(zBuf);
		}
		else if (message == mpImageRIF->mImageRLeftMouseUpMsg)
		{
			gConfig.PrintStatus("<WindowProc> Left Mouse Up");
		}
    }
	return CDialog::WindowProc(message, wParam, lParam);
}
void CIQVDlg::OnOK()
{
	static int i = 0;
	i++;
	OnBnClickedButtonUpPos();
}
// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CIQVDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
void CIQVDlg::OnBnClickedButtonShared()
{
	LoadViewer();

	if (!mpSharedVolume)
	{
		mpSharedVolume = new CTSharedImage<short>("SharedVolume");
		mpSharedVolume->SetVolumeSize(mnImageLines,mnImageCols,0,mnImageRows*mnImagesInRow-1);
		mpSharedVolume->mpHeader->mnImagesInRow = mnImagesInRow;
		mpSharedVolume->SetZoomName("SharedZoom");

		strcpy_s(mpSharedVolume->mpHeader->mzPositionName, sizeof(mpSharedVolume->mpHeader->mzPositionName), "Shared2d");
		InitSharedVolume();
		mpImageRIF->DisplayShared(mpSharedVolume);
	}
}
void CIQVDlg::LoadViewer(void)
{
	if (mpImageRIF)
		return;

	mpImageRIF = new CImageRIF(0, 0, false, mMyViewerOffsetX, mMyViewerOffsetY);
	mpImageRIF->SetTitle("Image Quality Verification Viewer");
	mpImageRIF->SetViewerBroadPos();
	mpImageRIF->SetIndicesToUpdatePosition2d(miPos,miPos2d);
}
bool CIQVDlg::LoadViewerWithImages(const char *zName)
{
	if (mpImageRIF)
		return false;

	mpImageRIF = new CImageRIF(0,0,false, mMyViewerOffsetX, mMyViewerOffsetY);
	mpImageRIF->SetTitle("Image Quality Verification Viewer");
	mpImageRIF->FileOpen(zName);
	mpImageRIF->SetViewerBroadPos();
	mpImageRIF->SetIndicesToUpdatePosition2d(miPos,miPos2d);
	mbDisplayReadyImages = true;
	return true;
}
void CIQVDlg::InitSharedVolume(void)
{
	FILE *pfLog = CMyWindows::FOpenLogFile("InitSharedVolume");
	mpSharedVolume->Zero();
	short *pLast = NULL;
	for (int iRow = 0; iRow < mnImageRows; iRow++)
	{
		if (pfLog)
			fprintf (pfLog, "iRow %d\n\n", iRow);

		for (int iImage = 0; iImage < mnImagesInRow; iImage++)
		{
			int iAbsIm = iRow*mnImagesInRow+iImage;
			short *pImData = mpSharedVolume->GetImageStart(iAbsIm);
			if (pfLog)
				fprintf (pfLog, "iImage %3d  iAbs %3d p %8p (%8p)\n", 
					iImage, iAbsIm, pImData, (void *)(pImData - pLast));
			pLast = pImData;

			int lastLineToFill = mnImageLines - 20 - iRow * 10;
			for (int iLine = 10; iLine < lastLineToFill; iLine++)
			{
				short *pLine = pImData + iLine * mnImageCols;
				int firstFill = iImage * 4 + 200;
				int lastFill = firstFill + 200;
				short value = 1000 + iRow * 20;

				for (int iCol = firstFill; iCol <= lastFill; iCol++)
					pLine[iCol] = value++;
			}
		}
	}
	if (pfLog)
		fclose(pfLog);
}
void CIQVDlg::OnBnClickedButtonAddRoi()
{
	if (!mpImageRIF)
		return;

	static int count = 0;
	count++;
	char zName[128];
	sprintf_s(zName,"DemoRoi%d_%d_%d", count,miPos+1,miPos2d+1);
	CDataRoi *pRoi = new CDataRoi(NULL, zName, 0xff0080);
	CDataCoordinates center (mnImageCols/2.0f, mnImageLines/2.0f);
	pRoi->InitEllipse(center, 10.0f+count);
	pRoi->LockToPosition2d(miPos, miPos2d);
	pRoi->SetSaveToXml(true);
	pRoi->mbReportClientOnActivation = true;
	mpImageRIF->DisplayGraphic(pRoi);
}
//void CIQVDlg::DisplayPos(void)
//{
//	CWnd *pWnd = GetDlgItem(IDC_EDIT_POS1);
//	if (pWnd)
//	{
//		char zBuf[128];
//		int iPos = miPos;
//		if (!mbDisplayReadyImages)
//			iPos++;
//		sprintf_s(zBuf,128,"%d",iPos);
//		pWnd->SetWindowText(zBuf);
//	}
//	pWnd = GetDlgItem(IDC_EDIT_POS2);
//	if (pWnd)
//	{
//		char zBuf[128];
//		sprintf_s(zBuf,128,"%d",miPos2d+1);
//		pWnd->SetWindowText(zBuf);
//	}
//}
void CIQVDlg::GetPos1(void)
{
	CWnd *pWnd = GetDlgItem(IDC_EDIT_POS1);
	if (pWnd)
	{
		CString sText;
		pWnd->GetWindowText(sText);
		miPos = atoi(sText);
		if (!mbDisplayReadyImages)
		{
			miPos--;
			CMyMath::Clip(0, miPos, mnImagesInRow - 1);
		}
	}
	else
		miPos = 0;
}
void CIQVDlg::OnBnClickedButtonUpPos()
{
	GetPos1();
	CWnd *pWnd = GetDlgItem(IDC_EDIT_POS2);
	if (pWnd)
	{
		CString sText;
		pWnd->GetWindowText(sText);
		miPos2d = atoi(sText) - 1;
		CMyMath::Clip(0, miPos2d, mnImageRows-1);
		if (mpImageRIF)
		{
			if (mpSharedVolume)
			{
				mpImageRIF->SetPosition2d(mpSharedVolume->Name(), 
					miPos2d, miPos);
			}
			else if (mpImages)
			{
				mpImageRIF->SetPosition(mpImages->GetPatternName(), miPos);
			}
			gfLog.Printf("<OnBnClickedButtonUpPos> (%3d %3d)\n", miPos, miPos2d);
		}
	}
}
void CIQVDlg::OnFileOpen32771()
{
	CMyFileDialog dlg(CMyFileDialog::FD_OPEN,"Open Dicom Images for Display","d:\\Cirs_Images");
	if (dlg.DoModal())
	{
		CString sImageName(dlg.m_ofn.lpstrFile);
		if (!CMyDicom::IsDicom(sImageName))
		{
			CMyWindows::MessBox("Expecting DICOM images here...", "Warning");
		}
		else
		{
			if (!mpImageRIF)
			{
				// Score first (headless - no viewer needed for this), so we know which image
				// is actually worth looking at before ever opening the viewer
				mpIQVManager = new CIQVManager();
				mpIQVManager->LoadAndScore(sImageName);

				mpImages = mpIQVManager->GetImages();
				mpRingsScorer = mpIQVManager->GetRingsScorer();
				miPos = mpIQVManager->GetScoredPosition();

				// Open the viewer directly on the target (worst-score) image - its one-time
				// window/level auto-fit is keyed off whichever image is displayed first, and
				// an arbitrary early image (e.g. the first) is often too sparse to fit well
				CString sTargetFile = mpImages->GetName(miPos);
				if (LoadViewerWithImages(sTargetFile))
				{
					mImages.AddTail(mpImages);
					mpImageRIF->DisplayShared(mpImages->GetSharedWideVolume());
					// Only ever populated by live scoring (not Case/Batch Review), so still
					// check for null even with the feature on
					if (mpImages->GetSharedCtPerRadiusVolume())
						mpImageRIF->DisplayShared(mpImages->GetSharedCtPerRadiusVolume());
					OnCurrentSelectedByScorer(miPos);
				}
			}
			else // Add Extra Images' series
			{
				if (mImages.GetSize() == 1)
					mpImageRIF->SetNColumns(2); // Do it once
				CArchivesImages* pNewImages = new CArchivesImages(sImageName);
				mImages.AddTail(pNewImages);
				mpImageRIF->FileOpen(sImageName);
			}
		}
	}
}
void CIQVDlg::OnFileBatchscoring()
{
	CMyFolderDialog dlg("Select Root Directory for Batch Scoring");
	if (!dlg.DoModal())
		return;

	CBatchScorer batchScorer;
	int nScored = batchScorer.RunOnDirTree(dlg.msFolderName);

	CString sMsg;
	sMsg.Format("Batch scoring complete.\n%d case(s) scored.\n\nBatch data root: %s\nBatch result root: %s",
		nScored, (LPCTSTR)dlg.msFolderName, (LPCTSTR)batchScorer.GetLogDir());

	// Offer to jump straight into reviewing what was just scored - the results are already on
	// disk at batchScorer.GetLogDir(), so no need to ask the user to pick it again.
	CBatchCompleteDlg completeDlg(sMsg, this);
	if (completeDlg.DoModal() != IDOK || mpImageRIF)
		return;

	mpBatchReviewer = new CBatchReviewer();
	if (!mpBatchReviewer->Init(batchScorer.GetLogDir()))
		return;

	DisplayBatchCase();
}
void CIQVDlg::OnFileOpencasescoring()
{
	if (mpImageRIF)
		return;

	mpCaseReviewer = new CCaseReviewer();
	if (!mpCaseReviewer->SelectCase())
		return;

	// Score first (headless - no viewer needed for this), so we know which image is
	// actually worth looking at before ever opening the viewer
	if (!mpCaseReviewer->LoadCase())
		return;

	CIQVManager* pManager = mpCaseReviewer->GetManager();
	mpImages = pManager->GetImages();
	mpRingsScorer = pManager->GetRingsScorer();
	miPos = pManager->GetScoredPosition();

	// Open the viewer directly on the target (worst-score) image - its one-time window/level
	// auto-fit is keyed off whichever image is displayed first, and an arbitrary early image
	// (e.g. the case's sample file) is often too sparse to fit well
	CString sTargetFile = mpImages->GetName(miPos);
	if (LoadViewerWithImages(sTargetFile))
	{
		mImages.AddTail(mpImages);
		mpImageRIF->DisplayShared(mpImages->GetSharedWideVolume());
		// Only ever populated by live scoring (not Case/Batch Review), so check for null
		// even with the feature on
		if (mpImages->GetSharedCtPerRadiusVolume())
			mpImageRIF->DisplayShared(mpImages->GetSharedCtPerRadiusVolume());
		OnCurrentSelectedByScorer(miPos);
	}
}
void CIQVDlg::OnFileOpenbatchscoring()
{
	if (mpImageRIF)
		return;

	mpBatchReviewer = new CBatchReviewer();
	if (!mpBatchReviewer->Init())
		return;

	DisplayBatchCase();
}
void CIQVDlg::OnBnClickedButtonWorstCase()
{
	if (!mpBatchReviewer)
		return;
	if (mpBatchReviewer->DisplayWorstCase())
		DisplayBatchCase();
}
void CIQVDlg::OnBnClickedButtonNextCase()
{
	if (!mpBatchReviewer)
		return;
	if (mpBatchReviewer->DisplayNextCase())
		DisplayBatchCase();
}
void CIQVDlg::OnBnClickedButtonPrevCase()
{
	if (!mpBatchReviewer)
		return;
	if (mpBatchReviewer->DisplayPrevCase())
		DisplayBatchCase();
}
void CIQVDlg::DisplayBatchCase()
{
	CIQVManager* pManager = mpBatchReviewer->GetManager();
	mpImages = pManager->GetImages();
	mpRingsScorer = pManager->GetRingsScorer();
	miPos = pManager->GetScoredPosition();

	CString sCaseIndex;
	sCaseIndex.Format("Case %d of %d", mpBatchReviewer->GetCurrentRank(), mpBatchReviewer->GetNumCases());
	SetDlgItemText(IDC_STATIC_CASE_INDEX, sCaseIndex);

	// Same one-shot-good-window-fit reasoning as OnFileOpencasescoring: show the target
	// (worst-score) image, not an arbitrary one
	CString sTargetFile = mpImages->GetName(miPos);

	// The viewer accumulates displays rather than replacing them, so leafing to a new case
	// needs a fresh viewer instead of reusing the current one
	if (mpImageRIF)
	{
		delete mpImageRIF;
		mpImageRIF = nullptr;
	}

	if (!LoadViewerWithImages(sTargetFile))
		return;

	mpImageRIF->DisplayShared(mpImages->GetSharedWideVolume());
	// Only ever populated by live scoring (not Case/Batch Review), so check for null even
	// with the feature on
	if (mpImages->GetSharedCtPerRadiusVolume())
		mpImageRIF->DisplayShared(mpImages->GetSharedCtPerRadiusVolume());
	OnCurrentSelectedByScorer(miPos);
}
void CIQVDlg::OnTestFinddicomsets()
{
	CMyFolderDialog dlg("Select Root Directory for Sample Files Scan");
	if (!dlg.DoModal())
		return;

	CFilesList list;
	int n = CMyWindows::ListSampleFilesInDirTree(dlg.msFolderName, gConfig.msDicomFilePattern.c_str(), list);

	CString sfLogName(gConfig.msLogRoot.c_str());
	sfLogName += "\\SampleFilesScan.txt";
	list.Print(sfLogName, "Sample files found", true);

	CString sMsg;
	sMsg.Format("Found %d sample file(s) under:\n%s\n\nList written to:\n%s",
		n, (LPCTSTR)dlg.msFolderName, (LPCTSTR)sfLogName);
	CMyWindows::MessBox(sMsg, "Sample Files Scan");
}
void CIQVDlg::OnUtilsDownloaddata()
{
	CMyFolderDialog dlg("Select Root Directory to Download From", gConfig.msDownloadDefaultSource.c_str());
	if (!dlg.DoModal())
		return;

	CDataDownloader downloader;
	int nCopied = downloader.DownloadFromRoot(dlg.msFolderName);

	CString sMsg;
	sMsg.Format("Copied %d image set(s) (under \"%s\" directories) from:\n%s\n\nTo:\n%s",
		nCopied, gConfig.msDownloadDirNameFilter.c_str(), (LPCTSTR)dlg.msFolderName, gConfig.msDataRoot.c_str());
	CMyWindows::MessBox(sMsg, "Download Data");
}
void CIQVDlg::OnFileExit()
{
	OnCancel();
}
void CIQVDlg::OnBnClickedButtonAddColors()
{
	if (!mpImages && !mpSharedVolume)
		return;

	static int nLines = 0;
	static int nCols = 0;
	if (!mpColors)
	{
		mpColors = new CTSharedImage<COLORREF>("ColorsOverlay");
		if (mpImages)
		{
			nLines = mpImages->GetNLines();
			nCols = mpImages->GetNCols();
			mpColors->SetSize(nLines, nCols);
			mpColors->SetPattern(mpImages->GetPatternName());
		}
		else if (mpSharedVolume)
		{
			nLines = mpSharedVolume->GetNLines();
			nCols = mpSharedVolume->GetNCols();
			mpColors->SetSize(nLines, nCols);
			mpColors->SetPattern(mpSharedVolume->Name());
		}
	}

	mpColors->StartWrite();
	if (mpImages)
		mpColors->SetIndex(mpImages->GetCurrentPosition());
	else
		mpColors->SetIndex(miPos);

	static int count = 0;
	count++;
	COLORREF color;
	switch(count % 3)
	{
	case 0:
		color = 0xff;
		break;
	case 1:
		color = 0xff00;
		break;
	case 2:
		color = 0xff0000;
		break;
	}

	mpColors->Zero();
	for (int iLine = nLines / 4; iLine <= nLines / 2; iLine++)
	{
		COLORREF *pLine = mpColors->GetLine(iLine);
		for (int iCol = nCols/4; iCol <= nCols / 2; iCol++)
		{
			pLine[iCol] = color;
		}
	}
	mpColors->EndWrite();
	
	mpImageRIF->DisplayShared(mpColors);
}
void CIQVDlg::DisplayCircle(CDataCoordinates& center, float radius)
{
	static int count = 0;
	//if (count > 0)
	//	return;
	count++;

	//char zName[128];
	//sprintf_s(zName, "CenteredCircle", count, miPos + 1, miPos2d + 1);
	CDataRoi* pCircle = new CDataRoi(NULL, "CenteredCircle", RGB(255, 255, 0)); // bright yellow - visible against grayscale CT
	pCircle->InitEllipse(center, max(radius, 5.0f)); // otherwise too small to see on screen
	pCircle->SetCircle();
	pCircle->SetFixedCenter();
	//pCircle->LockToPosition2d(miPos, miPos2d);
	pCircle->SetSaveToXml(true);
	pCircle->mbReportClientOnActivation = true;
	mpImageRIF->SetCurrentDR(0);
	mpImageRIF->DisplayGraphic(pCircle);
}
void CIQVDlg::OnCurrentSelectedByScorer(int iPos)
{
	miPos = iPos;
	mpImages->SetCurrent(miPos);
	mpImageRIF->SetPosition(mpImages->GetPatternName(), miPos);
	DisplayScore();
}
void CIQVDlg::DisplayScore()
{
	const CImageScore& score = mpRingsScorer->ScoreCurrentImage(miPos);
	SetParameter(IDC_EDIT_I_IMAGE, miPos);
	SetParameter(IDC_EDIT_SCORE, score.mScore);
	SetParameter(IDC_EDIT_RADIUS, score.miRing);
	if (score.miRing >= 0)
		DisplayCircle(mpImages->GetRotationCenter(), (float)score.miRing);
}
void CIQVDlg::OnBnClickedOk()
{
	gConfig.SaveToFile();
	CMyDialogEx::OnOK();
}
void CIQVDlg::OnBnClickedCancel()
{
	CMyDialogEx::OnCancel();
}
void CIQVDlg::OnBnClickedButtonMax()
{
	mpRingsScorer->DisplayMaxPeak();
}
void CIQVDlg::OnBnClickedButtonNext()
{
	mpRingsScorer->DisplayNextPeak();
}
void CIQVDlg::OnBnClickedButtonPrev()
{
	mpRingsScorer->DisplayPrevPeak();
}
void CIQVDlg::OnCbnSelchangeComboScoreType()
{
	CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_SCORE_TYPE);
	int iSel = pCombo ? pCombo->GetCurSel() : -1;
	if (iSel < 0)
		return;

	gConfig.mScoreType = (EScoreType)iSel;
	gConfig.SaveToFile();

	if (mpBatchReviewer)
	{
		// In batch review, "active scorer changed" means jump to the worst case for the
		// newly selected scorer (LoadCaseAtRank always keys off gConfig.mScoreType, so this
		// also correctly repositions within the case if it turns out to be the same one)
		if (mpBatchReviewer->DisplayWorstCase())
			DisplayBatchCase();
	}
	else if (mpRingsScorer)
	{
		mpRingsScorer->OnActiveScoreTypeChanged();
	}
}
