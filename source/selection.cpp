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
#include "selection.h"
#include "tile.h"
#include "creature.h"
#include "item.h"
#include "editor.h"
#include "settings.h"

void Selection::getBounds(Position &minPos, Position &maxPos) const
{
	if(tiles.empty()){
		minPos = Position(0, 0, 0);
		maxPos = Position(0, 0, 0);
		return;
	}

	minPos = Position(0x10000, 0x10000, 0x10);
	maxPos = Position(0, 0, 0);
	for(const Tile *tile: tiles) {
		ASSERT(tile);
		Position pos = tile->pos;
		if(minPos.x > pos.x) minPos.x = pos.x;
		if(minPos.y > pos.y) minPos.y = pos.y;
		if(minPos.z > pos.z) minPos.z = pos.z;
		if(maxPos.x < pos.x) maxPos.x = pos.x;
		if(maxPos.y < pos.y) maxPos.y = pos.y;
		if(maxPos.z < pos.z) maxPos.z = pos.z;
	}
}

// IMPORTANT(fusion): The actual tile set isn't modified in these functions but
// rather delayed until Action::commit is called.

void Selection::add(Action *action, Tile *tile, Item *item)
{
	ASSERT(tile->getIndexOf(item) != wxNOT_FOUND);
	if(!item->isSelected()){
		item->select();
		Tile newTile; newTile.deepCopy(*tile);
		item->deselect();

		if(g_settings.getInteger(Config::BORDER_IS_GROUND)) {
			if(item->getFlag(CLIP))
				newTile.selectGround();
		}

		if(action == NULL){
			action = g_editor.actionQueue.createAction(ACTION_SELECT, 1);
			action->changeTile(std::move(newTile));
			action->commit();
		}else{
			action->changeTile(std::move(newTile));
		}
	}
}

void Selection::add(Action *action, Tile *tile, Creature *creature)
{
	ASSERT(tile->creature == creature);
	if(!creature->isSelected()){
		creature->select();
		Tile newTile; newTile.deepCopy(*tile);
		creature->deselect();

		if(action == NULL){
			action = g_editor.actionQueue.createAction(ACTION_SELECT, 1);
			action->changeTile(std::move(newTile));
			action->commit();
		}else{
			action->changeTile(std::move(newTile));
		}
	}
}

void Selection::add(Action *action, Tile *tile)
{
	Tile newTile; newTile.deepCopy(*tile);
	newTile.select();

	if(action == NULL){
		action = g_editor.actionQueue.createAction(ACTION_SELECT, 1);
		action->changeTile(std::move(newTile));
		action->commit();
	}else{
		action->changeTile(std::move(newTile));
	}
}

void Selection::remove(Action *action, Tile *tile, Item *item)
{
	ASSERT(tile->getIndexOf(item) != wxNOT_FOUND);
	if(item->isSelected()){
		item->deselect();
		Tile newTile; newTile.deepCopy(*tile);
		item->select();

		if(g_settings.getInteger(Config::BORDER_IS_GROUND)){
			if(item->getFlag(CLIP)){
				newTile.deselectGround();
			}
		}

		if(action == NULL){
			action = g_editor.actionQueue.createAction(ACTION_UNSELECT, 1);
			action->changeTile(std::move(newTile));
			action->commit();
		}else{
			action->changeTile(std::move(newTile));
		}
	}
}

void Selection::remove(Action *action, Tile *tile, Creature *creature)
{
	ASSERT(tile->creature == creature);
	if(creature->isSelected()){
		creature->deselect();
		Tile newTile; newTile.deepCopy(*tile);
		creature->select();

		if(action == NULL){
			action = g_editor.actionQueue.createAction(ACTION_UNSELECT, 1);
			action->changeTile(std::move(newTile));
			action->commit();
		}else{
			action->changeTile(std::move(newTile));
		}
	}
}

void Selection::remove(Action *action, Tile *tile)
{
	Tile newTile; newTile.deepCopy(*tile);
	newTile.deselect();

	if(action == NULL){
		action = g_editor.actionQueue.createAction(ACTION_UNSELECT, 1);
		action->changeTile(std::move(newTile));
		action->commit();
	}else{
		action->changeTile(std::move(newTile));
	}
}

void Selection::clear(Action *action)
{
	if(tiles.empty()){
		return;
	}

	if(action == NULL){
		action = g_editor.actionQueue.createAction(ACTION_UNSELECT);
		for(Tile *tile: tiles){
			remove(action, tile);
		}
		action->commit();
	}else{
		for(Tile *tile: tiles){
			remove(action, tile);
		}
	}
}

void Selection::addInternal(Tile* tile)
{
	ASSERT(tile != NULL);
	tiles.insert(tile);
}

void Selection::removeInternal(Tile* tile)
{
	ASSERT(tile != NULL);
	tiles.erase(tile);
}

void Selection::updateSelectionCount()
{
	size_t count = tiles.size();
	if(count > 1){
		g_editor.SetStatusText(wxString() << count << " tiles selected.");
	}else if(count == 1){
		g_editor.SetStatusText("One tile selected.");
	}
}

