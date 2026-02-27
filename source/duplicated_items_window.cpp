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

#include "duplicated_items_window.h"
#include "map.h"
#include "tile.h"
#include "item.h"
#include "editor.h"

DuplicatedItemsWindow::DuplicatedItemsWindow(wxWindow* parent) :
	wxPanel(parent, wxID_ANY)
{
	wxSize icon_size = FROM_DIP(parent, wxSize(16, 16));
	wxBitmap save_bitmap = wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR, icon_size);

	wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	items_list = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(200, 330), 0, nullptr, wxLB_SINGLE | wxLB_ALWAYS_SB);
	sizer->Add(items_list, wxSizerFlags(1).Expand());

	wxSizer* buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

	remove_button = new wxButton(this, wxID_DELETE, "Remove");
	remove_button->Enable(false);
	remove_all_button = new wxButton(this, wxID_DELETE, "Remove All");
	remove_all_button->Enable(false);
	export_button = new wxBitmapButton(this, wxID_ANY, save_bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW);
	export_button->SetToolTip("Export");
	export_button->Enable(false);

	buttonsSizer->Add(remove_button, wxSizerFlags(0).Center());
	buttonsSizer->AddSpacer(10);
	buttonsSizer->Add(remove_all_button, wxSizerFlags(0).Center());
	buttonsSizer->AddSpacer(20);
	buttonsSizer->Add(export_button, wxSizerFlags(0).Center());
	sizer->Add(buttonsSizer, wxSizerFlags(0).Center().DoubleBorder());

	SetSizerAndFit(sizer);

	items_list->Bind(wxEVT_LISTBOX, &DuplicatedItemsWindow::OnClickResult, this);
	remove_button->Bind(wxEVT_BUTTON, &DuplicatedItemsWindow::OnClickRemove, this);
	remove_all_button->Bind(wxEVT_BUTTON, &DuplicatedItemsWindow::OnClickRemoveAll, this);
	export_button->Bind(wxEVT_BUTTON, &DuplicatedItemsWindow::OnClickExport, this);
}

DuplicatedItemsWindow::~DuplicatedItemsWindow()
{
	Clear();
}

void DuplicatedItemsWindow::StartSearch(bool selection)
{
	Clear();

	g_editor.CreateLoadBar(wxString::Format("Searching on %s...", selection ? "selected area" : "map"));

	double nextUpdate = 0.0;
	std::vector<DuplicatedItem*> duplicates;
	g_editor.map.forEachTile(
		[&nextUpdate, &duplicates, selection](Tile *tile, double progress){
			if(progress >= nextUpdate){
				g_editor.SetLoadDone((int)(progress * 100.0));
				nextUpdate = progress + 0.01;
			}

			if(selection && !tile->isSelected()){
				return;
			}

			int count  = 0;
			int prevId = 0;
			for(const Item *item = tile->items; item != NULL; item = item->next){
				if(item->typeId == prevId){
					count += 1;
				}else if(count > 0){
					duplicates.push_back(newd DuplicatedItem(tile->pos, prevId, count));
					count = 0;
				}
			}

			if(count > 0){
				duplicates.push_back(newd DuplicatedItem(tile->pos, prevId, count));
			}
		});

	g_editor.DestroyLoadBar();

	for(DuplicatedItem *item: duplicates) {
		items_list->Append(
				wxString::Format("item: %d, count: %d, pos: (%d,%d,%d)",
						item->itemId, item->count, item->position.x,
						item->position.y, item->position.z),
				item);
	}

	UpdateButtons();
}

void DuplicatedItemsWindow::Clear()
{
	uint32_t count = items_list->GetCount();
	if(count == 0) {
		return;
	}

	for(uint32_t i = 0; i < count; ++i) {
		delete reinterpret_cast<DuplicatedItem*>(items_list->GetClientData(i));
	}

	items_list->Clear();
	UpdateButtons();
}

void DuplicatedItemsWindow::OnClickResult(wxCommandEvent& event)
{
	DuplicatedItem* data = reinterpret_cast<DuplicatedItem*>(event.GetClientData());
	if(data) {
		g_editor.SetScreenCenterPosition(data->position);
		remove_button->Enable(true);
	} else {
		remove_button->Enable(false);
	}
}

void DuplicatedItemsWindow::OnClickRemove(wxCommandEvent& WXUNUSED(event))
{
	int32_t index = items_list->GetSelection();
	if (index == wxNOT_FOUND) {
		return;
	}

	DuplicatedItem *data = reinterpret_cast<DuplicatedItem*>(items_list->GetClientData(index));
	ActionGroup *group = g_editor.actionQueue.createGroup(ACTION_DELETE_TILES);
	{
		Action *action = group->createAction();
		removeItem(data, action);
		action->commit();
	}

	g_editor.updateActions();

	items_list->Delete(index);
	delete data;
	UpdateButtons();
}

void DuplicatedItemsWindow::OnClickRemoveAll(wxCommandEvent& WXUNUSED(event))
{
	int count = (int)items_list->GetCount();
	if (count == 0) {
		return;
	}

	g_editor.CreateLoadBar("Removing items...");
	ActionGroup *group = g_editor.actionQueue.createGroup(ACTION_DELETE_TILES);
	{
		Action *action = group->createAction();
		for(int i = 0; i < count; i += 1) {
			DuplicatedItem* data = reinterpret_cast<DuplicatedItem*>(items_list->GetClientData(i));
			removeItem(data, action);
			g_editor.SetLoadScale(i, count);
		}
		action->commit();
	}

	// TODO(fusion): Remove entries from items_list?

	g_editor.updateActions();
	g_editor.DestroyLoadBar();
	Clear();
}

void DuplicatedItemsWindow::OnClickExport(wxCommandEvent& WXUNUSED(event))
{
	wxFileDialog dialog(this, "Save file...", "", "", "Text Documents (*.txt) | *.txt", wxFD_SAVE);
	if(dialog.ShowModal() == wxID_OK) {
		wxFile file(dialog.GetPath(), wxFile::write);
		if(file.IsOpened()) {
			g_editor.CreateLoadBar("Exporting result...");

			file.Write(wxString() << "Generated by " << __RME_APPLICATION_NAME__ << " version " << __RME_VERSION__);
			file.Write("\n=============================================\n\n");
			wxArrayString lines = items_list->GetStrings();
			size_t count = lines.Count();
			for(size_t i = 0; i < count; ++i) {
				file.Write(lines[i] + "\n");
				g_editor.SetLoadScale((int32_t)i, (int32_t)count);
			}
			file.Close();

			g_editor.DestroyLoadBar();
		}
	}
}

void DuplicatedItemsWindow::UpdateButtons()
{
	if(!IsShownOnScreen()) {
		return;
	}

	bool enable = items_list->GetCount() != 0;
	remove_button->Enable(enable);
	remove_all_button->Enable(enable);
	export_button->Enable(enable);
}

bool DuplicatedItemsWindow::removeItem(DuplicatedItem *data, Action *action)
{
	Tile *tile = g_editor.map.getTile(data->position);
	if(!tile) {
		return false;
	}

	int count = 0;
	Tile newTile; newTile.deepCopy(*tile);
	newTile.removeItems(
		[&count, data](const Item *item){
			return item->getID() == data->itemId && ++count > 1;
		});
	action->changeTile(std::move(newTile));
	return true;
}
