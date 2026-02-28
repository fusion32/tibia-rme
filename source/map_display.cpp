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
#include "action.h"
#include "items.h"

#include <time.h>
#include <wx/wfstream.h>

#include "editor.h"
#include "brush.h"
#include "map.h"
#include "tile.h"
#include "property_window.h"
#include "palette_window.h"
#include "map_display.h"
#include "map_drawer.h"
#include "map_window.h"
#include "application.h"
#include "browse_tile_window.h"
#include "settings.h"

#include "doodad_brush.h"
#include "house_exit_brush.h"
#include "house_brush.h"
#include "wall_brush.h"
#include "creature_brush.h"
#include "ground_brush.h"
#include "waypoint_brush.h"
#include "raw_brush.h"
#include "carpet_brush.h"
#include "table_brush.h"

BEGIN_EVENT_TABLE(MapCanvas, wxGLCanvas)
	EVT_KEY_DOWN(MapCanvas::OnKeyDown)
	EVT_KEY_UP(MapCanvas::OnKeyUp)

	// Mouse events
	EVT_MOTION(MapCanvas::OnMouseMove)
	EVT_LEFT_UP(MapCanvas::OnMouseLeftRelease)
	EVT_LEFT_DOWN(MapCanvas::OnMouseLeftClick)
	EVT_LEFT_DCLICK(MapCanvas::OnMouseLeftDoubleClick)
	EVT_MIDDLE_DOWN(MapCanvas::OnMouseCenterClick)
	EVT_MIDDLE_UP(MapCanvas::OnMouseCenterRelease)
	EVT_RIGHT_DOWN(MapCanvas::OnMouseRightClick)
	EVT_RIGHT_UP(MapCanvas::OnMouseRightRelease)
	EVT_MOUSEWHEEL(MapCanvas::OnWheel)
	EVT_ENTER_WINDOW(MapCanvas::OnGainMouse)
	EVT_LEAVE_WINDOW(MapCanvas::OnLoseMouse)

	//Drawing events
	EVT_PAINT(MapCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(MapCanvas::OnEraseBackground)
	EVT_TIMER(MAP_ANIMATION_TIMER, MapCanvas::OnAnimationTimer)

	// Menu events
	EVT_MENU(MAP_POPUP_MENU_CUT, MapCanvas::OnCut)
	EVT_MENU(MAP_POPUP_MENU_COPY, MapCanvas::OnCopy)
	EVT_MENU(MAP_POPUP_MENU_COPY_POSITION, MapCanvas::OnCopyPosition)
	EVT_MENU(MAP_POPUP_MENU_PASTE, MapCanvas::OnPaste)
	EVT_MENU(MAP_POPUP_MENU_DELETE, MapCanvas::OnDelete)
	//----
	EVT_MENU(MAP_POPUP_MENU_COPY_TYPE_ID, MapCanvas::OnCopyTypeId)
	EVT_MENU(MAP_POPUP_MENU_COPY_NAME, MapCanvas::OnCopyName)
	// ----
	EVT_MENU(MAP_POPUP_MENU_ROTATE, MapCanvas::OnRotateItem)
	EVT_MENU(MAP_POPUP_MENU_GOTO, MapCanvas::OnGotoDestination)
	EVT_MENU(MAP_POPUP_MENU_COPY_DESTINATION, MapCanvas::OnCopyDestination)
	EVT_MENU(MAP_POPUP_MENU_SWITCH_DOOR, MapCanvas::OnSwitchDoor)
	// ----
	EVT_MENU(MAP_POPUP_MENU_SELECT_RAW_BRUSH, MapCanvas::OnSelectRAWBrush)
	EVT_MENU(MAP_POPUP_MENU_SELECT_GROUND_BRUSH, MapCanvas::OnSelectGroundBrush)
	EVT_MENU(MAP_POPUP_MENU_SELECT_DOODAD_BRUSH, MapCanvas::OnSelectDoodadBrush)
	EVT_MENU(MAP_POPUP_MENU_SELECT_DOOR_BRUSH, MapCanvas::OnSelectDoorBrush)
	EVT_MENU(MAP_POPUP_MENU_SELECT_WALL_BRUSH, MapCanvas::OnSelectWallBrush)
	EVT_MENU(MAP_POPUP_MENU_SELECT_CARPET_BRUSH, MapCanvas::OnSelectCarpetBrush)
	EVT_MENU(MAP_POPUP_MENU_SELECT_TABLE_BRUSH, MapCanvas::OnSelectTableBrush)
	EVT_MENU(MAP_POPUP_MENU_SELECT_CREATURE_BRUSH, MapCanvas::OnSelectCreatureBrush)
	EVT_MENU(MAP_POPUP_MENU_SELECT_HOUSE_BRUSH, MapCanvas::OnSelectHouseBrush)
	// ----
	EVT_MENU(MAP_POPUP_MENU_PROPERTIES, MapCanvas::OnProperties)
	// ----
	EVT_MENU(MAP_POPUP_MENU_BROWSE_TILE, MapCanvas::OnBrowseTile)
END_EVENT_TABLE()

bool MapCanvas::processed[] = {0};

MapCanvas::MapCanvas(MapWindow* parent) :
	wxGLCanvas(parent, wxID_ANY, nullptr, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS),
	floor(rme::MapGroundLayer),
	zoom(1.0),
	cursor_x(-1),
	cursor_y(-1),
	dragging(false),
	boundbox_selection(false),
	screendragging(false),
	drawing(false),
	dragging_draw(false),

	screenshot_buffer(nullptr),

	drag_start_x(-1),
	drag_start_y(-1),
	drag_start_z(-1),

	last_cursor_map_x(-1),
	last_cursor_map_y(-1),
	last_cursor_map_z(-1),

	last_click_map_x(-1),
	last_click_map_y(-1),
	last_click_map_z(-1),
	last_click_abs_x(-1),
	last_click_abs_y(-1),
	last_click_x(-1),
	last_click_y(-1),

	last_mmb_click_x(-1),
	last_mmb_click_y(-1)
{
	popup_menu = newd MapPopupMenu();
	animation_timer = newd wxTimer(this, MAP_ANIMATION_TIMER);
	drawer = new MapDrawer(this);
	keyCode = WXK_NONE;
}

MapCanvas::~MapCanvas()
{
	delete popup_menu;
	delete animation_timer;
	delete drawer;
	free(screenshot_buffer);
}

void MapCanvas::Refresh(bool eraseBackground /*= false*/, const wxRect *rect /*= NULL*/)
{
	if(refresh_watch.Time() > g_settings.getInteger(Config::HARD_REFRESH_RATE)) {
		refresh_watch.Start();
		wxGLCanvas::Update();
	}

	wxGLCanvas::Refresh(eraseBackground, rect);
}

void MapCanvas::SetZoom(double value)
{
	if(value < 0.125)
		value = 0.125;

	if(value > 25.00)
		value = 25.0;

	if(zoom != value) {
		int center_x, center_y;
		GetScreenCenter(&center_x, &center_y);

		zoom = value;
		GetMapWindow()->SetScreenCenterPosition(Position(center_x, center_y, floor));

		UpdatePositionStatus();
		UpdateZoomStatus();
		Refresh();
	}
}

void MapCanvas::GetViewBox(int* view_scroll_x, int* view_scroll_y, int* screensize_x, int* screensize_y) const
{
	MapWindow* window = GetMapWindow();
	window->GetViewSize(screensize_x, screensize_y);
	window->GetViewStart(view_scroll_x, view_scroll_y);
}

void MapCanvas::OnPaint(wxPaintEvent& event)
{
	SetCurrent(g_editor.GetGLContext(this));

	if(g_editor.IsRenderingEnabled()) {
		// NOTE(fusion): Make sure the map window is always adjusted with the
		// lastest size of the map. It only has side effects if the map size
		// has actually changed so it should be OK to be called multiple times.
		GetMapWindow()->FitToMap();

		DrawingOptions &options = drawer->getOptions();
		if(screenshot_buffer) {
			options.SetIngame();
		} else {
			options.transparent_floors = g_settings.getBoolean(Config::TRANSPARENT_FLOORS);
			options.transparent_items = g_settings.getBoolean(Config::TRANSPARENT_ITEMS);
			options.show_ingame_box = g_settings.getBoolean(Config::SHOW_INGAME_BOX);
			options.show_lights = g_settings.getBoolean(Config::SHOW_LIGHTS);
			options.grid_size = g_settings.getBoolean(Config::SHOW_GRID)
					? g_settings.getInteger(Config::GRID_SIZE) : 0;
			options.ingame = !g_settings.getBoolean(Config::SHOW_EXTRA);
			options.show_all_floors = g_settings.getBoolean(Config::SHOW_ALL_FLOORS);
			options.show_creatures = g_settings.getBoolean(Config::SHOW_CREATURES);
			options.show_houses = g_settings.getBoolean(Config::SHOW_HOUSES);
			options.show_shade = g_settings.getBoolean(Config::SHOW_SHADE);
			options.show_special_tiles = g_settings.getBoolean(Config::SHOW_SPECIAL_TILES);
			options.show_items = g_settings.getBoolean(Config::SHOW_ITEMS);
			options.highlight_items = g_settings.getBoolean(Config::HIGHLIGHT_ITEMS);
			options.show_blocking = g_settings.getBoolean(Config::SHOW_BLOCKING);
			options.show_tooltips = g_settings.getBoolean(Config::SHOW_TOOLTIPS);
			options.show_as_minimap = g_settings.getBoolean(Config::SHOW_AS_MINIMAP);
			options.show_only_colors = g_settings.getBoolean(Config::SHOW_ONLY_TILEFLAGS);
			options.show_only_modified = g_settings.getBoolean(Config::SHOW_ONLY_MODIFIED_TILES);
			options.show_preview = g_settings.getBoolean(Config::SHOW_PREVIEW);
			options.show_hooks = g_settings.getBoolean(Config::SHOW_WALL_HOOKS);
			options.show_pickupables = g_settings.getBoolean(Config::SHOW_PICKUPABLES);
			options.show_moveables = g_settings.getBoolean(Config::SHOW_MOVEABLES);
			options.hide_items_when_zoomed = g_settings.getBoolean(Config::HIDE_ITEMS_WHEN_ZOOMED);
		}

		options.dragging = boundbox_selection;

		if(!options.show_preview && drawer->GetPositionIndicatorTime() == 0){
			animation_timer->Stop();
		}else if(!animation_timer->IsRunning()){
			animation_timer->Start(100, wxTIMER_CONTINUOUS);
		}

		drawer->SetupVars();
		drawer->SetupGL();
		drawer->Draw();

		if(screenshot_buffer)
			drawer->TakeScreenshot(screenshot_buffer);

		drawer->Release();
	}

	// Clean unused textures
	g_editor.gfx.garbageCollection();

	// Swap buffer
	SwapBuffers();
}

void MapCanvas::OnAnimationTimer(wxTimerEvent &event)
{
	if(GetZoom() <= 2.0)
		Refresh();
}

void MapCanvas::ShowPositionIndicator(const Position& position)
{
	if(drawer) {
		drawer->ShowPositionIndicator(position);
		if(!animation_timer->IsRunning()) {
			Update();
		}
	}
}

void MapCanvas::TakeScreenshot(wxFileName path, wxString format)
{
	int screensize_x, screensize_y;
	GetViewBox(&view_scroll_x, &view_scroll_y, &screensize_x, &screensize_y);

	delete[] screenshot_buffer;
	screenshot_buffer = newd uint8_t[3 * screensize_x * screensize_y];

	// Draw the window
	Refresh();
	wxGLCanvas::Update(); // Forces immediate redraws the window.

	// screenshot_buffer should now contain the screenbuffer
	if(screenshot_buffer == nullptr) {
		g_editor.PopupDialog("Capture failed", "Image capture failed. Old Video Driver?", wxOK);
	} else {
		// We got the shit
		int screensize_x, screensize_y;
		GetMapWindow()->GetViewSize(&screensize_x, &screensize_y);
		wxImage screenshot(screensize_x, screensize_y, screenshot_buffer);

		time_t t = time(nullptr);
		struct tm* current_time = localtime(&t);
		ASSERT(current_time);

		wxString date;
		date << "screenshot_" << (1900 + current_time->tm_year);
		if(current_time->tm_mon < 9)
			date << "-" << "0" << current_time->tm_mon+1;
		else
			date << "-" << current_time->tm_mon+1;
		date << "-" << current_time->tm_mday;
		date << "-" << current_time->tm_hour;
		date << "-" << current_time->tm_min;
		date << "-" << current_time->tm_sec;

		int type = 0;
		path.SetName(date);
		if(format == "bmp") {
			path.SetExt(format);
			type = wxBITMAP_TYPE_BMP;
		} else if(format == "png") {
			path.SetExt(format);
			type = wxBITMAP_TYPE_PNG;
		} else if(format == "jpg" || format == "jpeg") {
			path.SetExt(format);
			type = wxBITMAP_TYPE_JPEG;
		} else if(format == "tga") {
			path.SetExt(format);
			type = wxBITMAP_TYPE_TGA;
		} else {
			g_editor.SetStatusText("Unknown screenshot format \'" + format + "\", switching to default (png)");
			path.SetExt("png");;
			type = wxBITMAP_TYPE_PNG;
		}

		path.Mkdir(0755, wxPATH_MKDIR_FULL);
		wxFileOutputStream of(path.GetFullPath());
		if(of.IsOk()) {
			if(screenshot.SaveFile(of, static_cast<wxBitmapType>(type)))
				g_editor.SetStatusText("Took screenshot and saved as " + path.GetFullName());
			else
				g_editor.PopupDialog("File error", "Couldn't save image file correctly.", wxOK);
		} else {
			g_editor.PopupDialog("File error", "Couldn't open file " + path.GetFullPath() + " for writing.", wxOK);
		}

	}

	Refresh();

	screenshot_buffer = nullptr;
}

void MapCanvas::ScreenToMap(int screen_x, int screen_y, int* map_x, int* map_y)
{
	int start_x, start_y;
	GetMapWindow()->GetViewStart(&start_x, &start_y);

	double m = zoom * GetContentScaleFactor();
	*map_x = (int)(start_x + screen_x * m) / rme::TileSize;
	*map_y = (int)(start_y + screen_y * m) / rme::TileSize;
}

MapWindow* MapCanvas::GetMapWindow() const
{
	wxWindow* window = GetParent();
	if(window)
		return static_cast<MapWindow*>(window);
	return nullptr;
}

void MapCanvas::GetScreenCenter(int* map_x, int* map_y)
{
	int width, height;
	GetMapWindow()->GetViewSize(&width, &height);
	return ScreenToMap(width/2, height/2, map_x, map_y);
}

Position MapCanvas::GetCursorPosition() const
{
	return Position(last_cursor_map_x, last_cursor_map_y, floor);
}

void MapCanvas::UpdatePositionStatus(int x, int y)
{
	if(x == -1) x = cursor_x;
	if(y == -1) y = cursor_y;

	int map_x, map_y;
	ScreenToMap(x, y, &map_x, &map_y);

	wxString status;

	{ // top object
		status.Clear();
		if(Tile *tile = g_editor.map.getTile(map_x, map_y, floor)){
			if(tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)){
				status << "Spawn of " << tile->creature->getName() << " (amount: " << tile->creature->spawnAmount << ")";
			}else if(Item* item = tile->getTopItem()){
				status << item->getName() << " (typeId: " << item->getID();

				if(item->getID() != item->getLookID()){
					status << ", lookId: " << item->getLookID();
				}

				if(item->getFlag(TAKE)){
					int weight = item->getAttribute(WEIGHT);
					if(item->getFlag(CUMULATIVE) && item->getAttribute(AMOUNT) > 0){
						weight *= item->getAttribute(AMOUNT);
					}
					status << ", weight: " << (weight / 100) << "." << (weight % 100) << "oz";
				}

				status << ")";
			}
		}

		if(status.IsEmpty()){
			status << "Nothing";
		}

		g_editor.root->SetStatusText(status, 1);
	}

	{ // position
		status.Clear();
		status << "[" << map_x << "," << map_y << "," << floor << "]";
		g_editor.root->SetStatusText(status, 2);
	}

	{ // sector
		status.Clear();
		int sectorX = map_x / MAP_SECTOR_SIZE;
		int sectorY = map_y / MAP_SECTOR_SIZE;
		int offsetX = map_x % MAP_SECTOR_SIZE;
		int offsetY = map_y % MAP_SECTOR_SIZE;
		status << sectorX << "-" << sectorY << "-" << floor
				<< ": " << offsetX << "-" << offsetY;
		g_editor.root->SetStatusText(status, 3);
	}
}

void MapCanvas::UpdateZoomStatus()
{
	int percentage = (int)((1.0 / zoom) * 100);
	wxString status;
	status << "zoom: " << percentage << "%";
	g_editor.root->SetStatusText(status, 4);
}

void MapCanvas::OnMouseMove(wxMouseEvent& event)
{
	if(screendragging) {
		GetMapWindow()->ScrollRelative(
				(int)(zoom * (cursor_x - event.GetX())),
				(int)(zoom * (cursor_y - event.GetY())));
		Refresh();
	}

	cursor_x = event.GetX();
	cursor_y = event.GetY();

	int mouse_map_x, mouse_map_y;
	MouseToMap(&mouse_map_x,&mouse_map_y);

	bool map_update = false;
	if(last_cursor_map_x != mouse_map_x || last_cursor_map_y != mouse_map_y || last_cursor_map_z != floor) {
		map_update = true;
	}
	last_cursor_map_x = mouse_map_x;
	last_cursor_map_y = mouse_map_y;
	last_cursor_map_z = floor;

	if(map_update) {
		UpdatePositionStatus(cursor_x, cursor_y);
		UpdateZoomStatus();
	}

	if(g_editor.IsSelectionMode()) {
		if(map_update && isPasting()) {
			Refresh();
		} else if(map_update && dragging) {
			wxString ss;

			int move_x = drag_start_x - mouse_map_x;
			int move_y = drag_start_y - mouse_map_y;
			int move_z = drag_start_z - floor;
			ss << "Dragging " << -move_x << "," << -move_y << "," << -move_z;
			g_editor.SetStatusText(ss);

			Refresh();
		} else if(boundbox_selection) {
			if(map_update) {
				wxString ss;

				int move_x = std::abs(last_click_map_x - mouse_map_x);
				int move_y = std::abs(last_click_map_y - mouse_map_y);
				ss << "Selection " << move_x+1 << ":" << move_y+1;
				g_editor.SetStatusText(ss);
			}

			Refresh();
		}
	} else { // Drawing mode
		Brush* brush = g_editor.GetCurrentBrush();
		if(map_update && drawing && brush) {
			if(brush->isDoodad()) {
				if(event.ControlDown()) {
					std::vector<Position> tilesToDraw;
					getTilesToDraw(mouse_map_x, mouse_map_y, floor, &tilesToDraw, nullptr);
					g_editor.undraw(tilesToDraw, event.ShiftDown() || event.AltDown());
				} else {
					g_editor.draw(Position(mouse_map_x, mouse_map_y, floor), event.ShiftDown() || event.AltDown());
				}
			} else if(brush->isDoor()) {
				if(!brush->canDraw(&g_editor.map, Position(mouse_map_x, mouse_map_y, floor))) {
					// We don't have to waste an action in this case...
				} else {
					std::vector<Position> tilesToDraw;
					std::vector<Position> tilesToBorderize;

					tilesToDraw.push_back(Position(mouse_map_x, mouse_map_y, floor));

					tilesToBorderize.push_back(Position(mouse_map_x    , mouse_map_y - 1, floor));
					tilesToBorderize.push_back(Position(mouse_map_x - 1, mouse_map_y    , floor));
					tilesToBorderize.push_back(Position(mouse_map_x    , mouse_map_y + 1, floor));
					tilesToBorderize.push_back(Position(mouse_map_x + 1, mouse_map_y    , floor));

					if(event.ControlDown()) {
						g_editor.undraw(tilesToDraw, tilesToBorderize, event.AltDown());
					} else {
						g_editor.draw(tilesToDraw, tilesToBorderize, event.AltDown());
					}
				}
			} else if(brush->needBorders()) {
				std::vector<Position> tilesToDraw, tilesToBorderize;

				getTilesToDraw(mouse_map_x, mouse_map_y, floor, &tilesToDraw, &tilesToBorderize);

				if(event.ControlDown()) {
					g_editor.undraw(tilesToDraw, tilesToBorderize, event.AltDown());
				} else {
					g_editor.draw(tilesToDraw, tilesToBorderize, event.AltDown());
				}
			} else if(brush->oneSizeFitsAll()) {
				drawing = true;
				std::vector<Position> tilesToDraw;
				tilesToDraw.push_back(Position(mouse_map_x,mouse_map_y, floor));
				if(event.ControlDown()) {
					g_editor.undraw(tilesToDraw, event.AltDown());
				} else {
					g_editor.draw(tilesToDraw, event.AltDown());
				}
			} else { // No borders
				std::vector<Position> tilesToDraw;
				for(int y = -g_editor.GetBrushSize(); y <= g_editor.GetBrushSize(); y++) {
					for(int x = -g_editor.GetBrushSize(); x <= g_editor.GetBrushSize(); x++) {
						if(g_editor.GetBrushShape() == BRUSHSHAPE_SQUARE) {
							tilesToDraw.push_back(Position(mouse_map_x+x,mouse_map_y+y, floor));
						} else if(g_editor.GetBrushShape() == BRUSHSHAPE_CIRCLE) {
							double distance = sqrt(double(x*x) + double(y*y));
							if(distance < g_editor.GetBrushSize()+0.005) {
								tilesToDraw.push_back(Position(mouse_map_x+x,mouse_map_y+y, floor));
							}
						}
					}
				}
				if(event.ControlDown()) {
					g_editor.undraw(tilesToDraw, event.AltDown());
				} else {
					g_editor.draw(tilesToDraw, event.AltDown());
				}
			}

			// Create newd doodad layout (does nothing if a non-doodad brush is selected)
			g_editor.FillDoodadPreviewBuffer();

			g_editor.RefreshView();
		} else if(dragging_draw) {
			g_editor.RefreshView();
		} else if(map_update && brush) {
			Refresh();
		}
	}
}

void MapCanvas::OnMouseLeftRelease(wxMouseEvent& event)
{
	OnMouseActionRelease(event);
}

void MapCanvas::OnMouseLeftClick(wxMouseEvent& event)
{
	OnMouseActionClick(event);
}

void MapCanvas::OnMouseLeftDoubleClick(wxMouseEvent& event)
{
	if(!g_settings.getInteger(Config::DOUBLECLICK_PROPERTIES)) {
		return;
	}

	Map &map = g_editor.map;
	int mouse_map_x, mouse_map_y;
	ScreenToMap(event.GetX(), event.GetY(), &mouse_map_x, &mouse_map_y);
	const Tile* tile = map.getTile(mouse_map_x, mouse_map_y, floor);
	if(tile && !tile->empty()) {
		wxDialog* dialog = nullptr;
		Tile newTile; newTile.deepCopy(*tile);
		if(newTile.creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
			dialog = newd CreaturePropertyWindow(g_editor.root, newTile.creature);
		} else if(Item *topItem = newTile.getTopItem()) {
			dialog = newd ItemPropertyWindow(g_editor.root, topItem);
		} else {
			return;
		}

		if(dialog->ShowModal() != 0){
			Action *action = g_editor.actionQueue.createAction(ACTION_CHANGE_PROPERTIES);
			action->changeTile(std::move(newTile));
			action->commit();
		}

		dialog->Destroy();
	}
}

void MapCanvas::OnMouseCenterClick(wxMouseEvent& event)
{
	if(g_settings.getInteger(Config::SWITCH_MOUSEBUTTONS)) {
		OnMousePropertiesClick(event);
	} else {
		OnMouseCameraClick(event);
	}
}

void MapCanvas::OnMouseCenterRelease(wxMouseEvent& event)
{
	if(g_settings.getInteger(Config::SWITCH_MOUSEBUTTONS)) {
		OnMousePropertiesRelease(event);
	} else {
		OnMouseCameraRelease(event);
	}
}

void MapCanvas::OnMouseRightClick(wxMouseEvent& event)
{
	if(g_settings.getInteger(Config::SWITCH_MOUSEBUTTONS)) {
		OnMouseCameraClick(event);
	} else {
		OnMousePropertiesClick(event);
	}
}

void MapCanvas::OnMouseRightRelease(wxMouseEvent& event)
{
	if(g_settings.getInteger(Config::SWITCH_MOUSEBUTTONS)) {
		OnMouseCameraRelease(event);
	} else {
		OnMousePropertiesRelease(event);
	}
}

void MapCanvas::OnMouseActionClick(wxMouseEvent& event)
{
	SetFocus();

	int mouse_map_x, mouse_map_y;
	ScreenToMap(event.GetX(), event.GetY(), &mouse_map_x, &mouse_map_y);

	if(event.ControlDown() && event.AltDown()) {
		Tile* tile = g_editor.map.getTile(mouse_map_x, mouse_map_y, floor);
		if(tile && !tile->empty()) {
			Item* item = tile->getTopItem();
			if(item && item->getRAWBrush())
				g_editor.SelectBrush(item->getRAWBrush(), TILESET_RAW);
		}
	} else if(g_editor.IsSelectionMode()) {
		if(isPasting()) {
			// Set paste to false (no rendering etc.)
			EndPasting();

			// Paste to the map
			g_editor.copybuffer.paste(Position(mouse_map_x, mouse_map_y, floor));

			// Start dragging
			dragging = true;
			drag_start_x = mouse_map_x;
			drag_start_y = mouse_map_y;
			drag_start_z = floor;
		} else do {
			Selection &selection = g_editor.selection;
			boundbox_selection = false;
			if(event.ShiftDown()) {
				boundbox_selection = true;

				if(!event.ControlDown()) {
					selection.clear(NULL);
					selection.updateSelectionCount();
				}
			} else if(event.ControlDown()) {
				if(Tile* tile = g_editor.map.getTile(mouse_map_x, mouse_map_y, floor)){
					if(tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
						if(tile->creature->isSelected()){
							selection.remove(NULL, tile, tile->creature);
						}else{
							selection.add(NULL, tile, tile->creature);
						}
						selection.updateSelectionCount();
					} else if(Item* item = tile->getTopItem()){
						if(item->isSelected()) {
							selection.remove(NULL, tile, item);
						} else {
							selection.add(NULL, tile, item);
						}
						selection.updateSelectionCount();
					}
				}
			} else {
				Tile* tile = g_editor.map.getTile(mouse_map_x, mouse_map_y, floor);
				if(!tile) {
					selection.clear(NULL);
					selection.updateSelectionCount();
				} else if(tile->isSelected()) {
					dragging = true;
					drag_start_x = mouse_map_x;
					drag_start_y = mouse_map_y;
					drag_start_z = floor;
				} else {
					Action *action = g_editor.actionQueue.createAction(ACTION_SELECT);
					selection.clear(action);
					if(tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)){
						selection.add(action, tile, tile->creature);
						dragging = true;
						drag_start_x = mouse_map_x;
						drag_start_y = mouse_map_y;
						drag_start_z = floor;
					}else if(Item *item = tile->getTopItem()){
						selection.add(action, tile, item);
						dragging = true;
						drag_start_x = mouse_map_x;
						drag_start_y = mouse_map_y;
						drag_start_z = floor;
					}
					action->commit();
					selection.updateSelectionCount();
				}
			}
		} while(false);
	} else if(g_editor.GetCurrentBrush()) { // Drawing mode
		Brush* brush = g_editor.GetCurrentBrush();
		if(event.ShiftDown() && brush->canDrag()) {
			dragging_draw = true;
		} else {
			if(g_editor.GetBrushSize() == 0 && !brush->oneSizeFitsAll()) {
				drawing = true;
			} else {
				drawing = g_editor.GetCurrentBrush()->canSmear();
			}
			if(brush->isWall()) {
				if(event.AltDown() && g_editor.GetBrushSize() == 0) {
					// z0mg, just clicked a tile, shift variaton.
					if(event.ControlDown()) {
						g_editor.undraw(Position(mouse_map_x, mouse_map_y, floor), event.AltDown());
					} else {
						g_editor.draw(Position(mouse_map_x, mouse_map_y, floor), event.AltDown());
					}
				} else {
					std::vector<Position> tilesToDraw;
					std::vector<Position> tilesToBorderize;

					int start_map_x = mouse_map_x - g_editor.GetBrushSize();
					int start_map_y = mouse_map_y - g_editor.GetBrushSize();
					int end_map_x   = mouse_map_x + g_editor.GetBrushSize();
					int end_map_y   = mouse_map_y + g_editor.GetBrushSize();

					for(int y = start_map_y -1; y <= end_map_y + 1; ++y) {
						for(int x = start_map_x - 1; x <= end_map_x + 1; ++x) {
							if((x <= start_map_x+1 || x >= end_map_x-1) || (y <= start_map_y+1 || y >= end_map_y-1)) {
								tilesToBorderize.push_back(Position(x,y,floor));
							}
							if(((x == start_map_x || x == end_map_x) || (y == start_map_y || y == end_map_y)) &&
								((x >= start_map_x && x <= end_map_x) && (y >= start_map_y && y <= end_map_y))) {
								tilesToDraw.push_back(Position(x,y,floor));
							}
						}
					}
					if(event.ControlDown()) {
						g_editor.undraw(tilesToDraw, tilesToBorderize, event.AltDown());
					} else {
						g_editor.draw(tilesToDraw, tilesToBorderize, event.AltDown());
					}
				}
			} else if(brush->isDoor()) {
				std::vector<Position> tilesToDraw;
				std::vector<Position> tilesToBorderize;

				tilesToDraw.push_back(Position(mouse_map_x, mouse_map_y, floor));

				tilesToBorderize.push_back(Position(mouse_map_x    , mouse_map_y - 1, floor));
				tilesToBorderize.push_back(Position(mouse_map_x - 1, mouse_map_y    , floor));
				tilesToBorderize.push_back(Position(mouse_map_x    , mouse_map_y + 1, floor));
				tilesToBorderize.push_back(Position(mouse_map_x + 1, mouse_map_y    , floor));

				if(event.ControlDown()) {
					g_editor.undraw(tilesToDraw, tilesToBorderize, event.AltDown());
				} else {
					g_editor.draw(tilesToDraw, tilesToBorderize, event.AltDown());
				}
			} else if(brush->isDoodad() || brush->isCreature()) {
				if(event.ControlDown()) {
					if(brush->isDoodad()) {
						std::vector<Position> tilesToDraw;
						getTilesToDraw(mouse_map_x, mouse_map_y, floor, &tilesToDraw, nullptr);
						g_editor.undraw(tilesToDraw, event.AltDown());
					} else {
						g_editor.undraw(Position(mouse_map_x, mouse_map_y, floor), event.ShiftDown() || event.AltDown());
					}
				} else {
					g_editor.draw(Position(mouse_map_x, mouse_map_y, floor), event.ShiftDown() || event.AltDown());
				}
			} else {
				if(brush->needBorders()) {
					std::vector<Position> tilesToDraw;
					std::vector<Position> tilesToBorderize;

					bool fill = keyCode == WXK_CONTROL_D && event.ControlDown() && brush->isGround();
					getTilesToDraw(mouse_map_x, mouse_map_y, floor, &tilesToDraw, &tilesToBorderize, fill);

					if(!fill && event.ControlDown()) {
						g_editor.undraw(tilesToDraw, tilesToBorderize, event.AltDown());
					} else {
						g_editor.draw(tilesToDraw, tilesToBorderize, event.AltDown());
					}
				} else if(brush->oneSizeFitsAll()) {
					if(brush->isHouseExit() || brush->isWaypoint()) {
						g_editor.draw(Position(mouse_map_x, mouse_map_y, floor), event.AltDown());
					} else {
						std::vector<Position> tilesToDraw;
						tilesToDraw.push_back(Position(mouse_map_x,mouse_map_y, floor));
						if(event.ControlDown()) {
							g_editor.undraw(tilesToDraw, event.AltDown());
						} else {
							g_editor.draw(tilesToDraw, event.AltDown());
						}
					}
				} else {
					std::vector<Position> tilesToDraw;

					getTilesToDraw(mouse_map_x, mouse_map_y, floor, &tilesToDraw, nullptr);

					if(event.ControlDown()) {
						g_editor.undraw(tilesToDraw, event.AltDown());
					} else {
						g_editor.draw(tilesToDraw, event.AltDown());
					}
				}
			}
			// Change the doodad layout brush
			g_editor.FillDoodadPreviewBuffer();
		}
	}
	last_click_x = int(event.GetX()*zoom);
	last_click_y = int(event.GetY()*zoom);

	int start_x, start_y;
	GetMapWindow()->GetViewStart(&start_x, &start_y);
	last_click_abs_x = last_click_x + start_x;
	last_click_abs_y = last_click_y + start_y;

	last_click_map_x = mouse_map_x;
	last_click_map_y = mouse_map_y;
	last_click_map_z = floor;
	g_editor.RefreshView();
	g_editor.UpdateMinimap();
}

void MapCanvas::OnMouseActionRelease(wxMouseEvent& event)
{
	int mouse_map_x, mouse_map_y;
	ScreenToMap(event.GetX(), event.GetY(), &mouse_map_x, &mouse_map_y);

	int move_x = last_click_map_x - mouse_map_x;
	int move_y = last_click_map_y - mouse_map_y;
	int move_z = last_click_map_z - floor;

	if(g_editor.IsSelectionMode()) {
		if(dragging && (move_x != 0 || move_y != 0 || move_z != 0)) {
			g_editor.moveSelection(Position(move_x, move_y, move_z));
		} else {
			Selection &selection = g_editor.selection;
			if(boundbox_selection) {
				if(mouse_map_x == last_click_map_x && mouse_map_y == last_click_map_y && event.ControlDown()) {
					// Mouse hasn't moved, do control+shift thingy!
					Tile* tile = g_editor.map.getTile(mouse_map_x, mouse_map_y, floor);
					if(tile) {
						if(tile->isSelected()) {
							selection.remove(NULL, tile);
						} else {
							selection.add(NULL, tile);
						}
						selection.updateSelectionCount();
					}
				} else {
					// The cursor has moved, do some boundboxing!
					if(last_click_map_x > mouse_map_x) {
						int tmp = mouse_map_x;
						mouse_map_x = last_click_map_x;
						last_click_map_x = tmp;
					}
					if(last_click_map_y > mouse_map_y) {
						int tmp = mouse_map_y;
						mouse_map_y = last_click_map_y;
						last_click_map_y = tmp;
					}

					int start_x = 0, start_y = 0, start_z = 0;
					int end_x = 0, end_y = 0, end_z = 0;
					bool compensated = g_settings.getInteger(Config::COMPENSATED_SELECT);
					switch(g_settings.getInteger(Config::SELECTION_TYPE)) {
						case SELECT_CURRENT_FLOOR: {
							start_z = end_z = floor;
							start_x = last_click_map_x;
							start_y = last_click_map_y;
							end_x = mouse_map_x;
							end_y = mouse_map_y;
							break;
						}
						case SELECT_ALL_FLOORS: {
							start_x = last_click_map_x;
							start_y = last_click_map_y;
							start_z = rme::MapMaxLayer;
							end_x = mouse_map_x;
							end_y = mouse_map_y;
							end_z = floor;

							if(compensated){
								start_x -= (floor < rme::MapGroundLayer ? rme::MapGroundLayer - floor : 0);
								start_y -= (floor < rme::MapGroundLayer ? rme::MapGroundLayer - floor : 0);
								end_x   -= (floor < rme::MapGroundLayer ? rme::MapGroundLayer - floor : 0);
								end_y   -= (floor < rme::MapGroundLayer ? rme::MapGroundLayer - floor : 0);
							}
							break;
						}
						case SELECT_VISIBLE_FLOORS: {
							start_x = last_click_map_x;
							start_y = last_click_map_y;
							if(floor < 8) {
								start_z = rme::MapGroundLayer;
							} else {
								start_z = std::min(rme::MapMaxLayer, floor + 2);
							}
							end_x = mouse_map_x;
							end_y = mouse_map_y;
							end_z = floor;

							if(compensated){
								start_x -= (floor < rme::MapGroundLayer ? rme::MapGroundLayer - floor : 0);
								start_y -= (floor < rme::MapGroundLayer ? rme::MapGroundLayer - floor : 0);
								end_x   -= (floor < rme::MapGroundLayer ? rme::MapGroundLayer - floor : 0);
								end_y   -= (floor < rme::MapGroundLayer ? rme::MapGroundLayer - floor : 0);
							}
							break;
						}
					}

					// TODO(fusion): We could restore the threaded selection here
					// if it turns out to be too slow. We could also iterate using
					// sectors which should be faster than accessing tiles one by one.
					Action *action = g_editor.actionQueue.createAction(ACTION_SELECT);
					for(int z = start_z; z <= end_z; z += 1){
						for(int y = start_y; y <= end_y; y += 1)
						for(int x = start_x; x <= end_x; x += 1){
							if(Tile *tile = g_editor.map.getTile(x, y, z)){
								selection.add(action, tile);
							}
						}

						if(compensated && z <= rme::MapGroundLayer){
							start_x += 1;
							start_y += 1;
							end_x   += 1;
							end_y   += 1;
						}
					}
					action->commit();
					selection.updateSelectionCount();
				}
			} else if(event.ControlDown()) {
				////
			} else {
				// User hasn't moved anything, meaning selection/deselection
				Tile* tile = g_editor.map.getTile(mouse_map_x, mouse_map_y, floor);
				if(tile) {
					if(tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
						if(!tile->creature->isSelected()) {
							selection.add(NULL, tile, tile->creature);
							selection.updateSelectionCount();
						}
					} else {
						Item* item = tile->getTopItem();
						if(item && !item->isSelected()) {
							selection.add(NULL, tile, item);
							selection.updateSelectionCount();
						}
					}
				}
			}
		}
		g_editor.resetActionsTimer();
		g_editor.updateActions();
		dragging = false;
		boundbox_selection = false;
	} else if(g_editor.GetCurrentBrush()){ // Drawing mode
		Brush* brush = g_editor.GetCurrentBrush();
		if(dragging_draw) {
			std::vector<Position> tilesToDraw;
			std::vector<Position> tilesToBorderize;
			if(brush->isWall()) {
				int start_map_x = std::min(last_click_map_x, mouse_map_x);
				int start_map_y = std::min(last_click_map_y, mouse_map_y);
				int end_map_x   = std::max(last_click_map_x, mouse_map_x);
				int end_map_y   = std::max(last_click_map_y, mouse_map_y);

				for(int y = start_map_y-1; y <= end_map_y+1; y ++) {
					for(int x = start_map_x-1; x <= end_map_x+1; x++) {
						if((x <= start_map_x+1 || x >= end_map_x-1) || (y <= start_map_y+1 || y >= end_map_y-1)) {
							tilesToBorderize.push_back(Position(x,y,floor));
						}
						if(((x == start_map_x || x == end_map_x) || (y == start_map_y || y == end_map_y)) &&
							((x >= start_map_x && x <= end_map_x) && (y >= start_map_y && y <= end_map_y))) {
							tilesToDraw.push_back(Position(x,y,floor));
						}
					}
				}
			} else {
				if(g_editor.GetBrushShape() == BRUSHSHAPE_SQUARE) {
					if(last_click_map_x > mouse_map_x) {
						int tmp = mouse_map_x; mouse_map_x = last_click_map_x; last_click_map_x = tmp;
					}
					if(last_click_map_y > mouse_map_y) {
						int tmp = mouse_map_y; mouse_map_y = last_click_map_y; last_click_map_y = tmp;
					}

					for(int x = last_click_map_x-1; x <= mouse_map_x+1; x++) {
						for(int y = last_click_map_y-1; y <= mouse_map_y+1; y ++) {
							if((x <= last_click_map_x || x >= mouse_map_x) || (y <= last_click_map_y || y >= mouse_map_y)) {
								tilesToBorderize.push_back(Position(x,y,floor));
							}
							if((x >= last_click_map_x && x <= mouse_map_x) && (y >= last_click_map_y && y <= mouse_map_y)) {
								tilesToDraw.push_back(Position(x,y,floor));
							}
						}
					}
				} else {
					int start_x, end_x;
					int start_y, end_y;
					int width = std::max(
						std::abs(
						std::max(mouse_map_y, last_click_map_y) -
						std::min(mouse_map_y, last_click_map_y)
						),
						std::abs(
						std::max(mouse_map_x, last_click_map_x) -
						std::min(mouse_map_x, last_click_map_x)
						)
						);
					if(mouse_map_x < last_click_map_x) {
						start_x = last_click_map_x - width;
						end_x = last_click_map_x;
					} else {
						start_x = last_click_map_x;
						end_x = last_click_map_x + width;
					}
					if(mouse_map_y < last_click_map_y) {
						start_y = last_click_map_y - width;
						end_y = last_click_map_y;
					} else {
						start_y = last_click_map_y;
						end_y = last_click_map_y + width;
					}

					int center_x = start_x + (end_x - start_x) / 2;
					int center_y = start_y + (end_y - start_y) / 2;
					float radii = width / 2.0f + 0.005f;

					for(int y = start_y-1; y <= end_y+1; y++) {
						float dy = center_y - y;
						for(int x = start_x-1; x <= end_x+1; x++) {
							float dx = center_x - x;
							//printf("%f;%f\n", dx, dy);
							float distance = sqrt(dx*dx + dy*dy);
							if(distance < radii) {
								tilesToDraw.push_back(Position(x,y,floor));
							}
							if(std::abs(distance - radii) < 1.5) {
								tilesToBorderize.push_back(Position(x,y,floor));
							}
						}
					}
				}
			}
			if(event.ControlDown()) {
				g_editor.undraw(tilesToDraw, tilesToBorderize, event.AltDown());
			} else {
				g_editor.draw(tilesToDraw, tilesToBorderize, event.AltDown());
			}
		}
		g_editor.resetActionsTimer();
		g_editor.updateActions();
		drawing = false;
		dragging_draw = false;
	}
	g_editor.RefreshView();
	g_editor.UpdateMinimap();
}

void MapCanvas::OnMouseCameraClick(wxMouseEvent& event)
{
	SetFocus();

	last_mmb_click_x = event.GetX();
	last_mmb_click_y = event.GetY();

	if(event.ControlDown()) {
		int screensize_x, screensize_y;
		MapWindow* window = GetMapWindow();
		window->GetViewSize(&screensize_x, &screensize_y);
		window->ScrollRelative(
			int(-screensize_x * (1.0 - zoom) * (std::max(cursor_x, 1) / double(screensize_x))),
			int(-screensize_y * (1.0 - zoom) * (std::max(cursor_y, 1) / double(screensize_y)))
		);
		zoom = 1.0;
		Refresh();
	} else {
		screendragging = true;
	}
}

void MapCanvas::OnMouseCameraRelease(wxMouseEvent& event)
{
	SetFocus();
	screendragging = false;
	if(event.ControlDown()) {
		// ...
		// Haven't moved much, it's a click!
	} else if(last_mmb_click_x > event.GetX() - 3 && last_mmb_click_x < event.GetX() + 3 &&
				last_mmb_click_y > event.GetY() - 3 && last_mmb_click_y < event.GetY() + 3) {
		int screensize_x, screensize_y;
		MapWindow* window = GetMapWindow();
		window->GetViewSize(&screensize_x, &screensize_y);
		window->ScrollRelative(
			int(zoom * (2*cursor_x - screensize_x)),
			int(zoom * (2*cursor_y - screensize_y))
		);
		Refresh();
	}
}

void MapCanvas::OnMousePropertiesClick(wxMouseEvent& event)
{
	SetFocus();

	int mouse_map_x, mouse_map_y;
	ScreenToMap(event.GetX(), event.GetY(), &mouse_map_x, &mouse_map_y);
	Tile* tile = g_editor.map.getTile(mouse_map_x, mouse_map_y, floor);

	if(g_editor.IsDrawingMode()) {
		g_editor.SetSelectionMode();
	}

	EndPasting();

	Selection& selection = g_editor.getSelection();

	boundbox_selection = false;
	if(event.ShiftDown()) {
		boundbox_selection = true;

		if(!event.ControlDown()) {
			selection.clear(NULL);
			selection.updateSelectionCount();
		}
	} else if(!tile) {
		selection.clear(NULL);
		selection.updateSelectionCount();
	} else if(tile->isSelected()) {
		// Do nothing!
	} else {
		Action *action = g_editor.actionQueue.createAction(ACTION_SELECT);
		selection.clear(action);
		if(tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
			selection.add(action, tile, tile->creature);
		} else if(Item* item = tile->getTopItem()) {
			selection.add(action, tile, item);
		}
		action->commit();
		selection.updateSelectionCount();
	}

	last_click_x = int(event.GetX()*zoom);
	last_click_y = int(event.GetY()*zoom);

	int start_x, start_y;
	GetMapWindow()->GetViewStart(&start_x, &start_y);
	last_click_abs_x = last_click_x + start_x;
	last_click_abs_y = last_click_y + start_y;

	last_click_map_x = mouse_map_x;
	last_click_map_y = mouse_map_y;
	g_editor.RefreshView();
}

void MapCanvas::OnMousePropertiesRelease(wxMouseEvent& event)
{
	int mouse_map_x, mouse_map_y;
	ScreenToMap(event.GetX(), event.GetY(), &mouse_map_x, &mouse_map_y);

	if(g_editor.IsDrawingMode()) {
		g_editor.SetSelectionMode();
	}

	if(boundbox_selection) {
		Selection& selection = g_editor.getSelection();
		if(mouse_map_x == last_click_map_x && mouse_map_y == last_click_map_y && event.ControlDown()) {
			// Mouse hasn't move, do control+shift thingy!
			Tile* tile = g_editor.map.getTile(mouse_map_x, mouse_map_y, floor);
			if(tile) {
				if(tile->isSelected()) {
					selection.remove(NULL, tile);
				} else {
					selection.add(NULL, tile);
				}
				selection.updateSelectionCount();
			}
		} else {
			// The cursor has moved, do some boundboxing!
			if(last_click_map_x > mouse_map_x) {
				int tmp = mouse_map_x;
				mouse_map_x = last_click_map_x;
				last_click_map_x = tmp;
			}
			if(last_click_map_y > mouse_map_y) {
				int tmp = mouse_map_y;
				mouse_map_y = last_click_map_y;
				last_click_map_y = tmp;
			}

			int start_x = 0, start_y = 0, start_z = 0;
			int end_x = 0, end_y = 0, end_z = 0;
			bool compensated = g_settings.getInteger(Config::COMPENSATED_SELECT);
			switch(g_settings.getInteger(Config::SELECTION_TYPE)) {
				case SELECT_CURRENT_FLOOR: {
					start_z = end_z = floor;
					start_x = last_click_map_x;
					start_y = last_click_map_y;
					end_x = mouse_map_x;
					end_y = mouse_map_y;
					break;
				}
				case SELECT_ALL_FLOORS: {
					start_x = last_click_map_x;
					start_y = last_click_map_y;
					start_z = rme::MapMaxLayer;
					end_x = mouse_map_x;
					end_y = mouse_map_y;
					end_z = floor;

					if(compensated){
						start_x -= (floor < rme::MapGroundLayer ? rme::MapGroundLayer - floor : 0);
						start_y -= (floor < rme::MapGroundLayer ? rme::MapGroundLayer - floor : 0);
						end_x   -= (floor < rme::MapGroundLayer ? rme::MapGroundLayer - floor : 0);
						end_y   -= (floor < rme::MapGroundLayer ? rme::MapGroundLayer - floor : 0);
					}
					break;
				}
				case SELECT_VISIBLE_FLOORS: {
					start_x = last_click_map_x;
					start_y = last_click_map_y;
					if(floor < 8) {
						start_z = rme::MapGroundLayer;
					} else {
						start_z = std::min(rme::MapMaxLayer, floor + 2);
					}
					end_x = mouse_map_x;
					end_y = mouse_map_y;
					end_z = floor;

					if(compensated){
						start_x -= (floor < rme::MapGroundLayer ? rme::MapGroundLayer - floor : 0);
						start_y -= (floor < rme::MapGroundLayer ? rme::MapGroundLayer - floor : 0);
						end_x   -= (floor < rme::MapGroundLayer ? rme::MapGroundLayer - floor : 0);
						end_y   -= (floor < rme::MapGroundLayer ? rme::MapGroundLayer - floor : 0);
					}
					break;
				}
			}

			// TODO(fusion): We could restore the threaded selection here
			// if it turns out to be too slow. We could also iterate using
			// sectors which should be faster than accessing tiles one by one.
			Action *action = g_editor.actionQueue.createAction(ACTION_SELECT);
			for(int z = start_z; z <= end_z; z += 1){
				for(int y = start_y; y <= end_y; y += 1)
				for(int x = start_x; x <= end_x; x += 1){
					if(Tile *tile = g_editor.map.getTile(x, y, z)){
						selection.add(action, tile);
					}
				}

				if(compensated && z <= rme::MapGroundLayer){
					start_x += 1;
					start_y += 1;
					end_x   += 1;
					end_y   += 1;
				}
			}
			action->commit();
			selection.updateSelectionCount();
		}
	} else if(event.ControlDown()) {
		// Nothing
	}

	popup_menu->Update();
	PopupMenu(popup_menu);

	g_editor.resetActionsTimer();
	dragging = false;
	boundbox_selection = false;

	last_cursor_map_x = mouse_map_x;
	last_cursor_map_y = mouse_map_y;
	last_cursor_map_z = floor;

	g_editor.RefreshView();
}

void MapCanvas::OnWheel(wxMouseEvent& event)
{
	// TODO(fusion): Is it even reasonable to use these static variables for
	// accumulating wheel rotation?
	if(event.ControlDown()) {
		static int accumulator = 0.0;
		accumulator += event.GetWheelRotation();
		if(accumulator <= event.GetWheelDelta() || accumulator >= event.GetWheelDelta()){
			if(accumulator < 0){
				g_editor.ChangeFloor(floor - 1);
			}else{
				g_editor.ChangeFloor(floor + 1);
			}
			accumulator = 0;
		}
		UpdatePositionStatus();
	} else if(event.AltDown()) {
		static int accumulator = 0.0;
		accumulator += event.GetWheelRotation();
		if(accumulator <= event.GetWheelDelta() || accumulator >= event.GetWheelDelta()){
			if(accumulator < 0){
				g_editor.IncreaseBrushSize();
			}else{
				g_editor.DecreaseBrushSize();
			}
			accumulator = 0;
		}
	} else {
		double diff = -((double)event.GetWheelRotation() / (double)event.GetWheelDelta())
				* g_settings.getFloat(Config::ZOOM_SPEED);
		double oldzoom = zoom;
		zoom += diff;

		if(zoom < 0.125) {
			diff = 0.125 - oldzoom;
			zoom = 0.125;
		}

		if(zoom > 25.00) {
			diff = 25.00 - oldzoom;
			zoom = 25.0;
		}

		UpdateZoomStatus();

		int screensize_x, screensize_y;
		MapWindow* window = GetMapWindow();
		window->GetViewSize(&screensize_x, &screensize_y);

		// This took a day to figure out!
		int scroll_x = int(screensize_x * diff * (std::max(cursor_x, 1) / double(screensize_x))) * GetContentScaleFactor();
		int scroll_y = int(screensize_y * diff * (std::max(cursor_y, 1) / double(screensize_y))) * GetContentScaleFactor();

		window->ScrollRelative(-scroll_x, -scroll_y);
	}

	Refresh();
}

void MapCanvas::OnLoseMouse(wxMouseEvent& event)
{
	Refresh();
}

void MapCanvas::OnGainMouse(wxMouseEvent& event)
{
	if(!event.LeftIsDown()) {
		dragging = false;
		boundbox_selection = false;
		drawing = false;
	}
	if(!event.MiddleIsDown()) {
		screendragging = false;
	}

	Refresh();
}

void MapCanvas::OnKeyDown(wxKeyEvent& event)
{
	MapWindow* window = GetMapWindow();

	//char keycode = event.GetKeyCode();
	// std::cout << "Keycode " << keycode << std::endl;
	switch(event.GetKeyCode()) {
		case WXK_NUMPAD_ADD:
		case WXK_PAGEUP: {
			g_editor.ChangeFloor(floor - 1);
			break;
		}
		case WXK_NUMPAD_SUBTRACT:
		case WXK_PAGEDOWN: {
			g_editor.ChangeFloor(floor + 1);
			break;
		}
		case WXK_NUMPAD_MULTIPLY: {
			double diff = -0.3;

			double oldzoom = zoom;
			zoom += diff;

			if(zoom < 0.125) {
				diff = 0.125 - oldzoom; zoom = 0.125;
			}

			int screensize_x, screensize_y;
			window->GetViewSize(&screensize_x, &screensize_y);

			// This took a day to figure out!
			int scroll_x = int(screensize_x * diff * (std::max(cursor_x, 1) / double(screensize_x)));
			int scroll_y = int(screensize_y * diff * (std::max(cursor_y, 1) / double(screensize_y)));

			window->ScrollRelative(-scroll_x, -scroll_y);

			UpdatePositionStatus();
			UpdateZoomStatus();
			Refresh();
			break;
		}
		case WXK_NUMPAD_DIVIDE: {
			double diff = 0.3;
			double oldzoom = zoom;
			zoom += diff;

			if(zoom > 25.00) {
				diff = 25.00 - oldzoom; zoom = 25.0;
			}

			int screensize_x, screensize_y;
			window->GetViewSize(&screensize_x, &screensize_y);

			// This took a day to figure out!
			int scroll_x = int(screensize_x * diff * (std::max(cursor_x, 1) / double(screensize_x)));
			int scroll_y = int(screensize_y * diff * (std::max(cursor_y, 1) / double(screensize_y)));
			window->ScrollRelative(-scroll_x, -scroll_y);

			UpdatePositionStatus();
			UpdateZoomStatus();
			Refresh();
			break;
		}
		// This will work like crap with non-us layouts, well, sucks for them until there is another solution.
		case '[':
		case '+': {
			g_editor.IncreaseBrushSize();
			Refresh();
			break;
		}
		case ']':
		case '-': {
			g_editor.DecreaseBrushSize();
			Refresh();
			break;
		}
		case WXK_NUMPAD_UP:
		case WXK_UP: {
			int start_x, start_y;
			window->GetViewStart(&start_x, &start_y);

			int tiles = 3;
			if(event.ControlDown())
				tiles = 10;
			else if(zoom == 1.0)
				tiles = 1;

			window->Scroll(start_x, int(start_y - rme::TileSize * tiles * zoom));
			UpdatePositionStatus();
			Refresh();
			break;
		}
		case WXK_NUMPAD_DOWN:
		case WXK_DOWN: {
			int start_x, start_y;
			window->GetViewStart(&start_x, &start_y);

			int tiles = 3;
			if(event.ControlDown())
				tiles = 10;
			else if(zoom == 1.0)
				tiles = 1;

			window->Scroll(start_x, int(start_y + rme::TileSize * tiles * zoom));
			UpdatePositionStatus();
			Refresh();
			break;
		}
		case WXK_NUMPAD_LEFT:
		case WXK_LEFT: {
			int start_x, start_y;
			window->GetViewStart(&start_x, &start_y);

			int tiles = 3;
			if(event.ControlDown())
				tiles = 10;
			else if(zoom == 1.0)
				tiles = 1;

			window->Scroll(int(start_x - rme::TileSize * tiles * zoom), start_y);
			UpdatePositionStatus();
			Refresh();
			break;
		}
		case WXK_NUMPAD_RIGHT:
		case WXK_RIGHT: {
			int start_x, start_y;
			window->GetViewStart(&start_x, &start_y);

			int tiles = 3;
			if(event.ControlDown())
				tiles = 10;
			else if(zoom == 1.0)
				tiles = 1;

			window->Scroll(int(start_x + rme::TileSize * tiles * zoom), start_y);
			UpdatePositionStatus();
			Refresh();
			break;
		}
		case WXK_SPACE: { // Utility keys
			if(event.ControlDown()) {
				g_editor.FillDoodadPreviewBuffer();
				g_editor.RefreshView();
			} else {
				g_editor.SwitchMode();
			}
			break;
		}
		case WXK_DELETE: { // Delete
			g_editor.destroySelection();
			g_editor.RefreshView();
			break;
		}
		case 'z':
		case 'Z': { // Rotate counterclockwise (actually shift variaton, but whatever... :P)
			int nv = g_editor.GetBrushVariation();
			--nv;
			if(nv < 0) {
				nv = std::max(0, (g_editor.GetCurrentBrush()? g_editor.GetCurrentBrush()->getMaxVariation() - 1 : 0));
			}
			g_editor.SetBrushVariation(nv);
			g_editor.RefreshView();
			break;
		}
		case 'x':
		case 'X': { // Rotate clockwise (actually shift variaton, but whatever... :P)
			int nv = g_editor.GetBrushVariation();
			++nv;
			if(nv >= (g_editor.GetCurrentBrush()? g_editor.GetCurrentBrush()->getMaxVariation() : 0)) {
				nv = 0;
			}
			g_editor.SetBrushVariation(nv);
			g_editor.RefreshView();
			break;
		}
		case 'q':
		case 'Q': { // Select previous brush
			g_editor.SelectPreviousBrush();
			break;
		}
		// Hotkeys
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9': {
			int index = event.GetKeyCode() - '0';
			if(event.ControlDown()) {
				Hotkey hk;
				if(g_editor.IsSelectionMode()) {
					int view_start_x, view_start_y;
					window->GetViewStart(&view_start_x, &view_start_y);
					int view_start_map_x = view_start_x / rme::TileSize, view_start_map_y = view_start_y / rme::TileSize;

					int view_screensize_x, view_screensize_y;
					window->GetViewSize(&view_screensize_x, &view_screensize_y);

					int map_x = int(view_start_map_x + (view_screensize_x * zoom) / rme::TileSize / 2);
					int map_y = int(view_start_map_y + (view_screensize_y * zoom) / rme::TileSize / 2);

					hk = Hotkey(Position(map_x, map_y, floor));
				} else if(g_editor.GetCurrentBrush()) {
					// Drawing mode
					hk = Hotkey(g_editor.GetCurrentBrush());
				} else {
					break;
				}
				g_editor.SetHotkey(index, hk);
			} else {
				// Click hotkey
				Hotkey hk = g_editor.GetHotkey(index);
				if(hk.IsPosition()) {
					g_editor.SetSelectionMode();

					int map_x = hk.GetPosition().x;
					int map_y = hk.GetPosition().y;
					int map_z = hk.GetPosition().z;

					window->Scroll(rme::TileSize * map_x, rme::TileSize * map_y, true);
					floor = map_z;

					g_editor.SetStatusText("Used hotkey " + i2ws(index));
					g_editor.RefreshView();
				} else if(hk.IsBrush()) {
					g_editor.SetDrawingMode();

					std::string name = hk.GetBrushname();
					Brush* brush = g_brushes.getBrush(name);
					if(brush == nullptr) {
						g_editor.SetStatusText("Brush \"" + wxstr(name) + "\" not found");
						return;
					}

					if(!g_editor.SelectBrush(brush)) {
						g_editor.SetStatusText("Brush \"" + wxstr(name) + "\" is not in any palette");
						return;
					}

					g_editor.SetStatusText("Used hotkey " + i2ws(index));
					g_editor.RefreshView();
				} else {
					g_editor.SetStatusText("Unassigned hotkey " + i2ws(index));
				}
			}
			break;
		}
		case 'd':
		case 'D': {
			keyCode = WXK_CONTROL_D;
			break;
		}
		default:{
			event.Skip();
			break;
		}
	}
}

void MapCanvas::OnKeyUp(wxKeyEvent& event)
{
	keyCode = WXK_NONE;
}

void MapCanvas::OnCopy(wxCommandEvent& WXUNUSED(event))
{
	if(g_editor.IsSelectionMode())
	   g_editor.copybuffer.copy(GetFloor());
}

void MapCanvas::OnCut(wxCommandEvent& WXUNUSED(event))
{
	if(g_editor.IsSelectionMode())
		g_editor.copybuffer.cut(GetFloor());
	g_editor.RefreshView();
}

void MapCanvas::OnPaste(wxCommandEvent& WXUNUSED(event))
{
	g_editor.DoPaste();
	g_editor.RefreshView();
}

void MapCanvas::OnDelete(wxCommandEvent& WXUNUSED(event))
{
	g_editor.destroySelection();
	g_editor.RefreshView();
}

void MapCanvas::OnCopyPosition(wxCommandEvent& WXUNUSED(event))
{
	if (g_editor.hasSelection()) {
		Position minPos, maxPos;
		g_editor.getSelection().getBounds(minPos, maxPos);
		if (minPos != maxPos) {
			posToClipboard(minPos.x, minPos.y, minPos.z, maxPos.x, maxPos.y, maxPos.z);
			return;
		}
	}

	int x, y;
	MouseToMap(&x, &y);
	posToClipboard(x, y, GetFloor(), g_settings.getInteger(Config::COPY_POSITION_FORMAT));
}

void MapCanvas::OnCopyTypeId(wxCommandEvent& WXUNUSED(event))
{
	ASSERT(g_editor.getSelection().size() == 1);
	if(wxTheClipboard->Open()) {
		Tile* tile = g_editor.getSelection().getSelectedTile();
		const Item *item = tile->getFirstSelectedItem();
		wxTextDataObject* obj = new wxTextDataObject();
		obj->SetText(i2ws(item->getID()));
		wxTheClipboard->SetData(obj);
		wxTheClipboard->Close();
	}
}

void MapCanvas::OnCopyName(wxCommandEvent& WXUNUSED(event))
{
	ASSERT(g_editor.getSelection().size() == 1);
	if(wxTheClipboard->Open()) {
		Tile* tile = g_editor.getSelection().getSelectedTile();
		const Item *item = tile->getFirstSelectedItem();
		wxTextDataObject* obj = new wxTextDataObject();
		obj->SetText(wxstr(item->getName()));
		wxTheClipboard->SetData(obj);
		wxTheClipboard->Close();
	}
}

void MapCanvas::OnBrowseTile(wxCommandEvent& WXUNUSED(event))
{
	if(g_editor.getSelection().size() != 1)
		return;

	Tile* tile = g_editor.getSelection().getSelectedTile();
	if(!tile) return;
	ASSERT(tile->isSelected());

	Tile newTile; newTile.deepCopy(*tile);
	wxDialog* w = new BrowseTileWindow(g_editor.root, &newTile, wxPoint(cursor_x, cursor_y));
	if(w->ShowModal() != 0) {
		Action *action = g_editor.actionQueue.createAction(ACTION_DELETE_TILES);
		action->changeTile(std::move(newTile));
		action->commit();
		g_editor.updateActions();
	}

	w->Destroy();
}

void MapCanvas::OnRotateItem(wxCommandEvent& WXUNUSED(event))
{
	if(!g_editor.hasSelection()) {
		return;
	}

	Selection& selection = g_editor.getSelection();
	if(selection.size() != 1) {
		return;
	}

	Tile *tile = selection.getSelectedTile();
	Item *item = tile->getFirstSelectedItem();
	if(!item || !item->getFlag(ROTATE)) {
		return;
	}

	Action *action = g_editor.actionQueue.createAction(ACTION_ROTATE_ITEM);
	Tile newTile; newTile.deepCopy(*tile);
	Item *newItem = newTile.getFirstSelectedItem();
	ASSERT(newItem->getID() == item->getID());
	newItem->transform(newItem->getAttribute(ROTATETARGET));
	action->changeTile(std::move(newTile));

	g_editor.updateActions();
	g_editor.RefreshView();
}

void MapCanvas::OnGotoDestination(wxCommandEvent& WXUNUSED(event))
{
	Tile *tile = g_editor.getSelection().getSelectedTile();
	Item *item = tile->getFirstSelectedItem();
	if(item && item->getFlag(TELEPORTABSOLUTE)){
		Position teleportDest = UnpackAbsoluteCoordinate(item->getAttribute(ABSTELEPORTDESTINATION));
		g_editor.SetScreenCenterPosition(teleportDest);
	}
}

void MapCanvas::OnCopyDestination(wxCommandEvent& WXUNUSED(event))
{
	Tile *tile = g_editor.getSelection().getSelectedTile();
	Item *item = tile->getFirstSelectedItem();
	if(item && item->getFlag(TELEPORTABSOLUTE)){
		Position teleportDest = UnpackAbsoluteCoordinate(item->getAttribute(ABSTELEPORTDESTINATION));
		int format = g_settings.getInteger(Config::COPY_POSITION_FORMAT);
		posToClipboard(teleportDest.x, teleportDest.y, teleportDest.z, format);

	}
}

void MapCanvas::OnSwitchDoor(wxCommandEvent& WXUNUSED(event))
{
	Tile *tile = g_editor.getSelection().getSelectedTile();
	Action *action = g_editor.actionQueue.createAction(ACTION_SWITCHDOOR);
	Tile newTile; newTile.deepCopy(*tile);
	DoorBrush::switchDoor(newTile.getFirstSelectedItem());
	action->changeTile(std::move(newTile));

	g_editor.updateActions();
	g_editor.RefreshView();
}

void MapCanvas::OnSelectRAWBrush(wxCommandEvent& WXUNUSED(event))
{
	if(g_editor.getSelection().size() != 1) return;
	Tile* tile = g_editor.getSelection().getSelectedTile();
	if(!tile) return;
	Item* item = tile->getLastSelectedItem();

	if(item && item->getRAWBrush())
		g_editor.SelectBrush(item->getRAWBrush(), TILESET_RAW);
}

void MapCanvas::OnSelectGroundBrush(wxCommandEvent& WXUNUSED(event))
{
	if(g_editor.getSelection().size() != 1) return;
	Tile* tile = g_editor.getSelection().getSelectedTile();
	if(!tile) return;
	GroundBrush* bb = tile->getGroundBrush();

	if(bb)
		g_editor.SelectBrush(bb, TILESET_TERRAIN);
}

void MapCanvas::OnSelectDoodadBrush(wxCommandEvent& WXUNUSED(event))
{
	if(g_editor.getSelection().size() != 1) return;
	Tile* tile = g_editor.getSelection().getSelectedTile();
	if(!tile) return;
	Item* item = tile->getLastSelectedItem();

	if(item)
		g_editor.SelectBrush(item->getDoodadBrush(), TILESET_DOODAD);
}

void MapCanvas::OnSelectDoorBrush(wxCommandEvent& WXUNUSED(event))
{
	if(g_editor.getSelection().size() != 1) return;
	Tile* tile = g_editor.getSelection().getSelectedTile();
	if(!tile) return;
	Item* item = tile->getLastSelectedItem();

	if(item)
		g_editor.SelectBrush(item->getDoorBrush(), TILESET_TERRAIN);
}

void MapCanvas::OnSelectWallBrush(wxCommandEvent& WXUNUSED(event))
{
	if(g_editor.getSelection().size() != 1) return;
	Tile* tile = g_editor.getSelection().getSelectedTile();
	if(!tile) return;
	Item* wall = tile->getWall();
	WallBrush* wb = wall->getWallBrush();

	if(wb)
		g_editor.SelectBrush(wb, TILESET_TERRAIN);
}

void MapCanvas::OnSelectCarpetBrush(wxCommandEvent& WXUNUSED(event))
{
	if(g_editor.getSelection().size() != 1) return;
	Tile* tile = g_editor.getSelection().getSelectedTile();
	if(!tile) return;
	Item* wall = tile->getCarpet();
	CarpetBrush* cb = wall->getCarpetBrush();

	if(cb)
		g_editor.SelectBrush(cb);
}

void MapCanvas::OnSelectTableBrush(wxCommandEvent& WXUNUSED(event))
{
	if(g_editor.getSelection().size() != 1) return;
	Tile* tile = g_editor.getSelection().getSelectedTile();
	if(!tile) return;
	Item* wall = tile->getTable();
	TableBrush* tb = wall->getTableBrush();

	if(tb)
		g_editor.SelectBrush(tb);
}

void MapCanvas::OnSelectHouseBrush(wxCommandEvent& WXUNUSED(event))
{
	Tile* tile = g_editor.getSelection().getSelectedTile();
	if(!tile)
		return;

#if TODO
	if(tile->houseId != 0) {
		House* house = g_editor.map.houses.getHouse(tile->getHouseID());
		if(house) {
			g_editor.house_brush->setHouse(house);
			g_editor.SelectBrush(g_editor.house_brush, TILESET_HOUSE);
		}
	}
#endif
}

void MapCanvas::OnSelectCreatureBrush(wxCommandEvent& WXUNUSED(event))
{
	Tile* tile = g_editor.getSelection().getSelectedTile();
	if(!tile)
		return;

	if(tile->creature)
		g_editor.SelectBrush(tile->creature->getBrush(), TILESET_CREATURE);
}

void MapCanvas::OnProperties(wxCommandEvent& WXUNUSED(event))
{
	if(g_editor.getSelection().size() != 1)
		return;

	Tile* tile = g_editor.getSelection().getSelectedTile();
	if(!tile) return;
	ASSERT(tile->isSelected());

	wxDialog* w = nullptr;
	Tile newTile; newTile.deepCopy(*tile);
	if(newTile.creature && g_settings.getInteger(Config::SHOW_CREATURES)){
		w = newd CreaturePropertyWindow(g_editor.root, newTile.creature);
	}else if(Item *item = newTile.getLastSelectedItem()){
		w = newd ItemPropertyWindow(g_editor.root, item);
	}else{
		return;
	}

	if(w->ShowModal() != 0) {
		Action *action = g_editor.actionQueue.createAction(ACTION_CHANGE_PROPERTIES);
		action->changeTile(std::move(newTile));
		action->commit();
	}

	w->Destroy();
}

void MapCanvas::ChangeFloor(int new_floor)
{
	ASSERT(new_floor >= 0 || new_floor <= rme::MapMaxLayer);
	int old_floor = floor;
	floor = new_floor;
	if(old_floor != new_floor) {
		UpdatePositionStatus();
		g_editor.root->UpdateFloorMenu();
		g_editor.UpdateMinimap(true);
	}
	Refresh();
}

void MapCanvas::EnterDrawingMode()
{
	dragging = false;
	boundbox_selection = false;
	EndPasting();
	Refresh();
}

void MapCanvas::EnterSelectionMode()
{
	drawing = false;
	dragging_draw = false;
	Refresh();
}

bool MapCanvas::isPasting() const
{
	return g_editor.IsPasting();
}

void MapCanvas::StartPasting()
{
	g_editor.StartPasting();
}

void MapCanvas::EndPasting()
{
	g_editor.EndPasting();
}

void MapCanvas::Reset()
{
	cursor_x = 0;
	cursor_y = 0;

	zoom = 1.0;
	floor = rme::MapGroundLayer;

	dragging = false;
	boundbox_selection = false;
	screendragging = false;
	drawing = false;
	dragging_draw = false;

	drag_start_x = -1;
	drag_start_y = -1;
	drag_start_z = -1;

	last_click_map_x = -1;
	last_click_map_y = -1;
	last_click_map_z = -1;

	last_mmb_click_x = -1;
	last_mmb_click_y = -1;
}

void MapPopupMenu::Update()
{
	// Clear the menu of all items
	while(GetMenuItemCount() != 0) {
		wxMenuItem* m_item = FindItemByPosition(0);
		// If you add a submenu, this won't delete it.
		Delete(m_item);
	}

	bool anything_selected = g_editor.hasSelection();

	wxMenuItem* cutItem = Append(MAP_POPUP_MENU_CUT, "&Cut\tCTRL+X", "Cut out all selected items");
	cutItem->Enable(anything_selected);

	wxMenuItem* copyItem = Append(MAP_POPUP_MENU_COPY, "&Copy\tCTRL+C", "Copy all selected items");
	copyItem->Enable(anything_selected);

	wxMenuItem* copyPositionItem = Append(MAP_POPUP_MENU_COPY_POSITION, "&Copy Position", "Copy the position as a lua table");
	copyPositionItem->Enable(true);

	wxMenuItem* pasteItem = Append(MAP_POPUP_MENU_PASTE, "&Paste\tCTRL+V", "Paste items in the copybuffer here");
	pasteItem->Enable(g_editor.copybuffer.canPaste());

	wxMenuItem* deleteItem = Append(MAP_POPUP_MENU_DELETE, "&Delete\tDEL", "Removes all seleceted items");
	deleteItem->Enable(anything_selected);

	if(anything_selected) {
		if(g_editor.getSelection().size() == 1) {
			Tile* tile = g_editor.getSelection().getSelectedTile();
			bool hasWall = false;
			bool hasCarpet = false;
			bool hasTable = false;
			Item* topSelectedItem = tile->getLastSelectedItem();
			Creature* topCreature = tile->creature;

			for(Item *item = tile->items; item != NULL; item = item->next){
				if(item->isWall()) {
					Brush* wb = item->getWallBrush();
					if(wb && wb->visibleInPalette()) hasWall = true;
				}
				if(item->isTable()) {
					Brush* tb = item->getTableBrush();
					if(tb && tb->visibleInPalette()) hasTable = true;
				}
				if(item->isCarpet()) {
					Brush* cb = item->getCarpetBrush();
					if(cb && cb->visibleInPalette()) hasCarpet = true;
				}
			}

			AppendSeparator();

			if(topSelectedItem) {
				Append(MAP_POPUP_MENU_COPY_TYPE_ID, "Copy Item Type Id", "Copy the type id of this item");
				Append(MAP_POPUP_MENU_COPY_NAME, "Copy Item Name", "Copy the name of this item");
				AppendSeparator();
			}

			if(topSelectedItem || topCreature) {
				if(topSelectedItem && (topSelectedItem->isBrushDoor() || topSelectedItem->getFlag(ROTATE) || topSelectedItem->getFlag(TELEPORTABSOLUTE))) {
					if(topSelectedItem->getFlag(ROTATE))
						Append(MAP_POPUP_MENU_ROTATE, "&Rotate item", "Rotate this item");

					if(topSelectedItem->getFlag(TELEPORTABSOLUTE)) {
						bool enabled = (topSelectedItem->getAttribute(ABSTELEPORTDESTINATION) != 0);
						wxMenuItem* goto_menu = Append(MAP_POPUP_MENU_GOTO, "&Go To Destination", "Go to the destination of this teleport");
						goto_menu->Enable(enabled);
						wxMenuItem* dest_menu = Append(MAP_POPUP_MENU_COPY_DESTINATION, "Copy &Destination", "Copy the destination of this teleport");
						dest_menu->Enable(enabled);
						AppendSeparator();
					}

#if TODO
					// TODO(fusion): We may still find a way to re-enable this with the `editor.xml` file.
					if(topSelectedItem->isDoor())
					{
						if(topSelectedItem->isOpen()) {
							Append(MAP_POPUP_MENU_SWITCH_DOOR, "&Close door", "Close this door");
						} else {
							Append(MAP_POPUP_MENU_SWITCH_DOOR, "&Open door", "Open this door");
						}
						AppendSeparator();
					}
#endif
				}

				if(topCreature)
					Append( MAP_POPUP_MENU_SELECT_CREATURE_BRUSH, "Select Creature", "Uses the current creature as a creature brush");

				Append( MAP_POPUP_MENU_SELECT_RAW_BRUSH, "Select RAW", "Uses the top item as a RAW brush");

				if(hasWall)
					Append( MAP_POPUP_MENU_SELECT_WALL_BRUSH, "Select Wallbrush", "Uses the current item as a wallbrush");

				if(hasCarpet)
					Append( MAP_POPUP_MENU_SELECT_CARPET_BRUSH, "Select Carpetbrush", "Uses the current item as a carpetbrush");

				if(hasTable)
					Append( MAP_POPUP_MENU_SELECT_TABLE_BRUSH, "Select Tablebrush", "Uses the current item as a tablebrush");

				if(topSelectedItem && topSelectedItem->getDoodadBrush() && topSelectedItem->getDoodadBrush()->visibleInPalette())
					Append( MAP_POPUP_MENU_SELECT_DOODAD_BRUSH, "Select Doodadbrush", "Use this doodad brush");

				if(topSelectedItem && topSelectedItem->isBrushDoor() && topSelectedItem->getDoorBrush())
					Append( MAP_POPUP_MENU_SELECT_DOOR_BRUSH, "Select Doorbrush", "Use this door brush");

				if(tile->getGroundBrush() && tile->getGroundBrush()->visibleInPalette())
					Append( MAP_POPUP_MENU_SELECT_GROUND_BRUSH, "Select Groundbrush", "Uses the current item as a groundbrush");

				if(tile->houseId != 0)
					Append(MAP_POPUP_MENU_SELECT_HOUSE_BRUSH, "Select House", "Draw with the house on this tile.");

				AppendSeparator();
				Append( MAP_POPUP_MENU_PROPERTIES, "&Properties", "Properties for the current object");
			} else {

				if(topCreature)
					Append( MAP_POPUP_MENU_SELECT_CREATURE_BRUSH, "Select Creature", "Uses the current creature as a creature brush");

				Append( MAP_POPUP_MENU_SELECT_RAW_BRUSH, "Select RAW", "Uses the top item as a RAW brush");
				if(hasWall) {
					Append( MAP_POPUP_MENU_SELECT_WALL_BRUSH, "Select Wallbrush", "Uses the current item as a wallbrush");
				}
				if(tile->getGroundBrush() && tile->getGroundBrush()->visibleInPalette()) {
					Append( MAP_POPUP_MENU_SELECT_GROUND_BRUSH, "Select Groundbrush", "Uses the current tile as a groundbrush");
				}

				if(tile->houseId != 0) {
					Append(MAP_POPUP_MENU_SELECT_HOUSE_BRUSH, "Select House", "Draw with the house on this tile.");
				}

				if(tile->getFlag(BANK) || topCreature) {
					AppendSeparator();
					Append( MAP_POPUP_MENU_PROPERTIES, "&Properties", "Properties for the current object");
				}
			}

			AppendSeparator();

			wxMenuItem* browseTile = Append(MAP_POPUP_MENU_BROWSE_TILE, "Browse Field", "Navigate from tile items");
			browseTile->Enable(anything_selected);
		}
	}
}

void MapCanvas::getTilesToDraw(int mouse_map_x, int mouse_map_y, int floor, std::vector<Position>* tilesToDraw, std::vector<Position>* tilesToBorderize, bool fill /*= false*/)
{
	if(fill) {
		Brush* brush = g_editor.GetCurrentBrush();
		if(!brush || !brush->isGround()) {
			return;
		}

		GroundBrush* newBrush = brush->asGround();
		Position position(mouse_map_x, mouse_map_y, floor);

		Tile* tile = g_editor.map.getTile(position);
		GroundBrush* oldBrush = nullptr;
		if(tile) {
			oldBrush = tile->getGroundBrush();
		}

		if(oldBrush && oldBrush->getID() == newBrush->getID()) {
			return;
		}

		if((tile && tile->getFlag(BANK) && !oldBrush) || (!tile && oldBrush)) {
			return;
		}

		if(tile && oldBrush) {
			GroundBrush* groundBrush = tile->getGroundBrush();
			if(!groundBrush || groundBrush->getID() != oldBrush->getID()) {
				return;
			}
		}

		std::fill(std::begin(processed), std::end(processed), false);
		floodFill(&g_editor.map, position, BLOCK_SIZE/2, BLOCK_SIZE/2, oldBrush, tilesToDraw);

	} else {
		for(int y = -g_editor.GetBrushSize() - 1; y <= g_editor.GetBrushSize() + 1; y++) {
			for(int x = -g_editor.GetBrushSize() - 1; x <= g_editor.GetBrushSize() + 1; x++) {
				if(g_editor.GetBrushShape() == BRUSHSHAPE_SQUARE) {
					if(x >= -g_editor.GetBrushSize() && x <= g_editor.GetBrushSize() && y >= -g_editor.GetBrushSize() && y <= g_editor.GetBrushSize()) {
						if(tilesToDraw)
							tilesToDraw->push_back(Position(mouse_map_x + x, mouse_map_y + y, floor));
					}
					if(std::abs(x) - g_editor.GetBrushSize() < 2 && std::abs(y) - g_editor.GetBrushSize() < 2) {
						if(tilesToBorderize)
							tilesToBorderize->push_back(Position(mouse_map_x + x, mouse_map_y + y, floor));
					}
				} else if(g_editor.GetBrushShape() == BRUSHSHAPE_CIRCLE) {
					double distance = sqrt(double(x*x) + double(y*y));
					if(distance < g_editor.GetBrushSize() + 0.005) {
						if(tilesToDraw)
							tilesToDraw->push_back(Position(mouse_map_x + x, mouse_map_y + y, floor));
					}
					if(std::abs(distance - g_editor.GetBrushSize()) < 1.5) {
						if(tilesToBorderize)
							tilesToBorderize->push_back(Position(mouse_map_x + x, mouse_map_y + y, floor));
					}
				}
			}
		}
	}
}

bool MapCanvas::floodFill(Map *map, const Position& center, int x, int y, GroundBrush* brush, std::vector<Position>* positions)
{
	if(x < 0 || y < 0 || x > BLOCK_SIZE || y > BLOCK_SIZE) {
		return false;
	}

	processed[getFillIndex(x, y)] = true;

	int px = (center.x + x) - (BLOCK_SIZE/2);
	int py = (center.y + y) - (BLOCK_SIZE/2);
	if(px <= 0 || py <= 0 || px >= map->getWidth() || py >= map->getHeight()) {
		return false;
	}

	Tile* tile = map->getTile(px, py, center.z);
	if((tile && tile->getFlag(BANK) && !brush) || (!tile && brush)) {
		return false;
	}

	if(tile && brush) {
		GroundBrush* groundBrush = tile->getGroundBrush();
		if(!groundBrush || groundBrush->getID() != brush->getID()) {
			return false;
		}
	}

	positions->push_back(Position(px, py, center.z));

	bool deny = false;
	if(!processed[getFillIndex(x-1, y)]) {
		deny = floodFill(map, center, x-1, y, brush, positions);
	}

	if(!deny && !processed[getFillIndex(x, y-1)]) {
		deny = floodFill(map, center, x, y-1, brush, positions);
	}

	if(!deny && !processed[getFillIndex(x+1, y)]) {
		deny = floodFill(map, center, x+1, y, brush, positions);
	}

	if(!deny && !processed[getFillIndex(x, y+1)]) {
		deny = floodFill(map, center, x, y+1, brush, positions);
	}

	return deny;
}

