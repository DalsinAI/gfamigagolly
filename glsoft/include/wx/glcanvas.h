/////////////////////////////////////////////////////////////////////////////
// Name:        wx/glcanvas.h (GLSoft)
// Purpose:     wxGLCanvas and wxGLContext for the GFOpenWX Toolkit, drawn
//              by GLSoft (software GL) into the canvas's own picture and
//              put on the window with the toolkit's wxDC
// Copyright:   (c) 2026 AmiCradle Team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////
#ifndef _WX_GLCANVAS_GLSOFT_H_
#define _WX_GLCANVAS_GLSOFT_H_

#include "wx/defs.h"
#include "wx/window.h"
#include "wx/palette.h"
#include "wx/bitmap.h"

#include "GL/gl.h"

#ifndef wxUSE_GLCANVAS_GLSOFT
#define wxUSE_GLCANVAS_GLSOFT 1
#endif

// the attributes wx's canvases take; GLSoft has one kind of picture, so
// they are read and ignored
enum
{
    WX_GL_RGBA = 1,
    WX_GL_BUFFER_SIZE,
    WX_GL_LEVEL,
    WX_GL_DOUBLEBUFFER,
    WX_GL_STEREO,
    WX_GL_AUX_BUFFERS,
    WX_GL_MIN_RED,
    WX_GL_MIN_GREEN,
    WX_GL_MIN_BLUE,
    WX_GL_MIN_ALPHA,
    WX_GL_DEPTH_SIZE,
    WX_GL_STENCIL_SIZE,
    WX_GL_MIN_ACCUM_RED,
    WX_GL_MIN_ACCUM_GREEN,
    WX_GL_MIN_ACCUM_BLUE,
    WX_GL_MIN_ACCUM_ALPHA,
    WX_GL_SAMPLE_BUFFERS,
    WX_GL_SAMPLES,
    WX_GL_FRAMEBUFFER_SRGB,
    WX_GL_MAJOR_VERSION,
    WX_GL_MINOR_VERSION,
    WX_GL_CORE_PROFILE,
    wx_GL_COMPAT_PROFILE,
    WX_GL_FORWARD_COMPAT,
    WX_GL_ES2,
    WX_GL_DEBUG,
    WX_GL_ROBUST_ACCESS,
    WX_GL_NO_RESET_NOTIFY,
    WX_GL_LOSE_ON_RESET,
    WX_GL_RESET_ISOLATION,
    WX_GL_RELEASE_FLUSH,
    WX_GL_RELEASE_NONE
};

#define wxGLCanvasName wxT("GLCanvas")

struct glsoft_ctx;
class WXDLLIMPEXP_FWD_CORE wxGLCanvas;

class wxGLContext : public wxObject
{
public:
    wxGLContext(wxGLCanvas *win, const wxGLContext *other = NULL, const void *ctxAttrs = NULL);
    virtual ~wxGLContext();

    bool IsOK() const { return m_ctx != NULL; }
    bool SetCurrent(const wxGLCanvas& win) const;

    struct glsoft_ctx *GLSoftContext() const { return m_ctx; }

private:
    struct glsoft_ctx *m_ctx;

    wxDECLARE_NO_COPY_CLASS(wxGLContext);
};

class wxGLCanvas : public wxWindow
{
public:
    wxGLCanvas() {}
    wxGLCanvas(wxWindow *parent, wxWindowID id = wxID_ANY, const int *attribList = NULL,
               const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
               long style = 0, const wxString& name = wxGLCanvasName,
               const wxPalette& palette = wxNullPalette);
    virtual ~wxGLCanvas();

    bool Create(wxWindow *parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize, long style = 0, const wxString& name = wxGLCanvasName,
                const int *attribList = NULL, const wxPalette& palette = wxNullPalette);

    bool SetCurrent(const wxGLContext& context) const;
    bool SwapBuffers();
    bool IsOK() const { return true; }

    static bool IsDisplaySupported(const int *WXUNUSED(attribList)) { return true; }
    static bool IsExtensionSupported(const char *WXUNUSED(extension)) { return false; }

    // GLSoft's own: the picture GL draws into, made the window's size
    void GLSoftMakeTarget() const;

private:
    mutable wxBitmap m_picture;     // A R G B, what GL draws into

    wxDECLARE_CLASS(wxGLCanvas);
    wxDECLARE_NO_COPY_CLASS(wxGLCanvas);
};

#endif // _WX_GLCANVAS_GLSOFT_H_
