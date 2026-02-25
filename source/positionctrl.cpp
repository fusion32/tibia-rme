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
#include "positionctrl.h"
#include "numberctrl.h"
#include "position.h"

PositionCtrl::PositionCtrl(wxWindow* parent, Position min, Position max, Position pos)
	: wxControl(parent, wxID_ANY)
{
	wxSizer *sizer = newd wxBoxSizer(wxHORIZONTAL);

	x_field = newd NumberCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(60, 20), wxTE_PROCESS_ENTER, min.x, max.x, pos.x, "X");
	x_field->Bind(wxEVT_TEXT_PASTE, &PositionCtrl::OnClipboardText, this);
	sizer->Add(x_field, 2, wxEXPAND | wxLEFT | wxBOTTOM, 5);

	y_field = newd NumberCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(60, 20), wxTE_PROCESS_ENTER, min.y, max.y, pos.y, "Y");
	y_field->Bind(wxEVT_TEXT_PASTE, &PositionCtrl::OnClipboardText, this);
	sizer->Add(y_field, 2, wxEXPAND | wxLEFT | wxBOTTOM, 5);

	z_field = newd NumberCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(35, 20), wxTE_PROCESS_ENTER, min.z, max.z, pos.z, "Z");
	z_field->Bind(wxEVT_TEXT_PASTE, &PositionCtrl::OnClipboardText, this);
	sizer->Add(z_field, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

	SetSizerAndFit(sizer);
}

PositionCtrl::~PositionCtrl()
{
	// no-op
}

Position PositionCtrl::GetPosition() const
{
	Position pos;
	pos.x = x_field->GetIntValue();
	pos.y = y_field->GetIntValue();
	pos.z = z_field->GetIntValue();
	return pos;
}

void PositionCtrl::SetPosition(Position pos)
{
	x_field->SetIntValue(pos.x);
	y_field->SetIntValue(pos.y);
	z_field->SetIntValue(pos.z);
}

bool PositionCtrl::Enable(bool enable)
{
	return (x_field->Enable(enable)
		&& y_field->Enable(enable)
		&& z_field->Enable(enable));
}

void PositionCtrl::OnClipboardText(wxClipboardTextEvent& evt)
{
	Position position;
	if(posFromClipboard(position.x, position.y, position.z)) {
		x_field->SetIntValue(position.x);
		y_field->SetIntValue(position.y);
		z_field->SetIntValue(position.z);
	} else {
		evt.Skip();
	}
}
