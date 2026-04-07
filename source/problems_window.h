#ifndef RME_PROBLEMS_WINDOW_H_
#define RME_PROBLEMS_WINDOW_H_ 1

#include "map.h"
#include "position.h"
#include <wx/listctrl.h>

enum ProblemSeverity {
	PROBLEM_SEVERITY_NOTICE  = 0,
	PROBLEM_SEVERITY_WARNING = 1,
	PROBLEM_SEVERITY_ERROR   = 2,
};

enum ProblemSourceType {
	PROBLEM_SOURCE_NONE         = 0,
	PROBLEM_SOURCE_OBJECT_TYPE  = 1,
	PROBLEM_SOURCE_MONSTER_TYPE = 2,
	PROBLEM_SOURCE_POSITION     = 3,
};

struct ProblemSource {
	ProblemSourceType type = PROBLEM_SOURCE_NONE;
	union{
		int typeId;
		int raceId;
		Position position;
	};

	static ProblemSource FromObjectType(int typeId){
		ProblemSource source = {};
		source.type = PROBLEM_SOURCE_OBJECT_TYPE;
		source.typeId = typeId;
		return source;
	}

	static ProblemSource FromMonsterType(int raceId){
		ProblemSource source = {};
		source.type = PROBLEM_SOURCE_MONSTER_TYPE;
		source.raceId = raceId;
		return source;
	}

	static ProblemSource FromPosition(Position position){
		ProblemSource source = {};
		source.type = PROBLEM_SOURCE_POSITION;
		source.position = position;
		return source;
	}

	static ProblemSource FromPosition(int x, int y, int z){
		ProblemSource source = {};
		source.type = PROBLEM_SOURCE_POSITION;
		source.position.x = x;
		source.position.y = y;
		source.position.z = z;
		return source;
	}

	static ProblemSource FromSector(int sectorX, int sectorY, int sectorZ){
		ProblemSource source = {};
		source.type = PROBLEM_SOURCE_POSITION;
		source.position.x = sectorX * MAP_SECTOR_SIZE + MAP_SECTOR_SIZE / 2;
		source.position.y = sectorY * MAP_SECTOR_SIZE + MAP_SECTOR_SIZE / 2;
		source.position.z = sectorZ;
		return source;
	}
};

struct Problem {
	ProblemSeverity severity;
	ProblemSource source;
	wxString message;
};

class ProblemsWindow: public wxListCtrl {
public:
	ProblemsWindow(wxWindow *parent);
	~ProblemsWindow(void) override;
	wxString OnGetItemText(long item, long column) const override;
	void OnItemSelected(wxListEvent &event);
	void OnTimer(wxTimerEvent &event);
	void Insert(ProblemSeverity severity, ProblemSource source, wxString message);
	void Clear(void);

private:
	wxTimer updateTimer = {};
	std::vector<Problem> problems = {};
};

#endif //RME_PROBLEMS_WINDOW_H_
