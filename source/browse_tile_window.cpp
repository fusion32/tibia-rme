//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "map.h"
#include "editor.h"
#include "raw_brush.h"
#include "tile.h"
#include "graphics.h"
#include "browse_tile_window.h"

// ============================================================================
//

class BrowseTileListBox : public wxVListBox
{
public:
	BrowseTileListBox(wxWindow* parent, wxWindowID id, Tile* tile_);

	void OnDrawItem(wxDC& dc, const wxRect& rect, size_t index) const override;
	wxCoord OnMeasureItem(size_t index) const override;

	Item* GetSelectedItem();
	void RemoveSelected();

protected:
	Tile *tile;
};

BrowseTileListBox::BrowseTileListBox(wxWindow* parent, wxWindowID id, Tile* tile_) :
	wxVListBox(parent, id, wxDefaultPosition, wxSize(200, 180), wxLB_MULTIPLE), tile(tile_)
{
	SetItemCount(tile->countItems());
}

void BrowseTileListBox::OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const
{
	Item *item = tile->getItemAt((int)n);
	if(!item){
		return;
	}

	Sprite* sprite = g_editor.gfx.getSprite(item->getLookID());
	if(sprite)
		sprite->DrawTo(&dc, SPRITE_SIZE_32x32, rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());

	if(IsSelected(n)) {
		item->select();
		if(HasFocus())
			dc.SetTextForeground(wxColor(0xFF, 0xFF, 0xFF));
		else
			dc.SetTextForeground(wxColor(0x00, 0x00, 0xFF));
	} else {
		item->deselect();
		dc.SetTextForeground(wxColor(0x00, 0x00, 0x00));
	}

	wxString label;
	label << item->getID() << " - " << item->getName();
	dc.DrawText(label, rect.GetX() + 40, rect.GetY() + 6);
}

wxCoord BrowseTileListBox::OnMeasureItem(size_t n) const
{
	return 32;
}

Item* BrowseTileListBox::GetSelectedItem()
{
	if(GetItemCount() == 0 || GetSelectedCount() == 0)
		return nullptr;

	return tile->getLastSelectedItem();
}

void BrowseTileListBox::RemoveSelected()
{
	if(GetItemCount() == 0 || GetSelectedCount() == 0) return;

	Clear();

	Item *items = tile->popSelectedItems();
	while(Item *it = items){
		items = it->next;
		it->next = NULL;
		delete it;
	}

	SetItemCount(tile->countItems());
	Refresh();
}

// ============================================================================
//

BEGIN_EVENT_TABLE(BrowseTileWindow, wxDialog)
	EVT_BUTTON(wxID_REMOVE, BrowseTileWindow::OnClickDelete)
	EVT_BUTTON(wxID_FIND, BrowseTileWindow::OnClickSelectRaw)
	EVT_BUTTON(wxID_OK, BrowseTileWindow::OnClickOK)
	EVT_BUTTON(wxID_CANCEL, BrowseTileWindow::OnClickCancel)
END_EVENT_TABLE()

BrowseTileWindow::BrowseTileWindow(wxWindow* parent, Tile* tile, wxPoint position /* = wxDefaultPosition */) :
wxDialog(parent, wxID_ANY, "Browse Field", position, wxSize(600, 400), wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER)
{
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	item_list = newd BrowseTileListBox(this, wxID_ANY, tile);
	sizer->Add(item_list, wxSizerFlags(1).Expand());

	wxString pos, sec;
	pos << "(" << tile->pos.x << ", " << tile->pos.y << ", " << tile->pos.z << ")";
	sec << (tile->pos.x / MAP_SECTOR_SIZE) << "-" << (tile->pos.y / MAP_SECTOR_SIZE) << "-" << tile->pos.z
			<< ": " << (tile->pos.x & MAP_SECTOR_MASK) << "-" << (tile->pos.y & MAP_SECTOR_MASK);

	wxSizer* infoSizer = newd wxBoxSizer(wxVERTICAL);
    wxBoxSizer* buttons = newd wxBoxSizer(wxHORIZONTAL);
	delete_button = newd wxButton(this, wxID_REMOVE, "Delete");
	delete_button->Enable(false);
	buttons->Add(delete_button);
	buttons->AddSpacer(5);
	select_raw_button = newd wxButton(this, wxID_FIND, "Select RAW");
	select_raw_button->Enable(false);
	buttons->Add(select_raw_button);
	infoSizer->Add(buttons);
	infoSizer->AddSpacer(5);
	infoSizer->Add(newd wxStaticText(this, wxID_ANY, "Position:  " + pos), wxSizerFlags(0).Left());
	infoSizer->Add(newd wxStaticText(this, wxID_ANY, "Sector:  " + sec), wxSizerFlags(0).Left());
	infoSizer->Add(item_count_txt = newd wxStaticText(this, wxID_ANY, "Item count:  " + i2ws(item_list->GetItemCount())), wxSizerFlags(0).Left());
	infoSizer->Add(newd wxStaticText(this, wxID_ANY, "Refresh:  " + b2yn(tile->getTileFlag(TILE_FLAG_REFRESH))), wxSizerFlags(0).Left());
	infoSizer->Add(newd wxStaticText(this, wxID_ANY, "No logout:  " + b2yn(tile->getTileFlag(TILE_FLAG_NOLOGOUT))), wxSizerFlags(0).Left());
	infoSizer->Add(newd wxStaticText(this, wxID_ANY, "Protection zone:  " + b2yn(tile->getTileFlag(TILE_FLAG_PROTECTIONZONE))), wxSizerFlags(0).Left());
	infoSizer->Add(newd wxStaticText(this, wxID_ANY, "House:  " + b2yn(tile->houseId != 0)), wxSizerFlags(0).Left());

	sizer->Add(infoSizer, wxSizerFlags(0).Left().DoubleBorder());

	// OK/Cancel buttons
	wxSizer* btnSizer = newd wxBoxSizer(wxHORIZONTAL);
	btnSizer->Add(newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(0).Center());
	btnSizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(0).Center());
	sizer->Add(btnSizer, wxSizerFlags(0).Center().DoubleBorder());

	SetSizerAndFit(sizer);

	// Connect Events
	item_list->Connect(wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler(BrowseTileWindow::OnItemSelected), NULL, this);
}

BrowseTileWindow::~BrowseTileWindow()
{
	// Disconnect Events
	item_list->Disconnect(wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler(BrowseTileWindow::OnItemSelected), NULL, this);
}

void BrowseTileWindow::OnItemSelected(wxCommandEvent& WXUNUSED(event))
{
	const size_t count = item_list->GetSelectedCount();
	delete_button->Enable(count != 0);
	select_raw_button->Enable(count == 1);
}

void BrowseTileWindow::OnClickDelete(wxCommandEvent& WXUNUSED(event))
{
	item_list->RemoveSelected();
	item_count_txt->SetLabelText("Item count:  " + i2ws(item_list->GetItemCount()));
}

void BrowseTileWindow::OnClickSelectRaw(wxCommandEvent& WXUNUSED(event))
{
	Item* item = item_list->GetSelectedItem();
	if(item && item->getRAWBrush())
		g_editor.SelectBrush(item->getRAWBrush(), TILESET_RAW);

	EndModal(1);
}

void BrowseTileWindow::OnClickOK(wxCommandEvent& WXUNUSED(event))
{
	EndModal(1);
}

void BrowseTileWindow::OnClickCancel(wxCommandEvent& WXUNUSED(event))
{
	EndModal(0);
}

