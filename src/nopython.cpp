// Golly's Python scripting (gui-wx/wxpython.cpp) loads a Python library at
// run time and needs Python.h to build; AmigaOS 3 has neither, so this
// takes its place: a Python script says so and stops.
// Copyright (c) 2026 AmiCradle Team
#include "wx/wx.h"
#include "wxutils.h"        // for Warning
#include "wxpython.h"

void RunPythonScript(const wxString& filepath)
{
    Warning(_("Python scripts can't run in this version of Golly (there is no Python for AmigaOS 3).\n"
              "Lua scripts can: Golly's Scripts drawer has Lua versions of most of them.\n\n") + filepath);
}

void AbortPythonScript()
{
}

void FinishPythonScripting()
{
}
