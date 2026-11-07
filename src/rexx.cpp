// Golly's ARexx port, GOLLY.1 (the toolkit's wxARexxPort, on arexx.class).
// Besides the toolkit's QUIT, ACTIVATE, DEACTIVATE, VERSION, HELP and FAULT:
//   OPEN FILE/A     open a pattern, rule or script file
//   NEW             a new, empty pattern
//   SAVE            save the pattern (asks for a name, as File > Save does)
//   RUN  STOP       start or stop generating
//   STEP            one step (the current step size)
//   RESET           back to the starting generation
//   GETGEN GETPOP   RESULT: the generation, the population
//   GETRULE         RESULT: the current rule
// Copyright (c) 2026 AmiCradle Team
#include "wx/wx.h"
#include "wx/openlayout/arexx.h"

#include "bigint.h"
#include "lifealgo.h"
#include "wxgolly.h"
#include "wxmain.h"
#include "wxlayer.h"

#define STR2(arg) #arg
#define STRINGIFY(arg) STR2(arg)

static wxARexxPort *gs_rexx = NULL;

#include <stdio.h>
static void diag(const char *what)
{
    FILE *f = fopen("RAM:golly-diag.txt", "a");
    if ( f )
    {
        fprintf(f, "%s\n", what);
        fclose(f);
    }
}

// every event wx dispatches, while the diagnosis runs: the last one logged
// before a hang is the handler it hangs in
class DiagFilter : public wxEventFilter
{
public:
    int FilterEvent(wxEvent& e) wxOVERRIDE
    {
        if ( !s_on )
            return Event_Skip;
        char line[160];
        wxObject *o = e.GetEventObject();
        const wxClassInfo *ci = o ? o->GetClassInfo() : NULL;
        snprintf(line, sizeof line, "  event %d on %s %p", (int)e.GetEventType(),
                 ci ? (const char *)wxString(ci->GetClassName()).mb_str() : "?", (void *)o);
        diag(line);
        return Event_Skip;
    }
    static bool s_on;
};
bool DiagFilter::s_on = false;
static DiagFilter *gs_filter = NULL;

void AmigaRexxDiag(const char *what)
{
    if ( !gs_filter )
    {
        gs_filter = new DiagFilter;
        wxEvtHandler::AddFilter(gs_filter);
    }
    diag(what);
}

// scroll bars made step by step, on the frame, on a fresh plain child of
// the right pane, and on the right pane itself, with every event logged
void AmigaRexxProbe(wxWindow *frame, wxWindow *pane)
{
    AmigaRexxDiag("probe: events on");
    DiagFilter::s_on = true;
    diag("probe 1: new wxScrollBar() (no Create)");
    wxScrollBar *a = new wxScrollBar();
    diag("probe 1: Create on the frame");
    a->Create(frame, wxID_ANY, wxPoint(0, 0), wxSize(-1, 15), wxSB_HORIZONTAL);
    diag("probe 1: made; Destroy");
    a->Destroy();
    diag("probe 2: plain wxWindow in the pane");
    wxWindow *w = new wxWindow(pane, wxID_ANY, wxPoint(0, 0), wxSize(40, 40));
    diag("probe 2: scroll bar on it");
    wxScrollBar *b = new wxScrollBar(w, wxID_ANY, wxPoint(0, 0), wxSize(-1, 15), wxSB_HORIZONTAL);
    diag("probe 2: made; Destroy both");
    b->Destroy();
    w->Destroy();
    diag("probe 3: wxControl on the pane");
    wxControl *c = new wxControl(pane, wxID_ANY, wxPoint(0, 0), wxSize(20, 15));
    diag("probe 3: made; Destroy");
    c->Destroy();
    diag("probe 4: Create on the pane, two-phase");
    wxScrollBar *d = new wxScrollBar();
    d->Create(pane, wxID_ANY, wxPoint(0, 0), wxSize(-1, 15), wxSB_HORIZONTAL);
    diag("probe 4: made; Destroy");
    d->Destroy();
    diag("probe done; Golly's own bars next");
}

void AmigaRexxStart(MainFrame *frame)
{
    diag("AmigaRexxStart: MainFrame made");
    if ( gs_rexx )
        return;
    gs_rexx = new wxARexxPort("GOLLY", "Golly " STRINGIFY(VERSION));
    gs_rexx->Add("OPEN", "FILE/A", [frame](wxARexxCommand& c) {
        if ( !wxFileExists(c.Str(0)) )
        {
            c.SetError(10, 205);        // object not found
            return;
        }
        frame->OpenFile(c.Str(0));
    });
    gs_rexx->Add("NEW", "", [frame](wxARexxCommand&) { frame->NewPattern(); });
    gs_rexx->Add("SAVE", "", [frame](wxARexxCommand& c) {
        if ( !frame->SavePattern() )
            c.SetError(5);
    });
    gs_rexx->Add("RUN", "", [frame](wxARexxCommand&) {
        if ( !frame->generating )
            frame->StartGenerating();
    });
    gs_rexx->Add("STOP", "", [frame](wxARexxCommand&) {
        if ( frame->generating )
            frame->StopGenerating();
    });
    gs_rexx->Add("STEP", "", [frame](wxARexxCommand& c) {
        if ( frame->generating )
        {
            c.SetError(5);              // it is running: STOP first
            return;
        }
        frame->NextGeneration(true);
    });
    gs_rexx->Add("RESET", "", [frame](wxARexxCommand&) {
        if ( frame->generating )
            frame->StopGenerating();
        frame->ResetPattern();
    });
    gs_rexx->Add("GETGEN", "", [](wxARexxCommand& c) {
        c.SetResult(currlayer->algo->getGeneration().tostring(0));
    });
    gs_rexx->Add("GETPOP", "", [](wxARexxCommand& c) {
        c.SetResult(currlayer->algo->getPopulation().tostring(0));
    });
    gs_rexx->Add("GETRULE", "", [](wxARexxCommand& c) {
        c.SetResult(currlayer->algo->getrule());
    });
    diag("commands added");
    diag(gs_rexx->Open() ? "port open" : "port NOT open");
    frame->CallAfter([]() { diag("main loop runs"); });
    // the port goes with the main window, before wx closes down
    frame->Bind(wxEVT_DESTROY, [frame](wxWindowDestroyEvent& e) {
        if ( e.GetEventObject() == frame )
        {
            delete gs_rexx;
            gs_rexx = NULL;
        }
        e.Skip();
    });
}
