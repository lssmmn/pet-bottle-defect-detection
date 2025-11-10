// MainDlg.cpp
#include "pch.h"
#include "PETBottleSealDefect.h"
#include "MainDlg.h"
#include "CameraDlg.h"

BEGIN_MESSAGE_MAP(CMainDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_INSPECT_START, &CMainDlg::OnBnClickedBtnInspectStart)
    ON_BN_CLICKED(IDC_BTN_PROGRAM_EXIT, &CMainDlg::OnBnClickedBtnProgramExit)
END_MESSAGE_MAP()

CMainDlg::CMainDlg(CWnd* pParent)
    : CDialogEx(IDD_MAIN_DIALOG, pParent)
{
}

void CMainDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BOOL CMainDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    m_font.CreatePointFont(140, _T("맑은 고딕")); 

    // 모든 컨트롤에 폰트 적용
    CWnd* pWnd = GetWindow(GW_CHILD);
    while (pWnd)
    {
        pWnd->SetFont(&m_font);
        pWnd = pWnd->GetWindow(GW_HWNDNEXT);
    }

    return TRUE;
}

void CMainDlg::OnBnClickedBtnInspectStart()
{
    CCameraDlg dlg;
    dlg.DoModal();
}

void CMainDlg::OnBnClickedBtnProgramExit()
{
    EndDialog(IDOK);
}