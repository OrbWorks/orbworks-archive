// OrbitVisualManager.h: interface for the COrbitVisualManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ORBITVISUALMANAGER_H__9B3BA807_061D_463C_9597_4A658B1D3526__INCLUDED_)
#define AFX_ORBITVISUALMANAGER_H__9B3BA807_061D_463C_9597_4A658B1D3526__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class COrbitVisualManager : public CBCGPVisualManagerVS2005
{
    DECLARE_DYNCREATE(COrbitVisualManager)

protected:
    COrbitVisualManager();

public:
    virtual ~COrbitVisualManager();

    virtual COLORREF OnDrawControlBarCaption (CDC* pDC, CBCGPDockingControlBar* pBar, 
        BOOL bActive, CRect rectCaption, CRect rectButtons);
};

#endif // !defined(AFX_ORBITVISUALMANAGER_H__9B3BA807_061D_463C_9597_4A658B1D3526__INCLUDED_)
