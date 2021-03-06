// DlgANPR.cpp : implementation file
//

#include "stdafx.h"
#include "DlgANPR.h"


// CDlgANPR dialog

IMPLEMENT_DYNAMIC(CDlgANPR, CBaseDialog)

static void vRescanList(LPVOID lpParam);

CDlgANPR::CDlgANPR(CWnd* pParent /*=NULL*/)
	: CBaseDialog(CDlgANPR::IDD, pParent)
{

}

CDlgANPR::CDlgANPR(NODEITEM* pNode, CWnd* pParent /*=NULL*/)
	: CBaseDialog(CDlgANPR::IDD, pParent)
{
	m_Node = pNode;
	bRefreshShotList = true;
}

CDlgANPR::~CDlgANPR()
{
	bRefreshShotList = false;

	HANDLE hCloseSnapshot = ::CreateEvent(NULL, false, false, _T("Global\\ETROCENTER_SHOTLIST_CLOSE"));
	if(hCloseSnapshot == NULL)
	{
		TRACE(_T("Create Restart Event Error code :%d"), ::GetLastError());
	}
	SetEvent( hCloseSnapshot );
	CloseHandle(hCloseSnapshot);
}

void CDlgANPR::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_VIDEO, m_videovv);
}


BEGIN_MESSAGE_MAP(CDlgANPR, CDialog)
	ON_NOTIFY(NM_CLICK, IDC_LIST_ANPR_FILES, &CDlgANPR::OnNMClickListAnprFiles)
	
END_MESSAGE_MAP()


// CDlgANPR message handlers

BOOL CDlgANPR::OnInitDialog()
{
	CBaseDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	CListCtrl * pSnapList = (CListCtrl *)this->GetDlgItem(IDC_LIST_ANPR_FILES);
	
	LV_COLUMN ListColumn;

	WCHAR * ListTitles[2] = {_T("Time"), _T("File")};
	//CString ListTitles[9] ={};
	//for(int iTemp =1; iTemp <= 8; iTemp++) // ListTitles[0] Set to NULL String
	//{
	//	::AfxExtractSubString(ListTitles[iTemp], csListTitles, iTemp-1, ';');
	//}

	DWORD nWidth[]={80, 200};
	ListColumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
	ListColumn.fmt = LVCFMT_LEFT;

	for(int i = 0; i < 2; i++)
	{
		ListColumn.iSubItem = i;
        ListColumn.cx = nWidth[i];
		ListColumn.pszText = (LPWSTR)(LPCTSTR)ListTitles[i];
		pSnapList->InsertColumn(i, &ListColumn);
	}

	DWORD dwStyle = pSnapList->GetStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;
	pSnapList->SetExtendedStyle(dwStyle);

	pSnapList->ModifyStyle(0, LVS_SINGLESEL);

	vScanAllShot();

	//m_videovv.ModifyStyle(0, WS_BORDER, 0);
	m_videovv.m_bDisplayOSD = false;
	m_videovv.vAttach(m_Node);
	m_videovv.MoveWindow(CRect(19, 25, 510, 330), true);
	m_videovv.ShowWindow(SW_SHOW);

	AfxBeginThread(RefreshShotList, this);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgANPR::vScanAllShot()
{
	CListCtrl * pSnapList = (CListCtrl *)this->GetDlgItem(IDC_LIST_ANPR_FILES);

	CIniReader s_ini;
	s_ini.setINIFullFileName(_T("\\ec.ini"));
	CString csFolder = s_ini.getKeyValue(_T("evt_snapshot_folder"), _T("lpr"));

	if(csFolder.GetBuffer() == 0)	csFolder = _T("c:\\recording\\dodoevt\\");
	csFolder = csFolder + _T("*.*");

	CFileFind finder;
	BOOL bFileExit = finder.FindFile(csFolder.GetBuffer());

	while(bFileExit)
	{
		bFileExit = finder.FindNextFile();

		if(finder.GetFileName() == _T(".") || finder.GetFileName() == _T(".."))
			continue;
		if(finder.IsDirectory())
		{
			continue;
		}
		else
		{
			CTime timeTemp;
			finder.GetCreationTime(timeTemp);
			CString csCreationTime = timeTemp.Format(_T("%Y-%m-%d %H:%M:%S"));
			CString csFileName = finder.GetFileName();

			int iItem = pSnapList->InsertItem(0, csCreationTime.GetBuffer());
			pSnapList->SetItemText(iItem, 1, csFileName.GetBuffer());
			
		}
	}
}
void CDlgANPR::OnNMClickListAnprFiles(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: Add your control notification handler code here
	USES_CONVERSION;
	CListCtrl * pSnapList = (CListCtrl *)this->GetDlgItem(IDC_LIST_ANPR_FILES);
	int nItem = pSnapList->GetSelectionMark();
	CString csTemp = pSnapList->GetItemText(nItem, 1);
	CString csFileName = csTemp;
	CString csCreationTime = pSnapList->GetItemText(nItem, 0);

	int nFileNameLength = csTemp.GetLength();
	csTemp = csTemp.Left(nFileNameLength-4);
	//AfxMessageBox(csTemp);

	CString csEventType, csIP;
	AfxExtractSubString(csIP, csTemp, 0, '_');
	AfxExtractSubString(csEventType, csTemp, 1, '_');

	CIniReader s_ini;
	s_ini.setINIFullFileName(_T("\\ec.ini"));
	CString csFolder = s_ini.getKeyValue(_T("evt_snapshot_folder"), _T("lpr"));

	if(csFolder.GetBuffer() == 0)	csFolder = _T("c:\\recording\\dodoevt\\");

	//--Show JPG
	CString csFilePath = csFolder + csTemp + _T(".jpg");
	//CFileDialog cfd(true, _T(".jpg"), NULL, OFN_FILEMUSTEXIST|OFN_HIDEREADONLY, _T("Executable   Files   (*.jpg)|*.jpg|All   Files   (*.*)|*.*||"), this);     
	CPictureCtrl* m_pic = (CPictureCtrl*)this->GetDlgItem(IDC_PIC_ANPR);
	CRect rect;
	m_pic->GetClientRect(&rect);

	CImage m_image;
	m_image.Load(csFilePath.GetBuffer());
	CDC* pDC = m_pic->GetWindowDC();
	m_image.Draw(pDC->m_hDC, rect);
	ReleaseDC(pDC);

	//--Set File Description
	SetDlgItemTextW(IDC_EDIT_ANPR_FILENAME, csFileName.GetBuffer());
	SetDlgItemTextW(IDC_EDIT_ANPR_TIME, csCreationTime.GetBuffer());
	SetDlgItemTextW(IDC_EDIT_ANPR_IP, A2W(m_Node->ip));

	*pResult = 0;
}

UINT CDlgANPR::RefreshShotList(LPVOID lpParam)
{
	CDlgANPR* pThis = (CDlgANPR*)lpParam;
	
	SECURITY_DESCRIPTOR secutityDese; 
	::InitializeSecurityDescriptor(&secutityDese, SECURITY_DESCRIPTOR_REVISION); 
	::SetSecurityDescriptorDacl(&secutityDese,TRUE,NULL,FALSE); 

	SECURITY_ATTRIBUTES securityAttr; 

	// set SECURITY_ATTRIBUTES 
	securityAttr.nLength = sizeof SECURITY_ATTRIBUTES; 
	securityAttr.bInheritHandle = FALSE; 
	securityAttr.lpSecurityDescriptor = &secutityDese; 

	HANDLE hRefreshSnapshot = ::CreateEvent(&securityAttr, false, false, _T("Global\\ETROCENTER_SHOTLIST_REFRESH"));
	if(hRefreshSnapshot == NULL)
	{
		//LogEvent(_T("Create Refresh Action Error code :%d"), ::GetLastError());
	}

	HANDLE hCloseSnapshot = ::CreateEvent(&securityAttr, false, false, _T("Global\\ETROCENTER_SHOTLIST_CLOSE"));
	if(hCloseSnapshot == NULL)
	{
		//LogEvent(_T("Create Refresh Action Error code :%d"), ::GetLastError());
	}

	HANDLE hThread[2] = {hRefreshSnapshot, hCloseSnapshot};

	DWORD dwResult = 0;

	while(pThis->bRefreshShotList)
	{
		dwResult = ::WaitForMultipleObjects(2, hThread, false, INFINITE);

		switch(dwResult)
		{
		case WAIT_OBJECT_0:
			vRescanList(pThis);
			break;
		case WAIT_OBJECT_0+1:
			return 0;
			break;
		default:
			ASSERT(true);
			break;
		}
	};

	return 0;
}

void vRescanList(LPVOID lpParam)
{
	CDlgANPR* pThis = (CDlgANPR*)lpParam;
	CListCtrl * pSnapList = (CListCtrl *)pThis->GetDlgItem(IDC_LIST_ANPR_FILES);
	pSnapList->DeleteAllItems();
	
	
	CIniReader s_ini;
	s_ini.setINIFullFileName(_T("\\ec.ini"));
	CString csFolder = s_ini.getKeyValue(_T("evt_snapshot_folder"), _T("lpr"));

	if(csFolder.GetBuffer() == 0)	csFolder = _T("c:\\recording\\dodoevt\\");
	csFolder = csFolder + _T("*.*");

	CFileFind finder;
	BOOL bFileExit = finder.FindFile(csFolder.GetBuffer());

	while(bFileExit)
	{
		bFileExit = finder.FindNextFile();

		if(finder.GetFileName() == _T(".") || finder.GetFileName() == _T(".."))
			continue;
		if(finder.IsDirectory())
		{
			continue;
		}
		else
		{
			CTime timeTemp;
			finder.GetCreationTime(timeTemp);
			CString csCreationTime = timeTemp.Format(_T("%Y-%m-%d %H:%M:%S"));
			CString csFileName = finder.GetFileName();

			int iItem = pSnapList->InsertItem(0, csCreationTime.GetBuffer());
			pSnapList->SetItemText(iItem, 1, csFileName.GetBuffer());
			
		}
	}

	pThis->UpdateWindow();
}