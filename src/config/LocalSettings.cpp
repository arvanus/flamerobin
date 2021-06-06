/*
  Copyright (c) 2004-2021 The FlameRobin Development Team

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "Config.h"
#include "LocalSettings.h"



LocalSettings::LocalSettings()
{
    wxLocale* locale;
    locale = new wxLocale();
    if (config().getUseLocalConfig())
        locale->Init(wxLANGUAGE_DEFAULT);
    else
        locale->Init(wxLANGUAGE_ENGLISH);
}

LocalSettings::~LocalSettings()
{
    wxLocale* locale;
    locale = new wxLocale();
    if (config().getUseLocalConfig())
        locale->Init(wxLANGUAGE_DEFAULT);
    else
        locale->Init(wxLANGUAGE_ENGLISH);
}


void LocalSettings::setDataBaseLenguage()
{
    wxLocale* locale;
    locale = new wxLocale();
    locale->Init(wxLANGUAGE_ENGLISH);
}