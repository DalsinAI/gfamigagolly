// Golly's ARexx port, GOLLY.1 (the toolkit's wxARexxPort, on arexx.class).
// Besides the toolkit's QUIT, ACTIVATE, DEACTIVATE, VERSION, HELP and FAULT:
//   OPEN FILE/A     open a pattern, rule or script file; RC 10 when it
//                   cannot be read, RC 5 when the pattern has unsaved changes
//   NEW             a new, empty pattern; RC 5 when there are unsaved changes
//   SAVE FILE/K     save the pattern in FILE (.rle, .mc, .gz: the format
//                   from its name), else in the file it came from; RC 10
//                   when there is no file name or it could not be written
// No command puts up a requester (patch 0003): what one would have said
// goes to the status bar, and the command answers RC 5 or 10.
//   RUN  STOP       start or stop generating
//   STEP            one step (the current step size)
//   RESET           back to the starting generation
//   GETGEN GETPOP   RESULT: the generation, the population
//   GETRULE         RESULT: the current rule
// Copyright (c) 2026 AmiCradle Team
#include "wx/wx.h"
#include "wx/openlayout/arexx.h"
#include "wx/file.h"

#include "bigint.h"
#include "lifealgo.h"
#include "wxgolly.h"
#include "wxmain.h"
#include "wxlayer.h"
#include "wxstatus.h"

#define STR2(arg) #arg
#define STRINGIFY(arg) STR2(arg)

static wxARexxPort *gs_rexx = NULL;

// patch 0003: while this is set, Golly's Note, Warning and SaveChanges put
// up no requester (SaveChanges answers Cancel) and keep the message here
int amigaRexxQuiet = 0;
wxString amigaRexxMessage;
int amigaRexxAsked = 0;     // SaveChanges was asked (and answered Cancel)

namespace {
struct Quiet
{
    Quiet() { amigaRexxQuiet++; amigaRexxMessage.Clear(); amigaRexxAsked = 0; }
    ~Quiet() { amigaRexxQuiet--; }
    bool Said() const { return !amigaRexxMessage.IsEmpty(); }
};
}

void AmigaRexxStart(MainFrame *frame)
{
    if ( gs_rexx )
        return;
    gs_rexx = new wxARexxPort("GOLLY", "Golly " STRINGIFY(VERSION));
    gs_rexx->Add("OPEN", "FILE/A", [frame](wxARexxCommand& c) {
        if ( !wxFileExists(c.Str(0)) )
        {
            c.SetError(10, 205);        // object not found
            return;
        }
        if ( !wxFile::Access(c.Str(0), wxFile::read) )
        {
            c.SetError(10, 222);        // not readable
            return;
        }
        if ( frame->generating )
            frame->StopGenerating();
        Quiet q;
        frame->OpenFile(c.Str(0));
        if ( q.Said() )
        {
            statusptr->DisplayMessage(amigaRexxMessage.BeforeFirst('\n'));
            c.SetError(amigaRexxAsked ? 5 : 10);    // unsaved changes, or unreadable
        }
    });
    gs_rexx->Add("NEW", "", [frame](wxARexxCommand& c) {
        if ( frame->generating )
            frame->StopGenerating();
        Quiet q;
        frame->NewPattern();
        if ( q.Said() )
            c.SetError(5);              // unsaved changes: SAVE them first
    });
    gs_rexx->Add("SAVE", "FILE/K", [frame](wxARexxCommand& c) {
        if ( frame->generating )
        {
            c.SetError(10);             // it is running: STOP first
            return;
        }
        wxString path = c.Has(0) ? c.Str(0) : currlayer->currfile;
        if ( path.IsEmpty() )
        {
            c.SetError(10, 210);        // no file name: give FILE
            return;
        }
        wxString lower = path.Lower();
        wxString format = wxT("rle");
        if ( lower.EndsWith(wxT(".mc")) || lower.EndsWith(wxT(".mc.gz")) )
            format = wxT("mc");
        if ( lower.EndsWith(wxT(".gz")) )
            format += wxT(".gz");
        Quiet q;
        const char *err = frame->SaveFile(path, format, true);
        if ( err )
        {
            statusptr->ErrorMessage(wxString(err, wxConvLocal));
            c.SetError(10);
        }
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
    gs_rexx->Open();
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
