/////////////////////////////////////////////////////////////////////////////
// Name:        glcanvas.cpp (GLSoft)
// Purpose:     wxGLCanvas and wxGLContext on GLSoft: a context is GLSoft's
//              state, a canvas a picture the size of the window that GL
//              draws into; SwapBuffers puts the picture on the window
//              through a wxClientDC (in a paint, olui's buffer)
// Copyright:   (c) 2026 AmiCradle Team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////
#include "wx/wx.h"
#include "wx/glcanvas.h"
#include "wx/dcclient.h"
#include "glsoft.h"

wxIMPLEMENT_CLASS(wxGLCanvas, wxWindow);

wxGLContext::wxGLContext(wxGLCanvas *WXUNUSED(win), const wxGLContext *WXUNUSED(other),
                         const void *WXUNUSED(ctxAttrs))
{
    m_ctx = glsoftCreateContext();
}

wxGLContext::~wxGLContext()
{
    glsoftDestroyContext(m_ctx);
}

bool wxGLContext::SetCurrent(const wxGLCanvas& win) const
{
    if ( !m_ctx )
        return false;
    glsoftMakeCurrent(m_ctx);
    win.GLSoftMakeTarget();
    return true;
}

wxGLCanvas::wxGLCanvas(wxWindow *parent, wxWindowID id, const int *attribList, const wxPoint& pos,
                       const wxSize& size, long style, const wxString& name, const wxPalette& palette)
{
    Create(parent, id, pos, size, style, name, attribList, palette);
}

wxGLCanvas::~wxGLCanvas()
{
    glsoftSetTarget(NULL, 0, 0, 0);
}

bool wxGLCanvas::Create(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style,
                        const wxString& name, const int *WXUNUSED(attribList),
                        const wxPalette& WXUNUSED(palette))
{
    if ( !wxWindow::Create(parent, id, pos, size, style | wxFULL_REPAINT_ON_RESIZE, name) )
        return false;
    // GL paints all of it: nothing to erase first
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    return true;
}

void wxGLCanvas::GLSoftMakeTarget() const
{
    int w, h;
    GetClientSize(&w, &h);
    if ( w < 1 ) w = 1;
    if ( h < 1 ) h = 1;
    if ( !m_picture.IsOk() || m_picture.GetWidth() != w || m_picture.GetHeight() != h )
        m_picture.Create(w, h, 24);
    // the pixels, marked as changed (each pixel is a big-endian 0xAARRGGBB,
    // the bitmap's A R G B bytes)
    unsigned char *px = m_picture.GetPixels();
    glsoftSetTarget((unsigned int *)px, w, h, w);
}

bool wxGLCanvas::SetCurrent(const wxGLContext& context) const
{
    return context.SetCurrent(*this);
}

bool wxGLCanvas::SwapBuffers()
{
    if ( !m_picture.IsOk() )
        return false;
    wxClientDC dc(this);
    dc.DrawBitmap(m_picture, 0, 0, false);
    // drawing again needs SetCurrent, which marks the pixels changed
    glsoftSetTarget(NULL, 0, 0, 0);
    return true;
}
