// PETBottleSealDefect.cpp
#include "pch.h"
#include "PETBottleSealDefect.h"
#include "MainDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CPETBottleSealDefectApp, CWinApp)
END_MESSAGE_MAP()

CPETBottleSealDefectApp::CPETBottleSealDefectApp()
{
}

CPETBottleSealDefectApp theApp;

BOOL CPETBottleSealDefectApp::InitInstance()
{
    CWinApp::InitInstance();

    CMainDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    return FALSE;
}