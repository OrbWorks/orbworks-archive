// OrbitVisualManager.cpp: implementation of the COrbitVisualManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "OrbitVisualManager.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(COrbitVisualManager, CBCGPVisualManagerVS2005)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

COrbitVisualManager::COrbitVisualManager()
{

}

COrbitVisualManager::~COrbitVisualManager()
{

}

bool g_bWindowStyle = false;
bool g_bRotateGradient = true;

COLORREF COrbitVisualManager::OnDrawControlBarCaption (CDC* pDC, CBCGPDockingControlBar* pBar, 
            BOOL bActive, CRect rectCaption, CRect rectButtons)
{
    ASSERT_VALID (pDC);

    if (globalData.m_nBitsPerPixel <= 8 || globalData.m_bIsBlackHighContrast)
    {
        return CBCGPVisualManagerXP::OnDrawControlBarCaption (pDC, pBar, 
            bActive, rectCaption, rectButtons);
    }

    CBCGPDrawManager dm (*pDC);

    if (!bActive)
    {
        CRect rectScreen;

        MONITORINFO mi;
        mi.cbSize = sizeof (MONITORINFO);
        if (GetMonitorInfo (MonitorFromPoint (CPoint (rectCaption.right, rectCaption.top), 
            MONITOR_DEFAULTTONEAREST), &mi))
        {
            rectScreen = mi.rcWork;
        }
        else
        {
            ::SystemParametersInfo (SPI_GETWORKAREA, 0, &rectScreen, 0);
        }

        rectCaption.right = rectScreen.right;

        dm.FillGradient (rectCaption, m_clrBarGradientDark, m_clrBarGradientDark, FALSE);
    }
    else
    {
        COLORREF clrActiveLight = 
            CBCGPDrawManager::PixelAlpha (globalData.clrActiveCaption, 150);

        dm.FillGradient (rectCaption,	
                        globalData.clrActiveCaption,
                        globalData.clrActiveCaption,
                        FALSE);
    }

    // get the text color
    COLORREF clrCptnText = bActive ?
        globalData.clrCaptionText :
        globalData.clrBarText;

    return clrCptnText;
}
