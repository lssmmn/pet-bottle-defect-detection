// PETBottleSealDefectDlg.cpp
#include "pch.h"
#include "PETBottleSealDefect.h"
#include "PETBottleSealDefectDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern bool SendImageToTcpServer(const cv::Mat& image, CString& result);

BEGIN_MESSAGE_MAP(CPETBottleSealDefectDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_START, &CPETBottleSealDefectDlg::OnBnClickedBtnStart)
    ON_BN_CLICKED(IDC_BTN_DETECT, &CPETBottleSealDefectDlg::OnBnClickedBtnDetect)
    ON_WM_TIMER()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

CPETBottleSealDefectDlg::CPETBottleSealDefectDlg(CWnd* pParent)
    : CDialogEx(IDD_PETBOTTLESEALDEFECT_DIALOG, pParent)
    , m_bCameraRunning(false)
    , m_nTimerID(0)
{
    Pylon::PylonInitialize();
}

CPETBottleSealDefectDlg::~CPETBottleSealDefectDlg()
{
    ReleaseCamera();
    Pylon::PylonTerminate();
}

void CPETBottleSealDefectDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_STATIC_CAM, m_staticCam);
    DDX_Control(pDX, IDC_EDIT_RESULT, m_editResult);
    DDX_Control(pDX, IDC_EDIT_DEFECT_TYPE, m_editDefectType);
    DDX_Control(pDX, IDC_BTN_START, m_btnStart);
    DDX_Control(pDX, IDC_BTN_DETECT, m_btnDetect);
}

BOOL CPETBottleSealDefectDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    SetWindowText(_T("목업"));
    m_btnStart.SetWindowText(_T("켜기"));
    m_btnDetect.SetWindowText(_T("불량"));
    m_editResult.SetWindowText(_T(""));
    m_editDefectType.SetWindowText(_T(""));

    return TRUE;
}

void CPETBottleSealDefectDlg::InitializeCamera()
{
    try
    {
        m_camera.Attach(Pylon::CTlFactory::GetInstance().CreateFirstDevice());
        m_camera.Open();
        m_camera.StartGrabbing(Pylon::GrabStrategy_LatestImageOnly);
        m_bCameraRunning = true;
        m_nTimerID = SetTimer(1, 33, NULL);
    }
    catch (const Pylon::GenericException& e)
    {
        CString msg;
        msg.Format(_T("카메라 초기화 실패: %s"), CString(e.GetDescription()));
        AfxMessageBox(msg);
    }
}

void CPETBottleSealDefectDlg::ReleaseCamera()
{
    if (m_nTimerID)
    {
        KillTimer(m_nTimerID);
        m_nTimerID = 0;
    }

    if (m_camera.IsGrabbing())
    {
        m_camera.StopGrabbing();
    }

    if (m_camera.IsOpen())
    {
        m_camera.Close();
    }

    m_bCameraRunning = false;
}

void CPETBottleSealDefectDlg::OnBnClickedBtnStart()
{
    if (!m_bCameraRunning)
    {
        InitializeCamera();
        m_btnStart.SetWindowText(_T("중지"));
    }
    else
    {
        ReleaseCamera();
        m_btnStart.SetWindowText(_T("켜기"));
    }
}

void CPETBottleSealDefectDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1 && m_bCameraRunning)
    {
        CaptureAndDisplay();
    }
    CDialogEx::OnTimer(nIDEvent);
}

void CPETBottleSealDefectDlg::CaptureAndDisplay()
{
    try
    {
        if (m_camera.RetrieveResult(5000, m_ptrGrabResult, Pylon::TimeoutHandling_ThrowException))
        {
            if (m_ptrGrabResult->GrabSucceeded())
            {
                int width = m_ptrGrabResult->GetWidth();
                int height = m_ptrGrabResult->GetHeight();
                cv::Mat img(height, width, CV_8UC1, (uint8_t*)m_ptrGrabResult->GetBuffer());
                cv::Mat imgColor;
                cv::cvtColor(img, imgColor, cv::COLOR_GRAY2BGR);
                DisplayImage(imgColor);
            }
        }
    }
    catch (const Pylon::GenericException&)
    {
    }
}

void CPETBottleSealDefectDlg::DisplayImage(const cv::Mat& img)
{
    CRect rect;
    m_staticCam.GetClientRect(&rect);

    cv::Mat resized;
    cv::resize(img, resized, cv::Size(rect.Width(), rect.Height()));

    CDC* pDC = m_staticCam.GetDC();

    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = resized.cols;
    bmi.bmiHeader.biHeight = -resized.rows;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    SetDIBitsToDevice(pDC->m_hDC, 0, 0, resized.cols, resized.rows,
        0, 0, 0, resized.rows, resized.data, &bmi, DIB_RGB_COLORS);

    m_staticCam.ReleaseDC(pDC);
}

void CPETBottleSealDefectDlg::OnBnClickedBtnDetect()
{
    try
    {
        if (m_camera.RetrieveResult(5000, m_ptrGrabResult, Pylon::TimeoutHandling_ThrowException))
        {
            if (m_ptrGrabResult->GrabSucceeded())
            {
                int width = m_ptrGrabResult->GetWidth();
                int height = m_ptrGrabResult->GetHeight();
                cv::Mat img(height, width, CV_8UC1, (uint8_t*)m_ptrGrabResult->GetBuffer());
                cv::Mat imgColor;
                cv::cvtColor(img, imgColor, cv::COLOR_GRAY2BGR);

                CString result;
                if (SendImageToTcpServer(imgColor, result))
                {
                    DisplayResult(result);
                }
                else
                {
                    AfxMessageBox(_T("서버 통신 실패"));
                }
            }
        }
    }
    catch (const Pylon::GenericException& e)
    {
        CString msg;
        msg.Format(_T("이미지 캡처 실패: %s"), CString(e.GetDescription()));
        AfxMessageBox(msg);
    }
}

void CPETBottleSealDefectDlg::DisplayResult(const CString& result)
{
    m_editResult.SetWindowText(result);
    m_editDefectType.SetWindowText(result);
}

void CPETBottleSealDefectDlg::OnDestroy()
{
    ReleaseCamera();
    CDialogEx::OnDestroy();
}