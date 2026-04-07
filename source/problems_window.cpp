#include "main.h"
#include "problems_window.h"
#include "editor.h"

ProblemsWindow::ProblemsWindow(wxWindow *parent) :
	wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_VIRTUAL),
	updateTimer(this)
{
	// TODO(fusion): Maybe there is a better way to fill the last column without
	// manually doing it in a wxEVT_SIZE event, or setting a large enough value
	// here?
	InsertColumn(0, "Severity", wxLIST_FORMAT_LEFT, 80);
	InsertColumn(1, "Source",   wxLIST_FORMAT_LEFT, 150);
	InsertColumn(2, "Message",  wxLIST_FORMAT_LEFT, 1000);

	Bind(wxEVT_LIST_ITEM_SELECTED, &ProblemsWindow::OnItemSelected, this);
	Bind(wxEVT_TIMER, &ProblemsWindow::OnTimer, this);
}

ProblemsWindow::~ProblemsWindow(void){
	// no-op
}

static wxString GetSeverityString(ProblemSeverity severity){
	wxString result = wxEmptyString;
	switch(severity){
		case PROBLEM_SEVERITY_NOTICE:  result = "Notice"; break;
		case PROBLEM_SEVERITY_WARNING: result = "Warning"; break;
		case PROBLEM_SEVERITY_ERROR:   result = "Error"; break;
	}
	return result;
}

static wxString GetSourceString(const ProblemSource &source){
	wxString result = wxEmptyString;
	switch(source.type){
		case PROBLEM_SOURCE_OBJECT_TYPE:	result << GetItemType(source.typeId).name; break;
		case PROBLEM_SOURCE_MONSTER_TYPE:	result << GetCreatureType(source.raceId).name; break;
		case PROBLEM_SOURCE_POSITION:		result << "(" << source.position.x << "," << source.position.y
													<< "," << source.position.z << ")"; break;
		default:							break;
	}
	return result;
}

wxString ProblemsWindow::OnGetItemText(long item, long column) const {
	wxString result = wxEmptyString;
	if(item >= 0 && item < (long)problems.size()){
		switch(column){
			case 0: result = GetSeverityString(problems[item].severity); break;
			case 1: result = GetSourceString(problems[item].source); break;
			case 2: result = problems[item].message; break;
		}
	}
	return result;
}

void ProblemsWindow::OnItemSelected(wxListEvent &event){
	long item = event.GetIndex();
	if(item >= 0 && item < (long)problems.size()){
		const ProblemSource &source = problems[item].source;
		if(source.type == PROBLEM_SOURCE_POSITION){
			g_editor.SetScreenCenterPosition(source.position);
		}
	}
}

void ProblemsWindow::OnTimer(wxTimerEvent &event){
	(void)event;

	// IMPORTANT(fusion): `SetItemCount` can be expensive on Windows, specially
	// when loading a map with lots of problems. Moving it to a delayed callback
	// should help improve load times then.
	SetItemCount((long)problems.size());
}

void ProblemsWindow::Insert(ProblemSeverity severity, ProblemSource source, wxString message){
	problems.emplace_back(severity, source, std::move(message));

	if(!updateTimer.IsRunning()){
		updateTimer.Start(500, wxTIMER_ONE_SHOT);
	}
}

void ProblemsWindow::Clear(void){
	problems.clear();

	if(!updateTimer.IsRunning()){
		updateTimer.Start(500, wxTIMER_ONE_SHOT);
	}
}

