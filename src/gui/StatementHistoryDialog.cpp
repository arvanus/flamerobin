/*
Copyright (c) 2004, 2005, 2006 The FlameRobin Development Team

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


  $Id$

*/
//-----------------------------------------------------------------------------
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif //WX_PRECOMP

#include "statementHistory.h"
#include "styleguide.h"
#include "ExecuteSqlFrame.h"
#include "StatementHistoryDialog.h"
//-----------------------------------------------------------------------------
StatementHistoryDialog::StatementHistoryDialog(ExecuteSqlFrame *parent,
    StatementHistory *history, const wxString& title)
    :BaseDialog(parent, -1, title), historyM(history),
     isSearchingM(false)
{
    wxBoxSizer *innerSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *topSizer = new wxBoxSizer(wxHORIZONTAL);
    m_staticText2 = new wxStaticText(getControlsPanel(), wxID_ANY,
        _("Search for:"), wxDefaultPosition, wxDefaultSize, 0);
    topSizer->Add(m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT,
        styleguide().getControlLabelMargin());

    textctrl_search = new wxTextCtrl(getControlsPanel(), wxID_ANY, wxT(""));
    button_search = new wxButton(getControlsPanel(), ID_button_search,
        _("&Search"));
    button_delete = new wxButton(getControlsPanel(), ID_button_delete,
        _("&Delete Selected"), wxDefaultPosition, wxDefaultSize, 0);
    topSizer->Add(textctrl_search, 1, wxRIGHT|wxALIGN_CENTER_VERTICAL,
        styleguide().getRelatedControlMargin(wxHORIZONTAL));
    topSizer->Add(button_search, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL,
        styleguide().getUnrelatedControlMargin(wxHORIZONTAL));
    topSizer->Add(button_delete, 0, wxALIGN_CENTER_VERTICAL, 0);
    innerSizer->Add(topSizer, 0, wxEXPAND, 5);

    int gaugeHeight = wxSystemSettings::GetMetric(wxSYS_HSCROLL_Y);
    gauge_progress = new wxGauge(getControlsPanel(), wxID_ANY, 100,
        wxDefaultPosition, wxSize(100, gaugeHeight),
        wxGA_HORIZONTAL | wxGA_SMOOTH);
    innerSizer->Add(gauge_progress, 0, wxTOP|wxEXPAND,
        styleguide().getRelatedControlMargin(wxVERTICAL));

    listbox_search = new wxListBox(getControlsPanel(), ID_listbox_search,
        wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_MULTIPLE);
    innerSizer->Add(listbox_search, 1, wxTOP|wxEXPAND,
        styleguide().getRelatedControlMargin(wxVERTICAL));

    button_copy = new wxButton(getControlsPanel(), ID_button_copy,
        _("C&opy Selection To Editor"), wxDefaultPosition, wxDefaultSize, 0);
    button_cancel = new wxButton(getControlsPanel(), wxID_CANCEL,
        _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0);

    wxSizer* sizerButtons = styleguide().createButtonSizer(
        button_copy, button_cancel);

    // use method in base class to set everything up
    layoutSizers(innerSizer, sizerButtons, true);

    #include "history.xpm"
    wxBitmap bmp = wxBitmap(history_xpm);
    wxIcon icon;
    icon.CopyFromBitmap(bmp);
    SetIcon(icon);

    button_search->SetDefault();
    button_copy->Enable(false);
    button_delete->Enable(false);
    textctrl_search->SetFocus();
    // center on parent
    SetSize(620, 400);
    Centre();
}
//-----------------------------------------------------------------------------
void StatementHistoryDialog::setSearching(bool searching)
{
    isSearchingM = searching;
    if (searching)
    {
        button_delete->Enable(false);
        button_copy->Enable(false);
        button_search->SetLabel(_("&Stop"));
    }
    else
        button_search->SetLabel(_("&Search"));
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(StatementHistoryDialog, BaseDialog)
    EVT_BUTTON(StatementHistoryDialog::ID_button_search,
        StatementHistoryDialog::OnButtonSearchClick)
    EVT_BUTTON(StatementHistoryDialog::ID_button_delete,
        StatementHistoryDialog::OnButtonDeleteClick)
    EVT_BUTTON(StatementHistoryDialog::ID_button_copy,
        StatementHistoryDialog::OnButtonCopyClick)
    EVT_LISTBOX(StatementHistoryDialog::ID_listbox_search,
        StatementHistoryDialog::OnListBoxSelect)
    EVT_LISTBOX_DCLICK(StatementHistoryDialog::ID_listbox_search,
        StatementHistoryDialog::OnListBoxSearchDoubleClick)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void StatementHistoryDialog::OnListBoxSelect(wxCommandEvent& WXUNUSED(event))
{
    wxArrayInt sels;
    bool hasSelection = (listbox_search->GetSelections(sels) > 0);
    button_copy->Enable(hasSelection);
    button_delete->Enable(hasSelection);
}
//-----------------------------------------------------------------------------
void StatementHistoryDialog::OnButtonSearchClick(wxCommandEvent&
    WXUNUSED(event))
{
    if (isSearchingM)
    {
        setSearching(false);
        return;
    }

    // start the search
    listbox_search->Clear();
    wxString searchString = textctrl_search->GetValue().Upper();
    setSearching(true);
    StatementHistory::Position total = historyM->size();
    gauge_progress->SetRange((int)total);
    for (StatementHistory::Position p = total - 1; (int)p >= 0; --p)
    {
        wxYield();
        if (!isSearchingM)
        {
            gauge_progress->SetValue(0);
            return;
        }

        gauge_progress->SetValue((int)(total - p - 1));
        wxString s = historyM->get(p);
        if (searchString.IsEmpty() || s.Upper().Contains(searchString))
        {
            wxString entry;
            entry = (s.Length() > 200) ? s.Mid(0, 200) + wxT("...") : s;
            entry.Replace(wxT("\n"), wxT(" "));
            entry.Replace(wxT("\r"), wxEmptyString);
            listbox_search->Append(entry, (void *)p);
        }
    }
    setSearching(false);
    gauge_progress->SetValue(0);
}
//-----------------------------------------------------------------------------
void StatementHistoryDialog::OnButtonDeleteClick(wxCommandEvent&
    WXUNUSED(event))
{
    wxArrayInt temp;
    if (listbox_search->GetSelections(temp) == 0)
    {
        wxMessageBox(_("Please select items you wish to delete"),
            _("Nothing is selected"),
            wxOK|wxICON_WARNING);
        return;
    }
    wxBusyCursor b;
    std::vector<StatementHistory::Position> vect;
    for (size_t i=0; i<temp.GetCount(); ++i)
    {
        vect.push_back(
            (StatementHistory::Position)listbox_search->GetClientData(
            temp.Item(i)));
    }
    historyM->deleteItems(vect);

    for (size_t i=temp.GetCount()-1; (int)i >= 0; --i)
        listbox_search->Delete(temp.Item(i));
}
//-----------------------------------------------------------------------------
void StatementHistoryDialog::OnButtonCopyClick(wxCommandEvent& WXUNUSED(event))
{
   // it is certain, but who knows...
    ExecuteSqlFrame *f = dynamic_cast<ExecuteSqlFrame *>(GetParent());
    if (!f)
        return;

    wxArrayInt temp;
    if (listbox_search->GetSelections(temp) == 0)
        return;

    wxString sql;
    for (size_t i=0; i<temp.GetCount(); ++i)
    {
        sql += historyM->get(
            (StatementHistory::Position)listbox_search->GetClientData(
                temp.Item(i)))
            + wxT("\n");
    }
    f->setSql(sql);
    EndModal(wxID_OK);
}
//-----------------------------------------------------------------------------
void StatementHistoryDialog::OnListBoxSearchDoubleClick(wxCommandEvent& event)
{
   // it is certain, but who knows...
    ExecuteSqlFrame *f = dynamic_cast<ExecuteSqlFrame *>(GetParent());
    StatementHistory::Position item =
        (StatementHistory::Position)event.GetClientData();
    if (!f || (int)item < 0)
        return;
    f->setSql(historyM->get(item));
    Destroy();
}
//-----------------------------------------------------------------------------

