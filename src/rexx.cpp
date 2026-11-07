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

void AmigaRexxDiag(const char *what) { diag(what); }

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
