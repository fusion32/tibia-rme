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
#include "script.h"

#include <wx/dir.h>
#include <wx/zipstrm.h>
#include <wx/wfstream.h>

static wxString BackupName(const wxString &name){
	return wxString() << name << (name.IsEmpty() ? "" : "-")
		<< wxDateTime().UNow().Format("%Y-%m-%d-%H%M%S%l.zip");
}

static bool BackupFiles(const wxFileName &outputName,
			const wxString &baseDir, const wxArrayString &filenames){
	if(!outputName.DirExists() && !wxDir::Make(outputName.GetPath(), 0755, wxPATH_MKDIR_FULL)){
		g_editor.Error(wxString() << "Unable to create backup directory " << outputName.GetPath());
		return false;
	}

	wxTempFileOutputStream outputFile(outputName.GetFullPath());
	if(!outputFile.IsOk()){
		g_editor.Error(wxString() << "Failed to create backup file " << outputName.GetFullName());
		return false;
	}

	{
		int numDone = 0;
		int numFiles = (int)filenames.size();
		wxZipOutputStream zip(outputFile, 9);
		for(const wxString &filename: filenames){
			wxFileName relativeName(filename);
			if(!relativeName.MakeRelativeTo(baseDir)){
				g_editor.Warning(wxString() << "Backup base directory "
						<< baseDir << " doesn't contain " << filename);
				numDone += 1;
				continue;
			}

			g_editor.SetLoadDone((numDone * 100) / numFiles,
					wxString() << "Backing up " << relativeName.GetFullName() << " ...");

			wxFileInputStream inputStream(filename);
			if(!inputStream.IsOk()){
				g_editor.Error(wxString() << "Failed to open file " << filename << " for reading");
				return false;
			}

			if(!zip.PutNextEntry(relativeName.GetFullPath())){
				g_editor.Error(wxString() << "Failed to append ZIP entry for " << relativeName.GetFullPath());
				return false;
			}

			zip << inputStream;
			numDone += 1;
		}

		if(!zip.Close()){
			g_editor.Error(wxString() << "Failed to wrap backup file " << outputName.GetFullName());
			return false;
		}
	}

	if(!outputFile.Commit()){
		g_editor.Error(wxString() << "Failed to commit backup file " << outputName.GetFullName());
		return false;
	}

	return true;
}

static void RemoveFiles(const wxArrayString &filenames){
	for(const wxString &filename: filenames){
		if(!wxRemoveFile(filename)){
			g_editor.Warning(wxString() << "Unable to delete file " << filename);
		}
	}
}

static Item *LoadObjects(Script *script){
	ASSERT(script != NULL);

	Item *items = NULL;
	Item **tail = &items;
	Item *currentItem = NULL;
	script->readSymbol('{');
	script->nextToken();
	while(!script->eof()){
		if(currentItem == NULL){
			if(script->token.kind != TOKEN_SPECIAL){
				int typeId = script->getNumber();
				if(!ItemTypeExists(typeId)){
					script->error("invalid object type");
					break;
				}

				currentItem = newd Item(typeId);
				*tail = currentItem;
				tail = &currentItem->next;
			}else{
				int special = script->getSpecial();
				if(special == '}') break;
				if(special != ','){
					script->error("expected comma");
					break;
				}
			}
			script->nextToken();
		}else{
			if(script->token.kind != TOKEN_SPECIAL){
				int attr = GetInstanceAttributeByName(script->getIdentifier());
				if(attr == -1){
					script->error("unknown attribute");
					break;
				}

				script->readSymbol('=');
				if(attr == CONTENT){
					// NOTE(fusion): Just to be sure...
					Item **contentTail = &currentItem->content;
					while(*contentTail){
						contentTail = &(*contentTail)->next;
					}

					*contentTail = LoadObjects(script);
				}else if(attr == TEXTSTRING || attr == EDITOR){
					currentItem->setTextAttribute((ObjectInstanceAttribute)attr, script->readString());
				}else{
					currentItem->setAttribute((ObjectInstanceAttribute)attr, script->readNumber());
				}

				script->nextToken();
			}else{
				// NOTE(fusion): Attributes are key-value pairs separated by space.
				// If we find a special token (probably ',' or '}'), then we're done
				// parsing attributes for the current object.
				currentItem = NULL;
			}
		}
	}
	return items;
}

static void SaveObjects(ScriptWriter *script, const Item *first){
	ASSERT(script != NULL);
	script->writeText("{");
	for(const Item *item = first; item != NULL; item = item->next){
		if(item != first) script->writeText(", ");
		script->writeNumber(item->getID());

		for(int attr = 0; attr < NUM_INSTANCE_ATTRIBUTES; attr += 1){
			if(attr == CONTENT
					|| item->getAttributeOffset((ObjectInstanceAttribute)attr) == -1
					|| item->getAttribute((ObjectInstanceAttribute)attr) == 0){
				continue;
			}

			script->writeText(" ");
			script->writeText(GetInstanceAttributeName(attr));
			script->writeText("=");
			if(attr == TEXTSTRING || attr == EDITOR){
				script->writeString(item->getTextAttribute((ObjectInstanceAttribute)attr));
			}else{
				script->writeNumber(item->getAttribute((ObjectInstanceAttribute)attr));
			}
		}

		if(item->getAttributeOffset(CONTENT) != -1 && item->content != NULL){
			script->writeText(" Content=");
			SaveObjects(script, item->content);
		}
	}
	script->writeText("}");
}

static void SaveTile(ScriptWriter *script, int offsetX, int offsetY, const Tile *tile){
	ASSERT(script != NULL && tile != NULL);

	script->writeNumber(offsetX);
	script->writeText("-");
	script->writeNumber(offsetY);
	script->writeText(": ");

	if(tile->getTileFlag(TILE_FLAG_REFRESH)){
		script->writeText("Refresh, ");
	}

	if(tile->getTileFlag(TILE_FLAG_NOLOGOUT)){
		script->writeText("NoLogout, ");
	}

	if(tile->getTileFlag(TILE_FLAG_PROTECTIONZONE)){
		script->writeText("ProtectionZone, ");
	}

	script->writeText("Content=");
	SaveObjects(script, tile->items);

	script->writeLn();
}

void Map::loadSector(SectorType type, MapSector *sector, Script *script){
	ASSERT(sector != NULL && script != NULL);

#if _DEBUG
	// NOTE(fusion): Baseline sectors should be loaded before others.
	if(type == SECTOR_BASELINE){
		for(Tile &tile: sector->tiles){
			if(tile.items != NULL || tile.flags != 0){
				g_editor.Warning("Non-empty tile while loading baseline sector",
						ProblemSource::FromPosition(tile.pos));
				tile.clear();
			}
		}
	}
#endif

	// NOTE(fusion): A full patch replaces the whole sector.
	if(type == SECTOR_FULL_PATCH){
		for(Tile &tile: sector->tiles){
			tile.clear();
			tile.setTileFlag(TILE_FLAG_DIRTY);
		}
	}

	Tile *tile = NULL;
	std::string ident;
	while(true){
		script->nextToken();
		if(script->eof()){
			break;
		}

		if(script->token.kind == TOKEN_SPECIAL && script->getSpecial() == ','){
			continue;
		}

		if(script->token.kind == TOKEN_BYTES){
			const uint8_t *offset = script->getBytes();
			int offsetX = (int)offset[0];
			int offsetY = (int)offset[1];
			script->readSymbol(':');

			tile = sector->getTile(offsetX, offsetY);
			if(tile == NULL){
				script->error("invalid sector offset");
				break;
			}

			// NOTE(fusion): A regular patch replaces only specified tiles.
			if(type == SECTOR_PATCH){
				tile->clear();
				tile->setTileFlag(TILE_FLAG_DIRTY);
			}

		}else if(script->token.kind == TOKEN_IDENTIFIER){
			if(tile == NULL){
				script->error("coordinate expected");
				break;
			}

			ident = script->getIdentifier();
			if(ident == "refresh"){
				tile->setTileFlag(TILE_FLAG_REFRESH);
			}else if(ident == "nologout"){
				tile->setTileFlag(TILE_FLAG_NOLOGOUT);
			}else if(ident == "protectionzone"){
				tile->setTileFlag(TILE_FLAG_PROTECTIONZONE);
			}else if(ident == "content"){
				script->readSymbol('=');

				// IMPORTANT(fusion): We want to preserve the exact same order specified on
				// disk. Trying to cull multiple BANK/BOTTOM/TOP items, or fix item ordering
				// here is a mistake and could cause de-sync issues, specially with baseline
				// tiles, which don't get flagged as dirty at this stage.
				tile->setItems(LoadObjects(script));
			}else{
				script->error("unknown map flag");
				break;
			}
		}else{
			script->error("next map point expected");
			break;
		}
	}
}

bool Map::loadSector(SectorType type, const wxFileName &filename){
	int sectorX, sectorY, sectorZ;
	if(sscanf(filename.GetFullName().mb_str(), "%d-%d-%d.sec", &sectorX, &sectorY, &sectorZ) != 3){
		g_editor.Error(wxString() << "Invalid sector name format: " << filename.GetFullName());
		return false;
	}

	if(!SectorValid(sectorX, sectorY, sectorZ)){
		g_editor.Error(wxString() << "Invalid sector coordinates "
				<< sectorX << "-" << sectorY << "-" << sectorZ);
		return false;
	}

	Script script(filename.GetFullPath().mb_str());
	MapSector *sector = getOrCreateSectorAt(sectorX * MAP_SECTOR_SIZE, sectorY * MAP_SECTOR_SIZE, sectorZ);
	loadSector(type, sector, &script);
	if(const char *error = script.getError()){
		g_editor.Error(error, ProblemSource::FromSector(sectorX, sectorY, sectorZ));
		return false;
	}

	return true;
}

bool Map::loadPatch(SectorType type, const wxFileName &filename){
	int patchNumber;
	if(sscanf(filename.GetFullName().mb_str(), "%d.sec", &patchNumber) != 1){
		g_editor.Error(wxString() << "Invalid patch name format: " << filename.GetFullName());
		return false;
	}

	Script script(filename.GetFullPath().mb_str());
	if(strcmp(script.readIdentifier(), "sector") != 0){
		g_editor.Error("Expected patch sector identifier");
		return false;
	}

	int sectorX = script.readNumber();
	script.readSymbol(',');
	int sectorY = script.readNumber();
	script.readSymbol(',');
	int sectorZ = script.readNumber();
	if(!script.eof()){
		MapSector *sector = getOrCreateSectorAt(
				sectorX * MAP_SECTOR_SIZE,
				sectorY * MAP_SECTOR_SIZE,
				sectorZ);
		loadSector(type, sector, &script);
	}

	if(const char *error = script.getError()){
		g_editor.Error(error, ProblemSource::FromSector(sectorX, sectorY, sectorZ));
		return false;
	}

	return true;
}

bool Map::loadSpawns(const wxString &filename){
	// NOTE(fusion): A singular ZERO is used to denote the end of the file.
	Script script(filename.mb_str());
	while(int raceId = script.readNumber()){
		int x = script.readNumber();
		int y = script.readNumber();
		int z = script.readNumber();
		int radius = script.readNumber();
		int amount = script.readNumber();
		int interval = script.readNumber();
		if(Tile *tile = getTile(x, y, z)){
			if(Creature *creature = tile->creature){
				g_editor.Warning(wxString() << "Spawn of " << GetCreatureType(raceId).name
						<< " overriding spawn of " << GetCreatureType(creature->raceId).name,
						ProblemSource::FromPosition(x, y, z));
			}
			tile->placeCreature(raceId, radius, amount, interval);
		}else{
			g_editor.Warning(wxString()
					<< "Spawn of " << GetCreatureType(raceId).name << " is out of bounds",
					ProblemSource::FromPosition(x, y, z));
		}
	}

	if(const char *error = script.getError()){
		g_editor.Error(error);
		return false;
	}

	return true;
}

bool Map::loadHouseAreas(const wxString &filename){
	// TODO
	return false;
}

bool Map::loadHouses(const wxString &filename){
	// TODO
	return false;
}

bool Map::loadMeta(const wxString &filename){
	// TODO
	return false;
}

bool Map::load(const wxString &projectDir)
{
	wxString mapDir = ConcatPath(projectDir, "origmap");
	if(!wxDir::Exists(mapDir)){
		g_editor.Error(wxString() << "Unable to locate map directory " << mapDir);
		return false;
	}

	// NOTE(fusion): Load base map.
	{
		g_editor.SetLoadDone(0, "Discovering sector files...");

		int numSectors = 0;
		wxString filename;
		wxDir dir(mapDir);
		if(dir.GetFirst(&filename, "*.sec", wxDIR_FILES)){
			do{
				numSectors += 1;
			}while(dir.GetNext(&filename));
		}

		int numLoaded = 0;
		if(dir.GetFirst(&filename, "*.sec", wxDIR_FILES)){
			do{
				g_editor.SetLoadDone((numLoaded * 95 / numSectors),
						wxString::Format("Loading sector %s (%d/%d)",
							filename, numLoaded, numSectors));

				wxFileName fn(mapDir, filename);
				if(!loadSector(SECTOR_BASELINE, fn)){
					g_editor.Error(wxString() << "Unable to load sector " << fn.GetFullPath());
				}

				numLoaded += 1;
			}while(dir.GetNext(&filename));
		}
	}

	// NOTE(fusion): Load patches.
	wxString saveDir = ConcatPath(projectDir, "save");
	if(wxDir::Exists(saveDir)){
		g_editor.SetLoadDone(94, "Loading patches...");

		int numLoaded = 0;
		wxString filename;
		wxDir dir(saveDir);

		if(dir.GetFirst(&filename, "*.sec", wxDIR_FILES)){
			do{
				g_editor.SetLoadDone(94, wxString::Format("Loading sector patch %s (%d)", filename, numLoaded));

				wxFileName fn(saveDir, filename);
				if(!loadSector(SECTOR_FULL_PATCH, fn)){
					g_editor.Error(wxString() << "Unable to load full patch " << fn.GetFullPath());
				}

				numLoaded += 1;
			}while(dir.GetNext(&filename));
		}

		if(dir.GetFirst(&filename, "*.pat", wxDIR_FILES)){
			do{
				g_editor.SetLoadDone(95, wxString::Format("Loading sector patch %s (%d)", filename, numLoaded));

				wxFileName fn(saveDir, filename);
				if(!loadPatch(SECTOR_PATCH, fn)){
					g_editor.Error(wxString() << "Unable to load patch " << fn.GetFullPath());
				}

				numLoaded += 1;
			}while(dir.GetNext(&filename));
		}
	}else{
		g_editor.Notice("Unable to locate save directory."
				" Existing map patches will not be loaded.");
	}


	wxString datDir = ConcatPath(projectDir, "dat");
	if(wxDir::Exists(datDir)){
		// NOTE(fusion): Load spawns.
		g_editor.SetLoadDone(96, "Loading spawns...");
		wxString spawnsFile = ConcatPath(datDir, "monster.db");
		if(!wxFileName::Exists(spawnsFile)){
			g_editor.Notice("Unable to locate spawns file");
		}else if(!loadSpawns(spawnsFile)){
			g_editor.Error("Unable to load spawns");
		}

#if TODO
		// TODO(fusion): Load house areas.
		g_editor.SetLoadDone(97, "Loading house areas...");
		wxString houseAreasFile = ConcatPath(datDir, "houseareas.dat");
		if(!wxFileName::Exists(houseAreasFile)){
			g_editor.Notice("Unable to locate house areas file");
		}else if(!loadHouseAreas(houseAreasFile)){
			g_editor.Error("Unable to load house areas");
		}

		// TODO(fusion): Load houses.
		g_editor.SetLoadDone(98, "Loading houses...");
		wxString housesFile = ConcatPath(datDir, "houses.dat");
		if(!wxFileName::Exists(housesFile)){
			g_editor.Notice("Unable to locate houses file");
		}else if(!loadHouseAreas(housesFile)){
			g_editor.Error("Unable to load houses");
		}
#endif

#if TODO
		// TODO(fusion): Load map.dat (find an appropriate name? maybe map meta
		// data, following the convention for sprite meta data). We need to load
		// it completely so we're able to save it with minimal changes.
		g_editor.SetLoadDone(99, "Loading map.dat...");
		wxString metaFile = ConcatPath(datDir, "map.dat");
		if(!wxFileName::Exists(metaFile)){
			g_editor.Notice("Unable to locate map.dat");
		}else if(!loadMeta(metaFile)){
			g_editor.Error("Unable to load map.dat");
		}
#endif
	}else{
		g_editor.Notice("Unable to locate dat directory."
				" Spawns, houses and marks will not be loaded");
	}

	return true;
}

bool Map::saveSector(const wxString &dir, const MapSector *sector){
	ASSERT(sector != NULL);
	int sectorX = sector->tiles[0].pos.x / MAP_SECTOR_SIZE;
	int sectorY = sector->tiles[0].pos.y / MAP_SECTOR_SIZE;
	int sectorZ = sector->tiles[0].pos.z;

	ScriptWriter script;
	wxString filename = wxString::Format("%s/%04d-%04d-%02d.sec", dir, sectorX, sectorY, sectorZ);
	if(!script.begin(filename.mb_str())){
		g_editor.Warning(wxString()
				<< "Failed to open sector script " << filename << " for writing",
				ProblemSource::FromSector(sectorX, sectorY, sectorZ));
		return false;
	}

	script.writeText("# Tibia - graphical Multi-User-Dungeon");
	script.writeLn();
	script.writeText("# Data for sector ");
	script.writeNumber(sectorX);
	script.writeText("/");
	script.writeNumber(sectorY);
	script.writeText("/");
	script.writeNumber(sectorZ);
	script.writeLn();
	script.writeLn();

	for(const Tile &tile: sector->tiles){
		// NOTE(fusion): Avoid storing empty tiles on a sector file. If it's
		// used as a patch, all sector tiles are cleared before applying it so
		// it doesn't make a difference. This is different from a regular patch
		// file (see Map::savePatch).
		if(tile.items == NULL && !tile.getTileFlag(TILE_PERSISTENT_FLAGS)){
			continue;
		}

		int offsetX = tile.pos.x & MAP_SECTOR_MASK;
		int offsetY = tile.pos.y & MAP_SECTOR_MASK;
		SaveTile(&script, offsetX, offsetY, &tile);
	}

	if(!script.end()){
		g_editor.Error(wxString()
				<< "Failed to wrap sector script " << filename
					<< ", it may not have been properly stored.",
				ProblemSource::FromSector(sectorX, sectorY, sectorZ));
		return false;
	}

	return true;
}

bool Map::savePatch(const wxString &dir, const MapSector *sector, int patchNumber){
	ASSERT(sector != NULL);
	int sectorX = sector->tiles[0].pos.x / MAP_SECTOR_SIZE;
	int sectorY = sector->tiles[0].pos.y / MAP_SECTOR_SIZE;
	int sectorZ = sector->tiles[0].pos.z;

	ScriptWriter script;
	wxString filename = wxString::Format("%s/%03d.pat", dir, patchNumber);
	if(!script.begin(filename.mb_str())){
		g_editor.Warning(wxString()
				<< "Failed to open patch script " << filename << " for writing",
				ProblemSource::FromSector(sectorX, sectorY, sectorZ));
		return false;
	}

	script.writeText("# Tibia - graphical Multi-User-Dungeon");
	script.writeLn();
	script.writeText("# Patch File");
	script.writeLn();
	script.writeLn();
	script.writeText("Sector ");
	script.writeNumber(sectorX);
	script.writeText(",");
	script.writeNumber(sectorY);
	script.writeText(",");
	script.writeNumber(sectorZ);
	script.writeLn();
	script.writeLn();

	for(const Tile &tile: sector->tiles){
		if(!tile.getTileFlag(TILE_FLAG_DIRTY)){
			continue;
		}

		// NOTE(fusion): Even if the tile is empty, we still want to store it
		// in a patch file, to make sure the tile gets cleared when the patch
		// is applied.
		int offsetX = tile.pos.x & MAP_SECTOR_MASK;
		int offsetY = tile.pos.y & MAP_SECTOR_MASK;
		SaveTile(&script, offsetX, offsetY, &tile);
	}

	if(!script.end()){
		g_editor.Warning(wxString()
				<< "Failed to wrap patch script " << filename
					<<  ", it may not have been properly stored.",
				ProblemSource::FromSector(sectorX, sectorY, sectorZ));
		return false;
	}

	return true;
}

bool Map::saveSpawns(const wxString &filename){
	if(filename.IsEmpty()){
		return false;
	}

	struct Spawn{
		int x, y, z;
		int raceId;
		int radius;
		int amount;
		int regen;
	};

	std::vector<Spawn> spawns;

	{
		// TODO(fusion): I wanted to have creatures/spawns separate from the tile,
		// in such a way that this gather step wouldn't be necessary.
		for(const auto &[sectorId, sector]: sectors){
			for(const Tile &tile: sector.tiles){
				Creature *creature = tile.creature;
				if(!creature){
					continue;
				}

				Spawn spawn = {};
				spawn.x = tile.pos.x;
				spawn.y = tile.pos.y;
				spawn.z = tile.pos.z;
				spawn.raceId = creature->raceId;
				spawn.radius = creature->spawnRadius;
				spawn.amount = creature->spawnAmount;
				spawn.regen = creature->spawnInterval;
				spawns.push_back(spawn);
			}
		}

		std::sort(spawns.begin(), spawns.end(),
			[](const Spawn &a, const Spawn &b){
				int aSectorX = a.x / MAP_SECTOR_SIZE;
				int aOffsetX = a.x % MAP_SECTOR_SIZE;
				int bSectorX = b.x / MAP_SECTOR_SIZE;
				int bOffsetX = b.x % MAP_SECTOR_SIZE;
				if(aSectorX != bSectorX){
					return aSectorX < bSectorX;
				}

				int aSectorY = a.y / MAP_SECTOR_SIZE;
				int aOffsetY = a.y % MAP_SECTOR_SIZE;
				int bSectorY = b.y / MAP_SECTOR_SIZE;
				int bOffsetY = b.y % MAP_SECTOR_SIZE;
				if(aSectorY != bSectorY){
					return aSectorY < bSectorY;
				}

				int aSectorZ = a.z;
				int bSectorZ = b.z;
				if(aSectorZ != bSectorZ){
					return aSectorZ < bSectorZ;
				}

				if(a.raceId != b.raceId){
					return a.raceId < b.raceId;
				}

				// TODO(fusion): Some groups follow this ascending, descending,
				// and others don't seem to follow this at all.
				int aOffset = GetTileIndex(aOffsetX, aOffsetY);
				int bOffset = GetTileIndex(bOffsetX, bOffsetY);
				return aOffset < bOffset;
			});
	}

	ScriptWriter script;
	if(!script.begin(filename.mb_str())){
		g_editor.Warning(wxString() << "Failed to open spawn file " << filename << " for writing");
		return false;
	}

	script.writeText("# Tibia - graphical Multi-User-Dungeon");
	script.writeLn();
	script.writeText("# MonsterHomes File");
	script.writeLn();
	script.writeLn();
	script.writeText("# Race     X     Y  Z Radius Amount Regen.");
	script.writeLn();
	script.writeLn();

	char line[256] = {};
	uint32_t currentSectorId = 0;
	for(const Spawn &spawn: spawns){
		uint32_t sectorId = GetMapSectorId(spawn.x, spawn.y, spawn.z);
		if(sectorId != currentSectorId){
			snprintf(line, sizeof(line),
					"# ====== %04d,%04d,%02d ====================",
					(spawn.x / MAP_SECTOR_SIZE),
					(spawn.y / MAP_SECTOR_SIZE),
					spawn.z);
			script.writeText(line);
			script.writeLn();
			currentSectorId = sectorId;
		}

		snprintf(line, sizeof(line), "%6d %5d %5d %2d %6d %6d %6d",
				spawn.raceId, spawn.x, spawn.y, spawn.z,
				spawn.radius, spawn.amount, spawn.regen);
		script.writeText(line);
		script.writeLn();
	}

	script.writeText("0 # zero for end of file");
	script.writeLn();

	if(!script.end()){
		g_editor.Warning(wxString()
				<< "Failed to wrap spawn file " << filename
					<<  ", it may not have been properly stored.");
		return false;
	}

	return true;
}

bool Map::saveHouseAreas(const wxString &filename){
	// TODO(fusion): Save house areas.
	return false;
}

bool Map::saveHouses(const wxString &filename){
	// TODO(fusion): Save houses.
	return false;
}

bool Map::saveMeta(const wxString &filename){
	// TODO(fusion): Save map.dat + mem.dat ?
	return false;
}

bool Map::save(const wxString &projectDir)
{
	if(projectDir.IsEmpty()){
		return false;
	}

	wxString datDir = ConcatPath(projectDir, "dat");
	if(!wxDir::Exists(datDir) && !wxDir::Make(datDir, 0755, wxPATH_MKDIR_FULL)){
		g_editor.Error("Unable to create dat directory");
		return false;
	}

	wxString saveDir = ConcatPath(projectDir, "save");
	if(!wxDir::Exists(datDir) && !wxDir::Make(datDir, 0755, wxPATH_MKDIR_FULL)){
		g_editor.Error("Unable to create save directory");
		return false;
	}

	wxString spawnsFile = ConcatPath(datDir, "monster.db");
	//wxString houseAreasFile = ConcatPath(datDir, "houseareas.dat");
	//wxString housesFile = ConcatPath(datDir, "houses.dat");
	//wxString metaFile = ConcatPath(datDir, "map.dat");

	// TODO(fusion): `backupPatch` will also remove stale files. We probably always
	// want to do some kind of backup here, but we should have a setting to control
	// whether we should write to the same backup file, or to have one for each time
	// we save.
	if(true){ // g_settings.getBoolean(Config::BACKUP_ON_SAVE)
		wxString backupDir = ConcatPath(projectDir, "backup");
		wxFileName backupName(backupDir, BackupName("save"));
		if(!backupPatch(projectDir, backupName, true)){
			return false;
		}
	}

	int numSaved = 0;
	int numSectors = (int)sectors.size();
	int nextPatchNumber = 0;
	int fullPatchThreshold = (MAP_SECTOR_SIZE * MAP_SECTOR_SIZE) / 2;
	for(const auto &[sectorId, sector]: sectors){
		g_editor.SetLoadDone((numSaved * 100) / numSectors,
				wxString() << "Saving sector "
					<< (sector.tiles[0].pos.x / MAP_SECTOR_SIZE) << "-"
					<< (sector.tiles[0].pos.y / MAP_SECTOR_SIZE) << "-"
					<< (sector.tiles[0].pos.z) << " ...");

		int numDirty = 0;
		for(const Tile &tile: sector.tiles){
			if(tile.getTileFlag(TILE_FLAG_DIRTY)){
				numDirty += 1;
			}
		}

		// TODO(fusion): I have no idea whether this is a good heuristic,
		// or if there something else when determining a full patch. There
		// is also the issue with tiles being dirty if we modify respawns
		// so we might want to do a different approach like only tagging
		// sectors as dirty and checking the differences with the original
		// sector.
		if(numDirty >= fullPatchThreshold){
			saveSector(saveDir, &sector);
		}else if(numDirty > 0){
			savePatch(saveDir, &sector, nextPatchNumber);
			nextPatchNumber += 1;
		}

		numSaved += 1;
	}

	saveSpawns(spawnsFile);
	//saveHouseAreas(houseAreasFile);
	//saveHouses(housesFile);
	//saveMeta(metaFile);

	return true;
}

bool Map::backupPatch(const wxString &projectDir, const wxFileName &outputName, bool del /*= false*/){
	wxString datDir = ConcatPath(projectDir, "dat");
	wxString saveDir = ConcatPath(projectDir, "save");
	wxString spawnsFile = ConcatPath(datDir, "monster.db");
	//wxString houseAreasFile = ConcatPath(datDir, "houseareas.dat");
	//wxString housesFile = ConcatPath(datDir, "houses.dat");
	//wxString metaFile = ConcatPath(datDir, "map.dat");
	while(true){
		wxArrayString filenames;
		filenames.Add(spawnsFile);
		//filenames.Add(houseAreasFile);
		//filenames.Add(housesFile);
		//filenames.Add(metaFile);
		wxDir::GetAllFiles(saveDir, &filenames, "*.sec", wxDIR_FILES | wxDIR_HIDDEN);
		wxDir::GetAllFiles(saveDir, &filenames, "*.pat", wxDIR_FILES | wxDIR_HIDDEN);
		if(BackupFiles(outputName, projectDir, filenames)){
			if(del){
				RemoveFiles(filenames);
			}
			break;
		}

		int ret = g_editor.PopupDialog("Patch backup error",
				"There was an error while backing up existing patch data, do you want to retry?",
				wxYES | wxNO | wxCANCEL);
		if(ret == wxID_CANCEL) return false;
		if(ret == wxID_NO)     break;
	}

	return true;
}

bool Map::backupMap(const wxString &projectDir, const wxFileName &outputName, bool del /*= false*/){
	wxString mapDir = ConcatPath(projectDir, "origmap");
	while(true){
		wxArrayString filenames;
		wxDir::GetAllFiles(mapDir, &filenames, "*.sec", wxDIR_FILES | wxDIR_HIDDEN);
		if(BackupFiles(outputName, projectDir, filenames)){
			if(del){
				RemoveFiles(filenames);
			}
			break;
		}

		int ret = g_editor.PopupDialog("Map backup error",
				"There was an error while backing up existing map data, do you want to retry?",
				wxYES | wxNO | wxCANCEL);
		if(ret == wxID_CANCEL) return false;
		if(ret == wxID_NO)     break;
	}

	return true;
}

bool Map::exportPatch(const wxString &projectDir,
		const wxString &filename, bool commit /*= false*/){
	// IMPORTANT(fusion): We want to have the latest version of the map on disk
	// before trying to export it.
	if(!save(projectDir)){
		g_editor.Error("Unable to save project while exporting patch");
		return false;
	}

	if(!backupPatch(projectDir, filename, false)){
		g_editor.Error("Unable to export patch");
		return false;
	}

	if(commit){
		// TODO(fusion): Similar to note in `Map::save`.
		if(true){ // g_settings.getBoolean(Config::BACKUP_ON_COMMIT)
			wxString backupDir = ConcatPath(projectDir, "backup");
			wxFileName backupName(backupDir, BackupName("origmap"));
			backupMap(projectDir, backupName, true);
		}

		// NOTE(fusion): Save patched map.
		int numSaved = 0;
		int numSectors = (int)sectors.size();
		wxString mapDir = ConcatPath(projectDir, "origmap");
		for(auto &[sectorId, sector]: sectors){
			bool empty = true;
			for(Tile &tile: sector.tiles){
				if(tile.items != NULL) empty = false;
				tile.clearTileFlag(TILE_FLAG_DIRTY);
			}

			g_editor.SetLoadDone((numSaved * 100) / numSectors,
					wxString() << "Saving sector "
						<< (sector.tiles[0].pos.x / MAP_SECTOR_SIZE) << "-"
						<< (sector.tiles[0].pos.y / MAP_SECTOR_SIZE) << "-"
						<< (sector.tiles[0].pos.z) << " ...");

			if(!empty){
				saveSector(mapDir, &sector);
			}

			numSaved += 1;
		}

		// NOTE(fusion): We cleared the DIRTY flag from all current tiles in the
		// loop above. That means that any previous tile states should be flagged
		// as DIRTY, otherwise there could inconsistencies when undoing previous
		// actions.
		//  This is different from only saving a patch, because in that case we
		// need to track changes relative to ORIGMAP, whereas in this case, we
		// have just BECOME ORIGMAP.
		g_editor.actionQueue.setDirty();

		// NOTE(fusion): We don't ask `backupPatch` to remove files here because
		// it would also remove non-patch files such as spawns, houses, etc...
		wxString saveDir = ConcatPath(projectDir, "save");
		{
			wxArrayString filenames;
			wxDir::GetAllFiles(saveDir, &filenames, "*.sec", wxDIR_FILES | wxDIR_HIDDEN);
			wxDir::GetAllFiles(saveDir, &filenames, "*.pat", wxDIR_FILES | wxDIR_HIDDEN);
			RemoveFiles(filenames);
		}
	}

	return true;
}

void Map::clear(void)
{
	minSectorX = 0;
	minSectorY = 0;
	minSectorZ = 0;
	maxSectorX = 0;
	maxSectorY = 0;
	maxSectorZ = 0;
	sectors.clear();
}

MapSector *Map::getSectorAt(int x, int y, int z){
	if(!PositionValid(x, y, z)){
		return NULL;
	}

	uint32_t sectorId = GetMapSectorId(x, y, z);
	auto it = sectors.find(sectorId);
	if(it != sectors.end()){
		return &it->second;
	}else{
		return NULL;
	}
}

MapSector *Map::getOrCreateSectorAt(int x, int y, int z){
	if(!PositionValid(x, y, z)){
		// NOTE(fusion): This is just to make sure we don't return NULL.
		static MapSector outOfBounder;
		return &outOfBounder;
	}

	uint32_t sectorId = GetMapSectorId(x, y, z);
	auto ret = sectors.try_emplace(sectorId);
	if(ret.second){
		int sectorX = x / MAP_SECTOR_SIZE;
		int sectorY = y / MAP_SECTOR_SIZE;
		int sectorZ = z;

		if(sectors.size() > 1){
			if(sectorX < minSectorX){
				minSectorX = sectorX;
			}else if(sectorX > maxSectorX){
				maxSectorX = sectorX;
			}

			if(sectorY < minSectorY){
				minSectorY = sectorY;
			}else if(sectorY > maxSectorY){
				maxSectorY = sectorY;
			}

			if(sectorZ < minSectorZ){
				minSectorZ = sectorZ;
			}else if(sectorZ > maxSectorZ){
				maxSectorZ = sectorZ;
			}
		}else{
			minSectorX = sectorX;
			minSectorY = sectorY;
			minSectorZ = sectorZ;
			maxSectorX = sectorX;
			maxSectorY = sectorY;
			maxSectorZ = sectorZ;
		}

		ret.first->second.setTilePositions(
				sectorX * MAP_SECTOR_SIZE,
				sectorY * MAP_SECTOR_SIZE,
				sectorZ);
	}

	return &ret.first->second;
}

Tile *Map::getTile(int x, int y, int z)
{
	if(MapSector *sector = getSectorAt(x, y, z)){
		int offsetX = x & MAP_SECTOR_MASK;
		int offsetY = y & MAP_SECTOR_MASK;
		return sector->getTile(offsetX, offsetY);
	}else{
		return NULL;
	}
}

Tile *Map::getOrCreateTile(int x, int y, int z)
{
	MapSector *sector = getOrCreateSectorAt(x, y, z);
	ASSERT(sector != NULL);
	int offsetX = x & MAP_SECTOR_MASK;
	int offsetY = y & MAP_SECTOR_MASK;
	return sector->getTile(offsetX, offsetY);
}

void Map::checkTiles(bool showDialog /*= false*/)
{
	if(showDialog){
		g_editor.CreateLoadBar("Checking tiles...");
	}

	forEachTile(
		[](Tile *tile, double progress){
			int numBank   = 0;
			int numBottom = 0;
			int numTop    = 0;
			bool ordered  = true;
			int prevStackPriority = -1;
			for(Item *item = tile->items; item != NULL; item = item->next){
				int stackPriority = item->getStackPriority();
				if(stackPriority == STACK_PRIORITY_BANK)   numBank += 1;
				if(stackPriority == STACK_PRIORITY_BOTTOM) numBottom += 1;
				if(stackPriority == STACK_PRIORITY_TOP)    numTop += 1;
				if(stackPriority < prevStackPriority)      ordered = false;
				prevStackPriority = stackPriority;
			}

			if(numBank > 1 || numBottom > 1 || numTop > 1){
				wxString warning;
				warning << "Multiple exclusive items detected on the same tile: ";
				if(numBank   > 1) warning << " " << numBank   << " BANK";
				if(numBottom > 1) warning << " " << numBottom << " BOTTOM";
				if(numTop    > 1) warning << " " << numTop    << " TOP";
				g_editor.Warning(warning, ProblemSource::FromPosition(tile->pos));
			}

			// TODO(fusion): Do we actually want to automatically sort items here?
			if(!ordered){
				tile->sortItems();
				tile->setTileFlag(TILE_FLAG_DIRTY);
				g_editor.Notice(wxString() << "Fixed issues with item ordering",
						ProblemSource::FromPosition(tile->pos));
			}

			g_editor.SetLoadDone((int)(progress * 100.0));
		});

	if(showDialog){
		g_editor.DestroyLoadBar();
	}
}

void Map::cleanInvalidTiles(bool showDialog /*= false*/)
{
	if(showDialog){
		g_editor.CreateLoadBar("Removing invalid tiles...");
	}

	removeItems(
		[](const Item *item, double progress){
			g_editor.SetLoadDone((int)(progress * 100.0));
			return !ItemTypeExists(item->getID());
		});

	if(showDialog){
		g_editor.DestroyLoadBar();
	}
}

bool Map::exportMinimap(const wxFileName &filename,
						int floor /*= rme::MapGroundLayer*/,
						bool showDialog /*= false*/)
{
	if(isEmpty()){
		return true;
	}

	FileWriteHandle fh(nstr(filename.GetFullPath()));
	if(!fh.isOpen()) {
		return false;
	}

	if(showDialog){
		g_editor.CreateLoadBar("Exporting minimap...");
	}

	Position min = getMinPosition();
	int width = getWidth();
	int height = getHeight();
	auto bitmap = std::make_unique<uint8_t[]>(width * height);
	memset(bitmap.get(), 0, width * height);

	{
		double nextUpdate = 0.0;
		forEachTile(
			[&nextUpdate, floor, showDialog, min, width, height, &bitmap](const Tile *tile, double progress){
				if(showDialog && progress >= nextUpdate){
					g_editor.SetLoadDone((int)(progress * 90));
					nextUpdate = progress + 0.01;
				}

				if(tile->empty() || tile->pos.z != floor){
					return;
				}

				int relX = tile->pos.x - min.x;
				int relY = tile->pos.y - min.y;
				int index = (height - relY - 1) * width + relX;
				bitmap[index] = tile->getMiniMapColor();
			});
	}


	int pitch = ((width + 3) / 4) * 4;
	int padding = width - pitch;
	int headerSize = (14 + 40 + 4 * 256);
	int fileSize = headerSize + pitch * height;

	// BMP header
	fh.addRAW("BM");			// identifier
	fh.addU32(fileSize);		// file size
	fh.addU16(0);				// reserved
	fh.addU16(0);				// reserved
	fh.addU32(headerSize);		// bitmap data offset

	// DIB header
	fh.addU32(40);				// DIB header size
	fh.addU32(width);			// width
	fh.addU32(height);			// height
	fh.addU16(1);				// color planes
	fh.addU16(8);				// bits per pixel
	fh.addU32(0);				// compression type
	fh.addU32(0);				// size of the raw bitmap data, can be zero when not compressed
	fh.addU32(4000);			// horizontal resolution in pixels per meter
	fh.addU32(4000);			// vertical resolution in pixels per meter
	fh.addU32(256);				// colors in the color palette
	fh.addU32(0);				// important colors, generally ignored

	// Palette
	for(int i = 0; i < 256; i += 1)
		fh.addU32(colorFromEightBit(i).GetRGB());

	// Bitmap
	{
		double nextUpdate = 0.0;
		for(int y = 0; y < height; y += 1){
			fh.addRAW(&bitmap[y*width], width);
			for(int i = 0; i < padding; i += 1){
				fh.addU8(0);
			}

			double progress = (double)y / (double)height;
			if(showDialog && nextUpdate >= progress){
				g_editor.SetLoadDone(90 + (int)(progress * 10.0));
				nextUpdate = progress + 0.01;
			}
		}
	}

	if(showDialog){
		g_editor.DestroyLoadBar();
	}

	fh.close();
	return true;
}

