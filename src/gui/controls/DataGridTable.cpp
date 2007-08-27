/*
  Copyright (c) 2004-2007 The FlameRobin Development Team

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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
  #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
  #include "wx/wx.h"
#endif

#include <wx/grid.h>

#include "config/Config.h"
#include "core/FRError.h"
#include "core/StringUtils.h"
#include "gui/controls/DataGridTable.h"
#include "metadata/database.h"
//-----------------------------------------------------------------------------
DataGridTable::DataGridTable(IBPP::Statement& s, Database *db)
    : wxGridTableBase(), statementM(s), databaseM(db)
{
    allRowsFetchedM = false;
    fetchAllRowsM = false;
    config().getValue(wxT("GridFetchAllRecords"), fetchAllRowsM);
    maxRowToFetchM = 100;

    nullAttrM = new wxGridCellAttr();
    nullAttrM->SetTextColour(*wxRED);
    nullAttrM->SetAlignment(wxALIGN_LEFT, wxALIGN_CENTRE);

    nullAttrReadonlyM = new wxGridCellAttr();
    nullAttrReadonlyM->SetTextColour(*wxRED);
    nullAttrReadonlyM->SetAlignment(wxALIGN_LEFT, wxALIGN_CENTRE);
    nullAttrReadonlyM->SetBackgroundColour(wxColour(240, 240, 240));

    nullAttrNumericM = new wxGridCellAttr();
    nullAttrNumericM->SetTextColour(*wxRED);
    nullAttrNumericM->SetAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);

    nullAttrNumericReadonlyM = new wxGridCellAttr();
    nullAttrNumericReadonlyM->SetTextColour(*wxRED);
    nullAttrNumericReadonlyM->SetAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);
    nullAttrNumericReadonlyM->SetBackgroundColour(wxColour(240, 240, 240));
}
//-----------------------------------------------------------------------------
DataGridTable::~DataGridTable()
{
    Clear();
    nullAttrNumericM->DecRef();
    nullAttrNumericReadonlyM->DecRef();
    nullAttrReadonlyM->DecRef();
    nullAttrM->DecRef();
}
//-----------------------------------------------------------------------------
// implementation methods
bool DataGridTable::canFetchMoreRows()
{
    if (allRowsFetchedM || statementM->Type() != IBPP::stSelect)
        return false;
    // could this really happen?
    if (statementM == 0)
        return false;
    // there should be a better way here...
    IBPP::Transaction tran = statementM->TransactionPtr();
    return (tran != 0 && tran->Started());
}
//-----------------------------------------------------------------------------
void DataGridTable::Clear()
{
    FR_TRY

    allRowsFetchedM = true;
    fetchAllRowsM = false;
    config().getValue(wxT("GridFetchAllRecords"), fetchAllRowsM);

    unsigned oldCols = rowsM.getRowFieldCount();
    unsigned oldRows = rowsM.getRowCount();
    rowsM.clear();

    if (GetView() && oldRows > 0)
    {
        wxGridTableMessage rowMsg(this, wxGRIDTABLE_NOTIFY_ROWS_DELETED,
            0, oldRows);
        GetView()->ProcessTableMessage(rowMsg);
    }
    if (GetView() && oldCols > 0)
    {
        wxGridTableMessage colMsg(this, wxGRIDTABLE_NOTIFY_COLS_DELETED,
            0, oldCols);
        GetView()->ProcessTableMessage(colMsg);
    }

    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGridTable::fetch()
{
    if (!canFetchMoreRows())
        return;

    // fetch the first 100 rows no matter how long it takes
    unsigned oldRows = rowsM.getRowCount();
    bool initial = oldRows == 0;
    // fetch more rows until maxRowToFetchM reached or 100 ms elapsed
    wxLongLong startms = ::wxGetLocalTimeMillis();
    do
    {
        try
        {
            if (!statementM->Fetch())
                allRowsFetchedM = true;
        }
        catch (IBPP::Exception& e)
        {
            allRowsFetchedM = true;
            ::wxMessageBox(std2wx(e.ErrorMessage()),
                _("An IBPP error occurred."), wxOK|wxICON_ERROR);
        }
        catch (...)
        {
            allRowsFetchedM = true;
            ::wxMessageBox(_("A system error occurred!"), _("Error"),
                wxOK|wxICON_ERROR);
        }
        if (allRowsFetchedM)
            break;
        rowsM.addRow(statementM, charsetConverterM);

        if (!initial && (::wxGetLocalTimeMillis() - startms > 100))
            break;
    }
    while ((fetchAllRowsM && !initial) || rowsM.getRowCount() < maxRowToFetchM);

    if (rowsM.getRowCount() > oldRows && GetView())   // notify the grid
    {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED,
            rowsM.getRowCount() - oldRows);
        GetView()->ProcessTableMessage(msg);
        // used in frame to update status bar
        wxCommandEvent evt(wxEVT_FRDG_ROWCOUNT_CHANGED, GetView()->GetId());
        evt.SetExtraLong(rowsM.getRowCount());
        wxPostEvent(GetView(), evt);
    }
}
//-----------------------------------------------------------------------------
wxGridCellAttr* DataGridTable::GetAttr(int row, int col,
    wxGridCellAttr::wxAttrKind kind)
{
    FR_TRY

    if (rowsM.isFieldNull(row, col))
    {
        if (rowsM.isRowFieldNumeric(col))
        {
            if (rowsM.isColumnReadonly(col))
            {
                nullAttrNumericReadonlyM->IncRef();
                return nullAttrNumericReadonlyM;
            }
            else
            {
                nullAttrNumericM->IncRef();
                return nullAttrNumericM;
            }
        }
        if (rowsM.isColumnReadonly(col))
        {
            nullAttrReadonlyM->IncRef();
            return nullAttrReadonlyM;
        }
        nullAttrM->IncRef();
        return nullAttrM;
    }
    return wxGridTableBase::GetAttr(row, col, kind);

    FR_CATCH
}
//-----------------------------------------------------------------------------
wxString DataGridTable::getCellValue(int row, int col)
{
    if (!isValidCellPos(row, col))
        return wxEmptyString;

    if (rowsM.isFieldNull(row, col))
        return wxT("[null]");
    return rowsM.getFieldValue(row, col);
}
//-----------------------------------------------------------------------------
wxString DataGridTable::getCellValueForInsert(int row, int col)
{
    if (!isValidCellPos(row, col))
        return wxEmptyString;

    if (rowsM.isFieldNull(row, col))
        return wxT("NULL");
    // return quoted text, but escape embedded quotes
    wxString s(rowsM.getFieldValue(row, col));
    s.Replace(wxT("'"), wxT("''"));
    return wxT("'") + s + wxT("'");
}
//-----------------------------------------------------------------------------
wxString DataGridTable::getCellValueForCSV(int row, int col)
{
    if (!isValidCellPos(row, col))
        return wxEmptyString;

    if (rowsM.isFieldNull(row, col))
        return wxT("\"NULL\"");
    wxString s(rowsM.getFieldValue(row, col));
    if (rowsM.isRowFieldNumeric(col))
        return s;

    // return quoted text, but escape embedded quotes
    s.Replace(wxT("\""), wxT("\"\""));
    return wxT("\"") + s + wxT("\"");
}
//-----------------------------------------------------------------------------
wxString DataGridTable::GetColLabelValue(int col)
{
    FR_TRY

    return rowsM.getRowFieldName(col);

    FR_CATCH
}
//-----------------------------------------------------------------------------
bool DataGridTable::getFetchAllRows()
{
    return fetchAllRowsM;
}
//-----------------------------------------------------------------------------
int DataGridTable::GetNumberCols()
{
    FR_TRY

    return rowsM.getRowFieldCount();

    FR_CATCH
}
//-----------------------------------------------------------------------------
int DataGridTable::GetNumberRows()
{
    FR_TRY

    return rowsM.getRowCount();

    FR_CATCH
}
//-----------------------------------------------------------------------------
wxString DataGridTable::getTableName()
{
    // TODO: using one table is not correct for JOINs or sub-SELECTs, so we
    //       should build separate statements for each table
    //       DataGridRows::statementTablesM contains that list
    //       (together with PK/UNQ info)
    if (statementM == 0 || statementM->Columns() == 0)
        return wxEmptyString;
    return std2wx(statementM->ColumnTable(1));
}
//-----------------------------------------------------------------------------
wxString DataGridTable::GetValue(int row, int col)
{
    FR_TRY

    if (!isValidCellPos(row, col))
        return wxEmptyString;

    // keep between 200 and 250 more rows fetched for better responsiveness
    // (but make the count of fetched rows a multiple of 50)
    unsigned maxRowToFetch = 50 * (row / 50 + 5);
    if (maxRowToFetchM < maxRowToFetch)
        maxRowToFetchM = maxRowToFetch;

    if (rowsM.isFieldNull(row, col))
        return wxT("[null]");
    wxString cellValue(rowsM.getFieldValue(row, col));

    // return first line of multi-line string only
    int nl = cellValue.Find(wxT("\n"));
    if (nl != wxNOT_FOUND)
    {
        cellValue.Truncate(nl);
        cellValue.Trim();
        cellValue += wxT(" [...]"); // and show that there is more data...
    }
    return cellValue;

    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGridTable::initialFetch(wxMBConv* conv)
{
    Clear();
    allRowsFetchedM = false;
    maxRowToFetchM = 100;

    if (conv)
        charsetConverterM = conv;
    else
        charsetConverterM = wxConvCurrent;

    try
    {
        rowsM.initialize(statementM, databaseM);
    }
    catch (IBPP::Exception& e)
    {
        ::wxMessageBox(std2wx(e.ErrorMessage()),
            _("An IBPP error occurred."), wxOK | wxICON_ERROR);
    }
    catch (...)
    {
        ::wxMessageBox(_("A system error occurred!"), _("Error"),
            wxOK | wxICON_ERROR);
    }

    if (GetView())
    {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_COLS_APPENDED,
            rowsM.getRowFieldCount());
        GetView()->ProcessTableMessage(msg);
    }
    fetch();
}
//-----------------------------------------------------------------------------
bool DataGridTable::IsEmptyCell(int row, int col)
{
    FR_TRY

    return !isValidCellPos(row, col);

    FR_CATCH
}
//-----------------------------------------------------------------------------
bool DataGridTable::isNullCell(int row, int col)
{
    return rowsM.isFieldNull(row, col);
}
//-----------------------------------------------------------------------------
bool DataGridTable::isNumericColumn(int col)
{
    return rowsM.isRowFieldNumeric(col);
}
//-----------------------------------------------------------------------------
bool DataGridTable::isReadonlyColumn(int col)
{
    return rowsM.isColumnReadonly(col);
}
//-----------------------------------------------------------------------------
bool DataGridTable::isValidCellPos(int row, int col)
{
    return (row >= 0 && col >= 0 && row < (int)rowsM.getRowCount()
        && col < (int)rowsM.getRowFieldCount());
}
//-----------------------------------------------------------------------------
bool DataGridTable::needsMoreRowsFetched()
{
    if (allRowsFetchedM)
        return false;
    // true if all rows are to be fetched, or more rows should be cached
    // for more responsive grid scrolling
    return (fetchAllRowsM || rowsM.getRowCount() < maxRowToFetchM);
}
//-----------------------------------------------------------------------------
void DataGridTable::setFetchAllRecords(bool fetchall)
{
    fetchAllRowsM = fetchall;
}
//-----------------------------------------------------------------------------
void DataGridTable::SetValue(int row, int col, const wxString& value)
{
    FR_TRY

    wxString statement = rowsM.setFieldValue(row, col, value);

    // used in frame to show executed statements
    wxCommandEvent evt(wxEVT_FRDG_STATEMENT, GetView()->GetId());
    evt.SetString(statement);
    wxPostEvent(GetView(), evt);

    FR_CATCH
}
//-----------------------------------------------------------------------------
bool DataGridTable::DeleteRows(size_t pos, size_t numRows)
{
    FR_TRY

    // remove rows from internal storage
    wxString statement;
    if (!rowsM.removeRows(pos, numRows, statement))
        return false;

    // notify visual control
    if (GetView() && numRows > 0)
    {
        wxGridTableMessage rowMsg(this, wxGRIDTABLE_NOTIFY_ROWS_DELETED,
            pos, numRows);
        GetView()->ProcessTableMessage(rowMsg);

        // used in frame to update status bar
        wxCommandEvent evt(wxEVT_FRDG_ROWCOUNT_CHANGED, GetView()->GetId());
        evt.SetExtraLong(rowsM.getRowCount());
        wxPostEvent(GetView(), evt);

        // used in frame to show executed statements
        wxCommandEvent evt2(wxEVT_FRDG_STATEMENT, GetView()->GetId());
        evt2.SetString(statement);
        wxPostEvent(GetView(), evt2);
    }

    FR_CATCH
}
//-----------------------------------------------------------------------------
DEFINE_EVENT_TYPE(wxEVT_FRDG_ROWCOUNT_CHANGED)
DEFINE_EVENT_TYPE(wxEVT_FRDG_STATEMENT)
//-----------------------------------------------------------------------------
