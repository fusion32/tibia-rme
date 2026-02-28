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

#include "brush.h"

#include "tile.h"
#include "creature.h"
#include "house.h"
#include "ground_brush.h"
#include "wall_brush.h"
#include "carpet_brush.h"
#include "table_brush.h"

void Tile::clear(void)
{
	clearItems();
	clearCreature();
	houseId  = 0;
	flags    = 0;
}

void Tile::swap(Tile &other){
	std::swap(items, other.items);
	std::swap(creature, other.creature);
	std::swap(pos, other.pos);
	std::swap(houseId, other.houseId);
	std::swap(flags, other.flags);
}

void Tile::deepCopy(const Tile &other)
{
	clear();

	pos     = other.pos;
	houseId = other.houseId;
	flags   = other.flags;

	if(other.creature){
		creature = other.creature->deepCopy();
	}

	if(other.items){
		Item **tail = &items;
		for(const Item *it = other.items; it != NULL; it = it->next){
			*tail = it->deepCopy();
			tail = &(*tail)->next;
		}
	}
}

Tile *Tile::deepCopy(void) const
{
	Tile *copy = newd Tile();
	copy->deepCopy(*this);
	return copy;
}

void Tile::mergeCopy(const Tile &other)
{
	flags |= other.flags;

	if(other.houseId) {
		houseId = other.houseId;
	}

	if(other.creature) {
		delete creature;
		creature = other.creature->deepCopy();
	}

	for(const Item *item = other.items; item != NULL; item = item->next){
		addItem(item->deepCopy());
	}
}

void Tile::merge(Tile &&other)
{
	flags |= other.flags;

	if(other.houseId) {
		houseId = other.houseId;
	}

	if(other.creature) {
		delete creature;
		creature = other.creature;
		other.creature = NULL;
	}

	while(Item *first = other.items){
		other.items = first->next;
		first->next = NULL;
		addItem(first);
	}
}

int Tile::countItems(void) const
{
	int count = 0;
	for(Item *it = items; it != NULL; it = it->next){
		count += 1;
	}
	return count;
}

uint32_t Tile::memsize() const
{
	uint32_t result = sizeof(Tile) + countItems() * sizeof(Item);
	if(creature != NULL){
		result += sizeof(Creature);
	}
	return result;
}

bool Tile::empty() const
{
	return items == NULL && creature == NULL;
}

void Tile::select()
{
	if(creature) creature->select();
	for(Item *it = items; it != NULL; it = it->next){
		it->select();
	}
}

void Tile::deselect()
{
	if(creature) creature->deselect();
	for(Item *it = items; it != NULL; it = it->next){
		it->deselect();
	}
}

void Tile::selectGround()
{
	for(Item *it = items; it != NULL; it = it->next){
		if(!it->getFlag(BANK) && !it->getFlag(CLIP)){
			break;
		}

		it->select();
	}
}

void Tile::deselectGround()
{
	for(Item *it = items; it != NULL; it = it->next){
		if(!it->getFlag(BANK) && !it->getFlag(CLIP)){
			break;
		}

		it->deselect();
	}
}

Item *Tile::popSelectedItems()
{
	Item *result = NULL;
	Item **tail = &result;
	Item **it   = &items;
	while(*it != NULL){
		if((*it)->isSelected()){
			*tail = (*it);			// insert into result list
			*it   = (*it)->next;	// remove from tile list (don't need to advance)
			tail  = &(*tail)->next; // advance result list
			*tail = NULL;			// make sure we don't cross reference
		}else{
			it = &(*it)->next;      // advance tile list
		}
	}
	return result;
}

void Tile::clearItems(void)
{
	while(Item *it = items){
		items = it->next;
		it->next = NULL;
		delete it;
	}
}

void Tile::addItem(Item *item, bool replaceUnique /*= true*/)
{
	if(!item) return;

	ASSERT(item->next == NULL);
	int stackPriority = item->getStackPriority();

	// IMPORTANT(fusion): Objects with CREATURE and LOW priority are stored in
	// reverse order, to allow the list to contain the most important/immediate
	// objects in its first ~10 entries.

	bool replace = replaceUnique
			&& (stackPriority == STACK_PRIORITY_BANK
				|| stackPriority == STACK_PRIORITY_BOTTOM
				|| stackPriority == STACK_PRIORITY_TOP);

	bool append = !replace
			&& (stackPriority == STACK_PRIORITY_BANK
				|| stackPriority == STACK_PRIORITY_CLIP
				|| stackPriority == STACK_PRIORITY_BOTTOM
				|| stackPriority == STACK_PRIORITY_TOP);

	Item **it = &items;
	while(*it != NULL){
		if( append && (*it)->getStackPriority() >  stackPriority) break;
		if(!append && (*it)->getStackPriority() >= stackPriority) break;
		it = &(*it)->next;
	}

	if(replace && *it != NULL && (*it)->getStackPriority() == stackPriority){
		item->next = (*it)->next;
		(*it)->next = NULL;
		delete (*it);
		*it = item;
	}else{
		item->next = *it;
		*it = item;
	}
}

static Item *ReverseStackGroup(Item *first){
	if(first != NULL){
		Item *prev        = first;
		Item *it          = first->next;
		int stackPriority = first->getStackPriority();
		while(it != NULL && it->getStackPriority() == stackPriority){
			Item *next = it->next;
			it->next = prev;
			prev = it;
			it = next;
		}

		// NOTE(fusion): After reversing, `prev` should contain the new head of the
		// group, while `first`, the new tail. We still need to make sure we don't
		// break the link to the remainder of the list so we need to adjust the tail
		// to point to the first object of the next group.
		if(prev != first){
			first->next = it;
			first       = prev;
		}
	}
	return first;
}

int Tile::addItems(Item *first, bool replaceUnique /*= true*/)
{
	// NOTE(fusion): We want a stable insertion and we know that CREATURE and LOW
	// stack priority objects are stored in reverse order. This means we also need
	// to insert these objects in reverse to get a stable result.

	int count = 0;
	int prevStackPriority = -1;
	while(Item *item = first){
		int stackPriority = item->getStackPriority();
		if(stackPriority != prevStackPriority){
			if(stackPriority == STACK_PRIORITY_CREATURE
					|| stackPriority == STACK_PRIORITY_LOW){
				item = ReverseStackGroup(item);
			}

			prevStackPriority = stackPriority;
		}

		first = item->next;
		item->next = NULL;
		addItem(item, replaceUnique);
		count += 1;
	}
	return count;
}

void Tile::clearCreature(void){
	delete creature;
	creature = NULL;
}

void Tile::placeCreature(int raceId, int spawnRadius, int spawnAmount, int spawnInterval){
	delete creature;
	creature = newd Creature();
	creature->raceId = raceId;
	creature->spawnRadius = spawnRadius;
	creature->spawnAmount = spawnAmount;
	creature->spawnInterval = spawnInterval;
	creature->selected = false;
}

int Tile::getIndexOf(Item *item) const
{
	if(item){
		int index = 0;
		for(Item *it = items; it != NULL; it = it->next){
			if(it == item){
				return index;
			}
			index += 1;
		}
	}
	return wxNOT_FOUND;
}

Item *Tile::getItemAt(int index) const
{
	Item *result = items;
	while(result && index > 0){
		result = result->next;
		index -= 1;
	}
	return result;
}

Item *Tile::getTopItem(void) const
{
	Item *result = NULL;
	for(Item *it = items; it != NULL; it = it->next){
		result = it;

		int stackPriority = result->getStackPriority();
		if(stackPriority == STACK_PRIORITY_CREATURE
				|| stackPriority == STACK_PRIORITY_LOW){
			break;
		}
	}
	return result;
}

bool Tile::getFlag(ObjectFlag flag) const
{
	for(Item *it = items; it != NULL; it = it->next){
		if(it->getFlag(flag)){
			return true;
		}
	}
	return false;
}

uint16_t Tile::getGroundSpeed(void) const noexcept
{
	if(items && items->getFlag(BANK)){
		return items->getAttribute(WAYPOINTS);
	}
	return 0;
}

uint8_t Tile::getMiniMapColor(void) const
{
	uint8_t result = 0;
	for(Item *it = items; it != NULL; it = it->next){
		uint8_t color = it->getMiniMapColor();
		if(color != 0){
			result = color;
		}
	}

	return result;
}

GroundBrush* Tile::getGroundBrush() const
{
	if(items && items->getFlag(BANK)){
		return items->getGroundBrush();
	}else{
		return nullptr;
	}
}

void Tile::borderize(Map *parent)
{
	GroundBrush::doBorders(parent, this);
}

void Tile::wallize(Map *parent)
{
	WallBrush::doWalls(parent, this);
}

void Tile::tableize(Map *parent)
{
	TableBrush::doTables(parent, this);
}

void Tile::carpetize(Map *parent)
{
	CarpetBrush::doCarpets(parent, this);
}

