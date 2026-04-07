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

#include "common.h"
#include "math.h"

#include <sstream>
#include <random>

// random generator
std::mt19937& getRandomGenerator()
{
	static std::random_device rd;
	static std::mt19937 generator(rd());
	return generator;
}

int32_t uniform_random(int32_t minNumber, int32_t maxNumber)
{
	static std::uniform_int_distribution<int32_t> uniformRand;
	if(minNumber == maxNumber) {
		return minNumber;
	} else if(minNumber > maxNumber) {
		std::swap(minNumber, maxNumber);
	}
	return uniformRand(getRandomGenerator(), std::uniform_int_distribution<int32_t>::param_type(minNumber, maxNumber));
}

int32_t uniform_random(int32_t maxNumber)
{
	return uniform_random(0, maxNumber);
}

//
std::string i2s(const int _i)
{
	static std::stringstream ss;
	ss.str("");
	ss << _i;
	return ss.str();
}

std::string f2s(const double _d)
{
	static std::stringstream ss;
	ss.str("");
	ss << _d;
	return ss.str();
}

int s2i(const std::string s)
{
	return atoi(s.c_str());
}

double s2f(const std::string s)
{
	return atof(s.c_str());
}

wxString i2ws(const int _i)
{
	wxString str;
	str << _i;
	return str;
}

wxString f2ws(const double _d)
{
	wxString str;
	str << _d;
	return str;
}

int ws2i(const wxString s)
{
	long _i;
	if(s.ToLong(&_i))
		return int(_i);
	return 0;
}

double ws2f(const wxString s)
{
	double _d;
	if(s.ToDouble(&_d))
		return _d;
	return 0.0;
}

void replaceString(std::string& str, const std::string sought, const std::string replacement)
{
	size_t pos = 0;
	size_t start = 0;
	size_t soughtLen = sought.length();
	size_t replaceLen = replacement.length();
	while((pos = str.find(sought, start)) != std::string::npos) {
		str = str.substr(0, pos) + replacement + str.substr(pos + soughtLen);
		start = pos + replaceLen;
	}
}

void trim(std::string& str) {
	// Trim from start
	str.erase(str.begin(), std::find_if(str.begin(), str.end(),
		[](int ch) { return !std::isspace(ch); }));

	// Trim from end
	str.erase(std::find_if(str.rbegin(), str.rend(),
		[](int ch) { return !std::isspace(ch); }).base(), str.end());
}

void trim_right(std::string& source, const std::string& t)
{
	source.erase(source.find_last_not_of(t)+1);
}

void trim_left(std::string& source, const std::string& t)
{
	source.erase(0, source.find_first_not_of(t));
}

void to_lower_str(std::string& source)
{
	std::transform(source.begin(), source.end(), source.begin(), tolower);
}

void to_upper_str(std::string& source)
{
	std::transform(source.begin(), source.end(), source.begin(), toupper);
}

std::string as_lower_str(const std::string& other)
{
	std::string ret = other;
	to_lower_str(ret);
	return ret;
}

std::string as_upper_str(const std::string& other)
{
	std::string ret = other;
	to_upper_str(ret);
	return ret;
}

int strcmp_ci(std::string_view a, std::string_view b){
	if(a.size() != b.size()){
		return (int)a.size() - (int)b.size();
	}

	for(size_t i = 0; i < a.size(); i += 1){
		int ca = tolower((unsigned char)a[i]);
		int cb = tolower((unsigned char)b[i]);
		if(ca != cb){
			return ca - cb;
		}
	}

	return 0;
}

bool isFalseString(std::string& str)
{
	if(str == "false" || str == "0" || str == "" || str == "no" || str == "not") {
		return true;
	}
	return false;
}

bool isTrueString(std::string& str)
{
	return !isFalseString(str);
}

int random(int low, int high)
{
	if(low == high) {
		return low;
	}

	if(low > high) {
		return low;
	}

	int range = high - low;

	double dist = double(mt_randi()) / 0xFFFFFFFF;
	return low + std::min(range, int((1 + range) * dist));
}

int random(int high)
{
	return random(0,high);
}

std::wstring string2wstring(const std::string& utf8string)
{
	wxString s(utf8string.c_str(), wxConvUTF8);
	return std::wstring((const wchar_t*)s.c_str());
}

std::string wstring2string(const std::wstring& widestring)
{
	wxString s(widestring.c_str());
	return std::string((const char*)s.mb_str(wxConvUTF8));
}

bool posFromClipboard(int &x, int &y, int &z)
{
	bool result = false;
	if(wxTheClipboard->Open()) {
		if(wxTheClipboard->IsSupported(wxDF_TEXT)) {
			wxTextDataObject data;
			wxTheClipboard->GetData(data);
			wxString text = data.GetText();

			// NOTE(fusion): Just grab the first three numbers we can find.
			int count = 0;
			int numbers[3] = {};
			bool wordStart = true;
			for(size_t i = 0; i < text.Length() && count < NARRAY(numbers); i += 1){
				if(!isalpha(text[i]) && !isdigit(text[i])){
					wordStart = true;
				}else if(wordStart){
					size_t j = i;
					int number = 0;
					while(j < text.Length() && isdigit(text[j])){
						number = number * 10 + (text[j] - '0');
						j += 1;
					}

					if(j > i){
						numbers[count++] = number;
					}

					wordStart = false;
				}
			}

			if(count == 3){
				x = numbers[0];
				y = numbers[1];
				z = numbers[2];
				result = true;
			}
		}
		wxTheClipboard->Close();
	}
	return result;
}

bool posToClipboard(int x, int y, int z, int format)
{
	bool result = false;
	if(wxTheClipboard->Open()){
		wxTextDataObject* data = NULL;
		switch (format) {
			case 0: data = newd wxTextDataObject(wxString::Format("[%d,%d,%d]",                     x, y, z)); break;
			case 1: data = newd wxTextDataObject(wxString::Format("{x = %d, y = %d, z = %d}",       x, y, z)); break;
			case 2: data = newd wxTextDataObject(wxString::Format("{\"x\":%d, \"y\":%d, \"z\":%d}", x, y, z)); break;
			case 3: data = newd wxTextDataObject(wxString::Format("%d, %d, %d",                     x, y, z)); break;
			case 4: data = newd wxTextDataObject(wxString::Format("(%d, %d, %d)",                   x, y, z)); break;
			case 5: data = newd wxTextDataObject(wxString::Format("Position(%d, %d, %d)",           x, y, z)); break;
			default: break;
		}

		if(data != NULL){
			result = wxTheClipboard->SetData(data);
		}

		wxTheClipboard->Close();
	}

	return result;
}

bool posToClipboard(int fromx, int fromy, int fromz, int tox, int toy, int toz)
{
	if(!wxTheClipboard->Open())
		return false;

	std::ostringstream clip;
	clip << "{";
	clip << "fromx = " << fromx << ", ";
	clip << "tox = " << tox << ", ";
	clip << "fromy = " << fromy << ", ";
	clip << "toy = " << toy << ", ";
	if(fromz != toz) {
		clip << "fromz = " << fromz << ", ";
		clip << "toz = " << toz;
	}
	else
		clip << "z = " << fromz;
	clip << "}";

	wxTheClipboard->SetData(new wxTextDataObject(clip.str()));
	wxTheClipboard->Close();
	return true;
}

wxString b2yn(bool value)
{
	return value ? "Yes" : "No";
}

wxColor colorFromEightBit(int color)
{
	if(color <= 0 || color >= 216)
		return wxColor(0, 0, 0);
	const uint8_t red = (uint8_t)(int(color / 36) % 6 * 51);
	const uint8_t green = (uint8_t)(int(color / 6) % 6 * 51);
	const uint8_t blue = (uint8_t)(color % 6 * 51);
	return wxColor(red, green, blue);
}

wxString GetExecDirectory(void)
{
	wxFileName fn(wxStandardPaths::Get().GetExecutablePath());
	return fn.GetPath();
}

wxString NormalizeDir(const wxString &dir){
	wxFileName fn(dir, "");
	fn.Normalize(wxPATH_NORM_ENV_VARS
				| wxPATH_NORM_DOTS
				| wxPATH_NORM_TILDE
				| wxPATH_NORM_ABSOLUTE
				| wxPATH_NORM_LONG);
	return fn.GetPath();
}

wxString ConcatPath(const wxString &a, const wxString &b){
	wxFileName fn(a, b);
	return fn.GetFullPath();
}

wxString ConcatPath(const wxString &a, const wxString &b, const wxString &c){
	wxFileName fn(a, c);
	fn.AppendDir(b);
	return fn.GetFullPath();
}

void SetWindowToolTip(wxWindow *a, const wxString &tooltip)
{
	a->SetToolTip(tooltip);
}

void SetWindowToolTip(wxWindow *a, wxWindow *b, const wxString &tooltip)
{
	a->SetToolTip(tooltip);
	b->SetToolTip(tooltip);
}

