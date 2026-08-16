// DemoAppDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DemoApp.h"
#include "DemoAppDlg.h"
#include "ArinetaImages.h"
#include "RingsScorer.h"
#include "ImageScore.h"
#include "Config.h"

#include "..\..\yUtils\MyMath.h"
#include "..\..\yUtils\MyFileDialog.h"
#include "..\..\yUtils\NameGetDialog.h"
#include "..\..\yUtils\MyProgress.h"
#include "..\..\yUtils\MyDicomWriter.h"
#include "..\..\yUtils\UserTextDialog.h"

#include "..\..\ImageRLib\ImageRIF.h"
#include "..\..\ImageRLib\DataRoi.h"
//#include "..\..\ImageRLib\ArchivesImages.h"
#include "..\..\ImageRLib\Smoother.h"
#include "..\..\ImageRLib\Zoomer.h"
#include "..\..\ImageRLib\Rle1read.h"
#include "..\..\ImageRLib\DataFiles.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CDemoAppDlg* gpDlg = nullptr;

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


// CDemoAppDlg dialog


CDemoAppDlg::CDemoAppDlg(CWnd* pParent /*=NULL*/)
	: CMyDialogEx(CDemoAppDlg::IDD, pParent)
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
	, mpDataFiles(NULL)
	, mpSmoothed(NULL)
	, mpColors(NULL)
	, mpSmoother(NULL)
	, msAppendToPatientName(_T(""))
	, mLowFactor(0.95f)
	, mHighFactor(1.0f)
	, mMyViewerOffsetX(100)
	, mMyViewerOffsetY(250)
	, mbColormapOn(false)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	gfLog.Log("<CDemoAppDlg::CDemoAppDlg>");
}
CDemoAppDlg::~CDemoAppDlg(void)
{
	gfLog.Log("<CDemoAppDlg::~CDemoAppDlg>");
	if (mpImageRIF)
		delete mpImageRIF;
	if (mpSmoother)
		delete mpSmoother;
	if (mpSmoothed)
		delete mpSmoothed;
	if (mpSharedVolume)
		delete mpSharedVolume;
	if (mpColors)
		delete mpColors;

	while (!mImages.IsEmpty())
	{
		delete mImages.GetTail();
		mImages.RemoveTail();
	}
}
void CDemoAppDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CDemoAppDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_SHARED, &CDemoAppDlg::OnBnClickedButtonShared)
	ON_BN_CLICKED(IDC_BUTTON_ADD_ROI, &CDemoAppDlg::OnBnClickedButtonAddRoi)
	ON_BN_CLICKED(IDC_BUTTON_UP_POS, &CDemoAppDlg::OnBnClickedButtonUpPos)
	ON_COMMAND(ID_FILE_OPEN32771, &CDemoAppDlg::OnFileOpen32771)
	ON_COMMAND(ID_FILE_EXIT, &CDemoAppDlg::OnFileExit)
	ON_COMMAND(ID_GET_WINDOW, &CDemoAppDlg::OnGetWindow)
	ON_COMMAND(ID_SET_WINDOW, &CDemoAppDlg::OnSetWindow)
	ON_COMMAND(ID_SET_AUTOWINDOW, &CDemoAppDlg::OnSetAutoWindow)
	ON_COMMAND(ID_PROCESS_SAVE, &CDemoAppDlg::OnProcessSave)
	ON_COMMAND(ID_PROCESS_SAVEALL, &CDemoAppDlg::OnProcessSaveall)
	ON_COMMAND(ID_PROCESS_SAVEALLWITHNEWNAME, &CDemoAppDlg::OnProcessSaveallwithnewname)
	ON_COMMAND(ID_SET_STATUSTEXT, &CDemoAppDlg::OnSetStatustext)
	ON_COMMAND(ID_SET_CURSURBROADCAST, &CDemoAppDlg::OnSetCursurbroadcast)
	ON_BN_CLICKED(IDC_BUTTON_ADD_COLORS, &CDemoAppDlg::OnBnClickedButtonAddColors)
	ON_COMMAND(ID_PROCESS_PROCESSVOLUME, &CDemoAppDlg::OnProcessProcessvolume)
	ON_COMMAND(ID_PROCESS_CURRENTINVOLUME, &CDemoAppDlg::OnProcessCurrentinvolume)
	ON_COMMAND(ID_PROCESS_SAVEWITHNEWMATRIX, &CDemoAppDlg::OnProcessSavewithnewmatrix)
	ON_COMMAND(ID_SET_TITLE, &CDemoAppDlg::OnSetTitle)
	//ON_BN_CLICKED(IDC_BUTTON_ADD_COLOR_MAP, &CDemoAppDlg::OnBnClickedButtonAddColorMap)
	ON_COMMAND(ID_SET_TOGGLECOLORMAP, &CDemoAppDlg::OnSetTogglecolormap)
	ON_COMMAND(ID_SET_WINDOWRANGE, &CDemoAppDlg::OnSetWindowrange)
	ON_COMMAND(ID_GET_TEST, &CDemoAppDlg::OnGetTest)
	ON_BN_CLICKED(IDC_OK, &CDemoAppDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_CANCEL, &CDemoAppDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_MAX, &CDemoAppDlg::OnBnClickedButtonMax)
	ON_BN_CLICKED(IDC_BUTTON_NEXT, &CDemoAppDlg::OnBnClickedButtonNext)
	ON_BN_CLICKED(IDC_BUTTON_PREV, &CDemoAppDlg::OnBnClickedButtonPrev)
	ON_CBN_SELCHANGE(IDC_COMBO_SCORE_TYPE, &CDemoAppDlg::OnCbnSelchangeComboScoreType)
END_MESSAGE_MAP()


// CDemoAppDlg message handlers

BOOL CDemoAppDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

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

	CComboBox* pComboScoreType = (CComboBox*)GetDlgItem(IDC_COMBO_SCORE_TYPE);
	if (pComboScoreType)
	{
		for (int i = 0; i < (int)EScoreType::N_SCORE_TYPES; i++)
			pComboScoreType->AddString(ScoreTypeName((EScoreType)i));
		pComboScoreType->SetCurSel((int)gConfig.mScoreType);
	}

	// TODO: Add extra initialization here
	//DisplayPos();

	gpDlg = this;
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDemoAppDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CDemoAppDlg::OnPaint()
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
LRESULT CDemoAppDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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
void CDemoAppDlg::OnOK()
{
	static int i = 0;
	i++;
	OnBnClickedButtonUpPos();
}
// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDemoAppDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
void CDemoAppDlg::OnBnClickedButtonShared()
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
void CDemoAppDlg::LoadViewer(void)
{
	if (mpImageRIF)
		return;

	mpImageRIF = new CImageRIF(0, 0, false, mMyViewerOffsetX, mMyViewerOffsetY);
	mpImageRIF->SetTitle("Image Quality Verification Viewer");
	mpImageRIF->SetViewerBroadPos();
	mpImageRIF->SetIndicesToUpdatePosition2d(miPos,miPos2d);
}
bool CDemoAppDlg::LoadViewerWithImages(const char *zName)
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
void CDemoAppDlg::InitSharedVolume(void)
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
void CDemoAppDlg::OnBnClickedButtonAddRoi()
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
//void CDemoAppDlg::DisplayPos(void)
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
void CDemoAppDlg::GetPos1(void)
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
void CDemoAppDlg::OnBnClickedButtonUpPos()
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
void CDemoAppDlg::OnFileOpen32771()
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
				if (LoadViewerWithImages(sImageName))
				{
					CArinetaImages::SetDebug(0xff);
					mpImages = new CArinetaImages(sImageName);
					miPos = mpImages->GetCurrentPosition();
					//DisplayPos();
					mImages.AddTail(mpImages);
					mpImages->ComputeRotationCenter(this);
					mpImages->PrepareOnInit();
					mpRingsScorer = new CRingsScorer(mpImages);
					miPos = mpRingsScorer->ScoreAllImages();
					OnCurrentSelectedByScorer(miPos);
					mpImageRIF->DisplayShared(mpImages->GetSharedWideVolume());
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
//void CDemoAppDlg::OnFileOpenbinary()
//{
//	CMyFileDialog dlg(CMyFileDialog::FD_OPEN, "Open Binary File for Display", "d:\\Dump");
//	if (dlg.DoModal())
//	{
//		CString sImageName(dlg.m_ofn.lpstrFile);
//
//		if (!mpImageRIF)
//		{
//			if (LoadViewerWithImages(sImageName))
//			{
//				mpDataFiles = new CDataFiles(sImageName);
//				miPos = mpDataFiles->GetCurrentPosition();
//				DisplayPos();
//				//mImages.AddTail(mpImages);
//			}
//		}
//	}
//
//}
//void CDemoAppDlg::OnFileOpencolorer()
//{
//	if (!mpImageRIF || !mpImages)
//	{
//		CMyWindows::MessBox("Please load some base images first","Notice");
//		return;
//	}
//	
//	CMyFileDialog dlg(CMyFileDialog::FD_OPEN,"Open Coloring Images","d:\\Cirs_Images");
//	if (dlg.DoModal())
//	{
//		CString sImageName(dlg.m_ofn.lpstrFile);
//		mpImageRIF->FileOpenColorer(sImageName);
//	}
//}
void CDemoAppDlg::OnFileExit()
{
	OnCancel();
}
//void CDemoAppDlg::OnProcessSmooth()
//{
//	if (!mpImages)
//		return;
//	if (!mpImages->GetDicom())
//	{
//		CMyWindows::MessBox("This function requires DICOM images", "Input Error");
//		return;
//	}
//	if (mpSmoother)
//		return;
//
//	mpSmoother = new CSmoother(mpImages->GetNLines(), mpImages->GetNCols());
//
//	if (!mpSmoothed)
//	{
//		mpSmoothed = mpImages->CreateSharedImage("Smoothed");
//		if (!mpSmoothed)
//		{
//			CMyWindows::MessBox("Failed to creat shared image", "SW Error");
//			return;
//		}
//	}
//	
//	short *pInput = mpImages->GetImageDataStart(miPos);
//	mpSmoother->Smooth((short *)mpSmoothed->GetDataStart(), pInput);
//	mpImageRIF->DisplayShared(mpSmoothed);
//	mpImageRIF->AddDiff();
//}
bool CDemoAppDlg::InitProcessVolume(void)
{
	if (!mpImages)
		return false;
	if (!mpSharedVolume)
	{
		if (!mpImages->CreateSharedVolume("SharedVolume",mpSharedVolume))
			return false;
	}
	return true;
}
void CDemoAppDlg::OnProcessCurrentinvolume()
{
	if (!InitProcessVolume())
		return;

	mHighFactor += 0.01f;

	mpSharedVolume->StartWrite();
	ProcessImageInVolume(miPos);
	mpSharedVolume->OnPageUpdate(miPos);
	mpImageRIF->DisplayShared(mpSharedVolume);
}
void CDemoAppDlg::OnProcessProcessvolume()
{
	if (!InitProcessVolume())
		return;

	mpSharedVolume->StartWrite();
	CPosition *pPos = mpImages->GetPosition();
	for (int iImage = pPos->miFirst; iImage <= pPos->miLast; iImage += pPos->mStep)
	{
		ProcessImageInVolume(iImage);
	}
	mpSharedVolume->OnAllUpdated();
	mpImageRIF->DisplayShared(mpSharedVolume);
	mpImages->SetCurrent(miPos);
}
void CDemoAppDlg::ProcessImageInVolume(int iImage)
{
	if (!mpImages || !mpSharedVolume)
		return;

	mpImages->SetCurrent(iImage);

	short * pProcessed = mpSharedVolume->GetImageStart(iImage);
	short *pImageRaster = mpImages->GetImageDataStart(iImage);
	int n = mpImages->GetNLines() * mpImages->GetNCols();

	CTImage<short> *pImage = mpImages->GetImage();

	short avg = (short)pImage->Average();
	while (n-- > 0)
	{
		short value = *pImageRaster++;
		if (value > avg)
			*pProcessed++ = (short)(value * mHighFactor);
		else
			*pProcessed++ = (short)(value * mLowFactor);
	}
}
void CDemoAppDlg::UpdateSmooth(void)
{
	if (!mpSmoother)
		return;
	short *pInput = mpImages->GetImageDataStart(miPos);
	mpSmoothed->StartWriteSync();
	mpSmoother->Smooth((short *)mpSmoothed->GetDataStart(), pInput);
	mpSmoothed->EndWriteSync();
	mpImageRIF->DisplayShared(mpSmoothed);
}
void CDemoAppDlg::OnGetWindow()
{
	if (!mpImageRIF)
		return;
	int iRectangle = - 1;
	CNameGetDialog::GetIntValue("Enter index of rectangle", iRectangle);
	mpImageRIF->GetWindow(iRectangle);
}
void CDemoAppDlg::OnSetWindow()
{
	if (!mpImageRIF)
		return;

	int center = 100;
	int width = 500;
	int iRectangle = - 1;

	CNameGetDialog::GetIntValue("Enter Window Center", center);
	CNameGetDialog::GetIntValue("Enter Window Width", width);
	CNameGetDialog::GetIntValue("Enter index of rectangle", iRectangle);

	mpImageRIF->SetWindow(center,width,iRectangle);
}
void CDemoAppDlg::OnSetStatustext()
{
	CString sText("Status Text");
	int iField = 4;

	CNameGetDialog::GetStringValue("Get Target Field Index", sText);
	CNameGetDialog::GetIntValue("Get Target Field Index", iField);

	mpImageRIF->SetStatusText(sText,iField);
}
void CDemoAppDlg::OnSetCursurbroadcast()
{
	int iRectangle = -1;
	CNameGetDialog::GetIntValue("Select Rectangle Index", iRectangle);
	mpImageRIF->SetCursorBroadcast(iRectangle);
}
void CDemoAppDlg::OnSetAutoWindow()
{
	if (!mpImageRIF)
		return;

	CDataCoordinates coo(200,200);
	CString sName(mpSharedVolume->Name());
	mpImageRIF->SetAutoWindow(sName,coo);
}
void CDemoAppDlg::OnProcessSave()
{
	if (!mpImages)
		return;
	if (!mpSmoothed)
		return;

	short *pRaster = mpSmoothed->GetData();
	
	mpImages->SaveDicomWithNewRaster(mpImages->GetCurrentPosition(), (short *)pRaster, "Smoothed");
}
void CDemoAppDlg::OnProcessSaveall()
{
	if (!mpImages)
	{
		CMyWindows::MessBox("Failed to find images to save", "Application Node");
		return;
	}
	if (!mpSmoothed)
	{
		CMyWindows::MessBox("Failed to find smoothed images to save", "Application Node");
		return;
	}

	CString saveDir("d:\\Dump\\SaveDemo");
	CMyWindows::DeleteDirFiles(saveDir, false, false);

	mpImages->StartSeriesSave(true, "Smoothed", "Smoothed Image", saveDir);
	if (!msAppendToPatientName.IsEmpty())
	{
		CMyDicom *pDicom = mpImages->GetDicom();
		if (pDicom)
		{
			CString sPatName = pDicom->GetPatientName(true);
			sPatName += msAppendToPatientName;
			CMyDicomWriter::AddTextTag(PATIENT_NAME_GROUP, PATIENT_NAME_NUM, sPatName);
		}
	}

	int iFirst = mpImages->GetFirst();
	int iLast = mpImages->GetLast();
	int step = mpImages->GetStep();

	CMyProgress sand(iFirst,iLast);

	int nSaved = 0;
	for (int iSave = iFirst; iSave <= iLast; iSave += step)
	{
		short *pInput = mpImages->GetImageDataStart(iSave);
		mpSmoother->Smooth((short *)mpSmoothed->GetDataStart(), pInput);
		short *pRaster = mpSmoothed->GetData();

		//CMyDicom *pDicom = mpImages->GetDicom();
		//CFileName fName = pDicom->Name();

		//CString sFileName(saveDir);
		//sFileName += "\\";
		//sFileName += fName.Private();

		if (!mpImages->SaveDicomInSeries(iSave, NULL/*sFileName*/, (short *)pRaster))
		{
			CMyWindows::MessBox("Failed to save series","SW Error");
			return;
		}

		sand.SetPos(iSave);
		nSaved++;
	}
	char zBuf[512];
	sprintf_s(zBuf, sizeof(zBuf), "%d images saved in <%s>", nSaved, (const char *)saveDir);
	CMyWindows::MessBox(zBuf, "Save All Finished");
}
void CDemoAppDlg::OnProcessSaveallwithnewname()
{
	CString sText;
	if (!CMyWindows::GetUserText("Enter Text to Append to Patients' Name", sText))
		return;

	msAppendToPatientName = " ";
	msAppendToPatientName += sText;
	OnProcessSaveall();
	msAppendToPatientName.Empty();
}
void CDemoAppDlg::OnProcessSavewithnewmatrix()
{
	if (!mpImages)
	{
		CMyWindows::MessBox("Failed to find images to save", "Application Node");
		return;
	}

	CString sText;
	if (!CMyWindows::GetUserText("Enter New Matrix (128 - 2048)", sText))
		return;
	int matrix = atoi(sText);
	if (matrix < 128 || matrix > 2048)
	{
		CMyWindows::MessBox("Required matrix is out of range", "Warning");
		return;
	}

	CString saveDir("d:\\Dump\\SaveWithNewMatrixDemo");
	CMyWindows::DeleteDirFiles(saveDir, false, false);

	mpImages->StartSeriesSave(true, "NewMatrix", "New Matrix Image", saveDir);
	CMyDicomWriter::PrepareNewSize(matrix, matrix, true, mpImages->GetDicom());

	int iFirst = mpImages->GetFirst();
	int iLast = mpImages->GetLast();
	int step = mpImages->GetStep();

	CMyProgress sand(iFirst,iLast);
	CTImage<float> zoomedRaster("zoomedFloat", matrix, matrix);
	CTImage<short> zoomedRasterUShort("zoomedShort", matrix, matrix);

	CZoomPanParams params("zoomParams");
	float originalMmPerPixel = mpImages->MmPerPixel();
	params.SetMmPerPixel(originalMmPerPixel * mpImages->GetNCols() / matrix);

	int nSaved = 0;
	for (int iSave = iFirst; iSave <= iLast; iSave += step)
	{
		mpImages->SetCurrent(iSave);
		CTImage<short> *pInputImage = mpImages->GetImage();
		CZoomer::Zoom(zoomedRaster, pInputImage, &params);

		// Float to short
		int count = zoomedRaster.GetNPixels();
		float *pZoomed = zoomedRaster.GetData();
		short *pResultRaster = zoomedRasterUShort.GetData();
		short *pResult = pResultRaster;
		while (count--)
		{
			*pResult++ = (short)*pZoomed++;
		}

		mpImages->SaveDicomInSeries(iSave, NULL, (short *)pResultRaster);
		sand.SetPos(iSave);
		nSaved++;
	}
	char zBuf[512];
	sprintf_s(zBuf, sizeof(zBuf), "%d images saved in <%s>", nSaved, (const char *)saveDir);
	CMyWindows::MessBox(zBuf, "Save All Finished");
}
void CDemoAppDlg::OnBnClickedButtonAddColors()
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
//void CDemoAppDlg::OnBnClickedButtonAddColorMap()
//{
//	if (!mpImages && !mpSharedVolume)
//		return;
//	if (!mpImageRIF)
//		return;
//
//	CString sPath("d:\\tmp");
//	if (!CMyWindows::VerifyDirectory(sPath))
//		return;
//
//	const int N_GRAY_LEVELS = 256;
//	typedef struct
//	{
//		float red;
//		float green;
//		float blue;
//	} SRGB_Entry;
//	SRGB_Entry aLut[N_GRAY_LEVELS];
//	int GREEN_MIN = N_GRAY_LEVELS / 4;
//	int GREEN_MAX = N_GRAY_LEVELS * 3 / 4;
//	int GREEN_MIDDLE = (GREEN_MIN + GREEN_MAX) / 2;
//	int GREEN_HALF_SPAN = GREEN_MIDDLE - GREEN_MIN;
//
//	for (int i = 0; i < N_GRAY_LEVELS; i++)
//	{
//		float relative = (float)i / (N_GRAY_LEVELS - 1);
//		aLut[i].blue = 1 - relative;
//		aLut[i].red = relative;
//		if (i > GREEN_MIN && i < GREEN_MAX)
//		{
//			aLut[i].green = 1 - (float)(abs(i - GREEN_MIDDLE)) / GREEN_HALF_SPAN;
//		}
//	}
//
//	CString sfName(sPath + "\\DemoApp.ColorMap");
//	FILE *pf = MyFOpenWithErrorBox(sfName,"wb","ColorMap Example");
//	if (!pf)
//		return;
//
//	fwrite (&aLut[0], sizeof(aLut[0]), N_GRAY_LEVELS, pf);
//	fclose(pf);
//
//	mpImageRIF->DisplayColorMap(sfName);
//	mbColormapOn = true;
//}
void CDemoAppDlg::OnSetTitle()
{
	if (!mpImageRIF)
		return;

	CUserTextDialog dlg("Select Title for MyViewer");
	if (dlg.DoModal() == IDOK)
	{
		CString sText = dlg.GetText();
		if (!sText.IsEmpty())
		{
			mpImageRIF->SetTitle(sText);
			mpImageRIF->Refresh();
		}
	}
}


void CDemoAppDlg::OnSetTogglecolormap()
{
	if (!mpImageRIF)
		return;

	mbColormapOn = !mbColormapOn;
	mpImageRIF->SetColorMap(mbColormapOn);
}


void CDemoAppDlg::OnSetWindowrange()
{
	if (!mpImageRIF)
		return;

	int minVal = 0;
	int maxVal = 100;

	int iRectangle = -1;
	CNameGetDialog::GetIntValue("Enter Window Min Value", minVal);
	CNameGetDialog::GetIntValue("Enter Window Max Value", maxVal);

	if (maxVal > minVal)
		mpImageRIF->SetWindowRange(minVal,maxVal,iRectangle);
}

static double ux = 0;

void CDemoAppDlg::OnGetTest()
{
       CArchivesImages Image("F:\\MyViewer\\_From_Users\\Decode error\\S20190\\00001\\Image0001.dcm"); //file = path of S20190 Iodine series
	   ux = 1;
	   /*
       CString var;
       Image.GetDicom()->GetTextField(0x0008, 0x0033, var); //get content time
       double x = atof(var);
       ux = x; // heap error by return from the function
	   */
}
void CDemoAppDlg::DisplayCircle(CDataCoordinates& center, float radius)
{
	static int count = 0;
	//if (count > 0)
	//	return;
	count++;

	//char zName[128];
	//sprintf_s(zName, "CenteredCircle", count, miPos + 1, miPos2d + 1);
	CDataRoi* pCircle = new CDataRoi(NULL, "CenteredCircle", 0xff0080);
	pCircle->InitEllipse(center, radius);
	pCircle->SetCircle();
	pCircle->SetFixedCenter();
	//pCircle->LockToPosition2d(miPos, miPos2d);
	pCircle->SetSaveToXml(true);
	pCircle->mbReportClientOnActivation = true;
	mpImageRIF->SetCurrentDR(0);
	mpImageRIF->DisplayGraphic(pCircle);
}
void CDemoAppDlg::OnCurrentSelectedByScorer(int iPos)
{
	miPos = iPos;
	mpImages->SetCurrent(miPos);
	mpImageRIF->SetPosition(mpImages->GetPatternName(), miPos);
	DisplayScore();
}
void CDemoAppDlg::DisplayScore()
{
	const CImageScore& score = mpRingsScorer->ScoreCurrentImage(miPos);
	SetParameter(IDC_EDIT_I_IMAGE, miPos);
	SetParameter(IDC_EDIT_SCORE, score.mScore);
	SetParameter(IDC_EDIT_RADIUS, score.miRing);
	if (score.miRing >= 1)
		DisplayCircle(mpImages->GetRotationCenter(), (float)score.miRing);
}
void CDemoAppDlg::OnBnClickedOk()
{
	gConfig.SaveToFile();
	CMyDialogEx::OnOK();
}
void CDemoAppDlg::OnBnClickedCancel()
{
	CMyDialogEx::OnCancel();
}
void CDemoAppDlg::OnBnClickedButtonMax()
{
	mpRingsScorer->DisplayMaxPeak();
}
void CDemoAppDlg::OnBnClickedButtonNext()
{
	mpRingsScorer->DisplayNextPeak();
}
void CDemoAppDlg::OnBnClickedButtonPrev()
{
	mpRingsScorer->DisplayPrevPeak();
}
void CDemoAppDlg::OnCbnSelchangeComboScoreType()
{
	CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_SCORE_TYPE);
	int iSel = pCombo ? pCombo->GetCurSel() : -1;
	if (iSel < 0)
		return;

	gConfig.mScoreType = (EScoreType)iSel;
	gConfig.SaveToFile();

	if (mpRingsScorer)
		mpRingsScorer->OnActiveScoreTypeChanged();
}
