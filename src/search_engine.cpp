/***************************************************************************
 *   Copyright (C) 2008-2009 by Andrzej Rybczak                            *
 *   electricityispower@gmail.com                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include "display.h"
#include "global.h"
#include "helpers.h"
#include "playlist.h"
#include "search_engine.h"
#include "settings.h"
#include "status.h"

using namespace MPD;
using namespace Global;

SearchEngine *mySearcher = new SearchEngine;

const char *SearchEngine::NormalMode = "Match if tag contains searched phrase (regexes supported)";
const char *SearchEngine::StrictMode = "Match only if both values are the same";

size_t SearchEngine::StaticOptions = 20;
size_t SearchEngine::SearchButton = 15;
size_t SearchEngine::ResetButton = 16;

bool SearchEngine::MatchToPattern = 1;
int SearchEngine::CaseSensitive = REG_ICASE;

void SearchEngine::Init()
{
	w = new Menu< std::pair<Buffer *, Song *> >(0, MainStartY, COLS, MainHeight, "", Config.main_color, brNone);
	w->HighlightColor(Config.main_highlight_color);
	w->SetTimeout(ncmpcpp_window_timeout);
	w->CyclicScrolling(Config.use_cyclic_scrolling);
	w->SetItemDisplayer(Display::SearchEngine);
	w->SetSelectPrefix(&Config.selected_item_prefix);
	w->SetSelectSuffix(&Config.selected_item_suffix);
	w->SetGetStringFunction(SearchEngineOptionToString);
	isInitialized = 1;
}

void SearchEngine::Resize()
{
	w->Resize(COLS, MainHeight);
	hasToBeResized = 0;
}

void SearchEngine::SwitchTo()
{
	if (myScreen == this)
		return;
	
	if (!isInitialized)
		Init();
	
	if (hasToBeResized)
		Resize();
	
	if (w->Empty())
		Prepare();
	myScreen = this;
	RedrawHeader = 1;
	
	if (!w->Back().first)
	{
		*w << XY(0, 0) << "Updating list...";
		UpdateFoundList();
	}
}

std::string SearchEngine::Title()
{
	return "Search engine";
}

void SearchEngine::EnterPressed()
{
	size_t option = w->Choice();
	LockStatusbar();
	
	if (option < SearchButton)
		w->Current().first->Clear();
	
	switch (option)
	{
		case 0:
		{
			Statusbar() << fmtBold << "Any: " << fmtBoldEnd;
			itsPattern.Any(wFooter->GetString(itsPattern.Any()));
			*w->Current().first << fmtBold << "Any:      " << fmtBoldEnd << ' ' << ShowTag(itsPattern.Any());
			break;
		}
		case 1:
		{
			Statusbar() << fmtBold << "Artist: " << fmtBoldEnd;
			itsPattern.SetArtist(wFooter->GetString(itsPattern.GetArtist()));
			*w->Current().first << fmtBold << "Artist:   " << fmtBoldEnd << ' ' << ShowTag(itsPattern.GetArtist());
			break;
		}
		case 2:
		{
			Statusbar() << fmtBold << "Title: " << fmtBoldEnd;
			itsPattern.SetTitle(wFooter->GetString(itsPattern.GetTitle()));
			*w->Current().first << fmtBold << "Title:    " << fmtBoldEnd << ' ' << ShowTag(itsPattern.GetTitle());
			break;
		}
		case 3:
		{
			Statusbar() << fmtBold << "Album: " << fmtBoldEnd;
			itsPattern.SetAlbum(wFooter->GetString(itsPattern.GetAlbum()));
			*w->Current().first << fmtBold << "Album:    " << fmtBoldEnd << ' ' << ShowTag(itsPattern.GetAlbum());
			break;
		}
		case 4:
		{
			Statusbar() << fmtBold << "Filename: " << fmtBoldEnd;
			itsPattern.SetFile(wFooter->GetString(itsPattern.GetFile()));
			*w->Current().first << fmtBold << "Filename: " << fmtBoldEnd << ' ' << ShowTag(itsPattern.GetFile());
			break;
		}
		case 5:
		{
			Statusbar() << fmtBold << "Composer: " << fmtBoldEnd;
			itsPattern.SetComposer(wFooter->GetString(itsPattern.GetComposer()));
			*w->Current().first << fmtBold << "Composer: " << fmtBoldEnd << ' ' << ShowTag(itsPattern.GetComposer());
			break;
		}
		case 6:
		{
			Statusbar() << fmtBold << "Performer: " << fmtBoldEnd;
			itsPattern.SetPerformer(wFooter->GetString(itsPattern.GetPerformer()));
			*w->Current().first << fmtBold << "Performer:" << fmtBoldEnd << ' ' << ShowTag(itsPattern.GetPerformer());
			break;
		}
		case 7:
		{
			Statusbar() << fmtBold << "Genre: " << fmtBoldEnd;
			itsPattern.SetGenre(wFooter->GetString(itsPattern.GetGenre()));
			*w->Current().first << fmtBold << "Genre:    " << fmtBoldEnd << ' ' << ShowTag(itsPattern.GetGenre());
			break;
		}
		case 8:
		{
			Statusbar() << fmtBold << "Year: " << fmtBoldEnd;
			itsPattern.SetDate(wFooter->GetString(itsPattern.GetDate()));
			*w->Current().first << fmtBold << "Year:     " << fmtBoldEnd << ' ' << ShowTag(itsPattern.GetDate());
			break;
		}
		case 9:
		{
			Statusbar() << fmtBold << "Comment: " << fmtBoldEnd;
			itsPattern.SetComment(wFooter->GetString(itsPattern.GetComment()));
			*w->Current().first << fmtBold << "Comment:  " << fmtBoldEnd << ' ' << ShowTag(itsPattern.GetComment());
			break;
		}
		case 11:
		{
			Config.search_in_db = !Config.search_in_db;
			*w->Current().first << fmtBold << "Search in:" << fmtBoldEnd << ' ' << (Config.search_in_db ? "Database" : "Current playlist");
			break;
		}
		case 12:
		{
			MatchToPattern = !MatchToPattern;
			*w->Current().first << fmtBold << "Search mode:" << fmtBoldEnd << ' ' << (MatchToPattern ? NormalMode : StrictMode);
			break;
		}
		case 13:
		{
			CaseSensitive = !CaseSensitive * REG_ICASE;
			*w->Current().first << fmtBold << "Case sensitive:" << fmtBoldEnd << ' ' << (!CaseSensitive ? "Yes" : "No");
			break;
		}
		case 15:
		{
			ShowMessage("Searching...");
			if (w->Size() > StaticOptions)
				Prepare();
			Search();
			if (!w->Back().first)
			{
				if (Config.columns_in_search_engine)
					w->SetTitle(Display::Columns(Config.song_columns_list_format));
				size_t found = w->Size()-SearchEngine::StaticOptions;
				found += 3; // don't count options inserted below
				w->InsertSeparator(ResetButton+1);
				w->InsertOption(ResetButton+2, std::make_pair(static_cast<Buffer *>(0), static_cast<Song *>(0)), 1, 1);
				w->at(ResetButton+2).first = new Buffer();
				*w->at(ResetButton+2).first << Config.color1 << "Search results: " << Config.color2 << "Found " << found  << (found > 1 ? " songs" : " song") << clDefault;
				w->InsertSeparator(ResetButton+3);
				UpdateFoundList();
				ShowMessage("Searching finished!");
				if (Config.block_search_constraints_change)
					for (size_t i = 0; i < StaticOptions-4; ++i)
						w->Static(i, 1);
				w->Scroll(wDown);
				w->Scroll(wDown);
			}
			else
				ShowMessage("No results found");
			break;
		}
		case 16:
		{
			itsPattern.Clear();
			w->Reset();
			Prepare();
			ShowMessage("Search state reset");
			break;
		}
		default:
		{
			BlockItemListUpdate = 1;
			if (Config.ncmpc_like_songs_adding && w->isBold())
			{
				unsigned hash = w->Current().second->GetHash();
				for (size_t i = 0; i < myPlaylist->Main()->Size(); ++i)
				{
					if (myPlaylist->Main()->at(i).GetHash() == hash)
					{
						Mpd.Play(i);
						break;
					}
				}
			}
			else
			{
				const Song &s = *w->Current().second;
				int id = Mpd.AddSong(s);
				if (id >= 0)
				{
					Mpd.PlayID(id);
					ShowMessage("Added to playlist: %s", s.toString(Config.song_status_format).c_str());
					w->BoldOption(w->Choice(), 1);
				}
			}
			break;
		}
	}
	UnlockStatusbar();
}

void SearchEngine::SpacePressed()
{
	if (w->Current().first)
		return;
	
	if (Config.space_selects)
	{
		w->SelectCurrent();
		w->Scroll(wDown);
		return;
	}
	
	BlockItemListUpdate = 1;
	if (Config.ncmpc_like_songs_adding && w->isBold())
	{
		Playlist::BlockUpdate = 1;
		unsigned hash = w->Current().second->GetHash();
		Mpd.StartCommandsList();
		for (size_t i = 0; i < myPlaylist->Main()->Size(); ++i)
		{
			if (myPlaylist->Main()->at(i).GetHash() == hash)
			{
				Mpd.Delete(i);
				myPlaylist->Main()->DeleteOption(i);
				i--;
			}
		}
		Mpd.CommitCommandsList();
		w->BoldOption(w->Choice(), 0);
		Playlist::BlockUpdate = 0;
	}
	else
	{
		const Song &s = *w->Current().second;
		if (Mpd.AddSong(s) != -1)
		{
			ShowMessage("Added to playlist: %s", s.toString(Config.song_status_format).c_str());
			w->BoldOption(w->Choice(), 1);
		}
	}
	w->Scroll(wDown);
}

void SearchEngine::MouseButtonPressed(MEVENT me)
{
	if (w->Empty() || !w->hasCoords(me.x, me.y) || size_t(me.y) >= w->Size())
		return;
	if (me.bstate & BUTTON1_PRESSED || me.bstate & BUTTON3_PRESSED)
	{
		if (!w->Goto(me.y))
			return;
		w->Refresh();
		if ((me.bstate & BUTTON3_PRESSED || w->Choice() > 10) && w->Choice() < StaticOptions)
			EnterPressed();
		else if (w->Choice() >= StaticOptions)
		{
			if (me.bstate & BUTTON1_PRESSED)
			{
				size_t pos = w->Choice();
				SpacePressed();
				if (pos < w->Size()-1)
					w->Scroll(wUp);
			}
			else
				EnterPressed();
		}
	}
	else
		Screen< Menu< std::pair<Buffer *, MPD::Song *> > >::MouseButtonPressed(me);
}

MPD::Song *SearchEngine::CurrentSong()
{
	return !w->Empty() ? w->Current().second : 0;
}

void SearchEngine::GetSelectedSongs(MPD::SongList &v)
{
	std::vector<size_t> selected;
	w->GetSelected(selected);
	for (std::vector<size_t>::const_iterator it = selected.begin(); it != selected.end(); ++it)
	{
		v.push_back(new MPD::Song(*w->at(*it).second));
	}
}

void SearchEngine::ApplyFilter(const std::string &s)
{
	w->ApplyFilter(s, StaticOptions, REG_ICASE | Config.regex_type);
}

void SearchEngine::UpdateFoundList()
{
	bool bold = 0;
	for (size_t i = StaticOptions; i < w->Size(); ++i)
	{
		for (size_t j = 0; j < myPlaylist->Main()->Size(); ++j)
		{
			if (myPlaylist->Main()->at(j).GetHash() == w->at(i).second->GetHash())
			{
				bold = 1;
				break;
			}
		}
		w->BoldOption(i, bold);
		bold = 0;
	}
}

void SearchEngine::Prepare()
{
	for (size_t i = 0; i < w->Size(); ++i)
	{
		try
		{
			delete (*w)[i].first;
			delete (*w)[i].second;
		}
		catch (List::InvalidItem) { }
	}
	
	w->SetTitle("");
	w->Clear(0);
	w->ResizeList(17);
	
	w->IntoSeparator(10);
	w->IntoSeparator(14);
	
	for (size_t i = 0; i < 17; ++i)
	{
		try
		{
			w->at(i).first = new Buffer();
		}
		catch (List::InvalidItem) { }
	}
	
	*w->at(0).first << fmtBold << "Any:      " << fmtBoldEnd << ' ' << ShowTag(itsPattern.Any());
	*w->at(1).first << fmtBold << "Artist:   " << fmtBoldEnd << ' ' << ShowTag(itsPattern.GetArtist());
	*w->at(2).first << fmtBold << "Title:    " << fmtBoldEnd << ' ' << ShowTag(itsPattern.GetTitle());
	*w->at(3).first << fmtBold << "Album:    " << fmtBoldEnd << ' ' << ShowTag(itsPattern.GetAlbum());
	*w->at(4).first << fmtBold << "Filename: " << fmtBoldEnd << ' ' << ShowTag(itsPattern.GetName());
	*w->at(5).first << fmtBold << "Composer: " << fmtBoldEnd << ' ' << ShowTag(itsPattern.GetComposer());
	*w->at(6).first << fmtBold << "Performer:" << fmtBoldEnd << ' ' << ShowTag(itsPattern.GetPerformer());
	*w->at(7).first << fmtBold << "Genre:    " << fmtBoldEnd << ' ' << ShowTag(itsPattern.GetGenre());
	*w->at(8).first << fmtBold << "Year:     " << fmtBoldEnd << ' ' << ShowTag(itsPattern.GetDate());
	*w->at(9).first << fmtBold << "Comment:  " << fmtBoldEnd << ' ' << ShowTag(itsPattern.GetComment());
	
	*w->at(11).first << fmtBold << "Search in:" << fmtBoldEnd << ' ' << (Config.search_in_db ? "Database" : "Current playlist");
	*w->at(12).first << fmtBold << "Search mode:" << fmtBoldEnd << ' ' << (MatchToPattern ? NormalMode : StrictMode);
	*w->at(13).first << fmtBold << "Case sensitive:" << fmtBoldEnd << ' ' << (!CaseSensitive ? "Yes" : "No");
	
	*w->at(15).first << "Search";
	*w->at(16).first << "Reset";
}

void SearchEngine::Search()
{
	if (itsPattern.Empty())
		return;
	
	SearchPattern s = itsPattern;
	
	SongList list;
	if (Config.search_in_db)
		Mpd.GetDirectoryRecursive("/", list);
	else
	{
		list.reserve(myPlaylist->Main()->Size());
		for (size_t i = 0; i < myPlaylist->Main()->Size(); ++i)
			list.push_back(&(*myPlaylist->Main())[i]);
	}
	
	bool any_found = 1;
	bool found = 1;
	
	if (!CaseSensitive && !MatchToPattern)
	{
		std::string t;
		t = s.Any();
		ToLower(t);
		s.Any(t);
		
		t = s.GetArtist();
		ToLower(t);
		s.SetArtist(t);
		
		t = s.GetTitle();
		ToLower(t);
		s.SetTitle(t);
		
		t = s.GetAlbum();
		ToLower(t);
		s.SetAlbum(t);
		
		t = s.GetFile();
		ToLower(t);
		s.SetFile(t);
		
		t = s.GetComposer();
		ToLower(t);
		s.SetComposer(t);
		
		t = s.GetPerformer();
		ToLower(t);
		s.SetPerformer(t);
		
		t = s.GetGenre();
		ToLower(t);
		s.SetGenre(t);
		
		t = s.GetComment();
		ToLower(t);
		s.SetComment(t);
	}
	
	for (SongList::const_iterator it = list.begin(); it != list.end(); ++it)
	{
		(*it)->CopyPtr(CaseSensitive || MatchToPattern);
		Song copy = **it;
		
		if (!CaseSensitive && !MatchToPattern)
		{
			std::string t;
			t = copy.GetArtist();
			ToLower(t);
			copy.SetArtist(t);
			
			t = copy.GetTitle();
			ToLower(t);
			copy.SetTitle(t);
			
			t = copy.GetAlbum();
			ToLower(t);
			copy.SetAlbum(t);
			
			t = copy.GetName();
			ToLower(t);
			copy.SetFile(t);
			
			t = copy.GetComposer();
			ToLower(t);
			copy.SetComposer(t);
			
			t = copy.GetPerformer();
			ToLower(t);
			copy.SetPerformer(t);
			
			t = copy.GetGenre();
			ToLower(t);
			copy.SetGenre(t);
			
			t = copy.GetComment();
			ToLower(t);
			copy.SetComment(t);
		}
		
		if (MatchToPattern)
		{
			regex_t rx;
			
			if (!s.Any().empty())
			{
				if (regcomp(&rx, s.Any().c_str(), CaseSensitive | Config.regex_type) == 0)
				{
					any_found =
						regexec(&rx, copy.GetArtist().c_str(), 0, 0, 0) == 0
					||	regexec(&rx, copy.GetTitle().c_str(), 0, 0, 0) == 0
					||	regexec(&rx, copy.GetAlbum().c_str(), 0, 0, 0) == 0
					||	regexec(&rx, copy.GetName().c_str(), 0, 0, 0) == 0
					||	regexec(&rx, copy.GetComposer().c_str(), 0, 0, 0) == 0
					||	regexec(&rx, copy.GetPerformer().c_str(), 0, 0, 0) == 0
					||	regexec(&rx, copy.GetGenre().c_str(), 0, 0, 0) == 0
					||	regexec(&rx, copy.GetDate().c_str(), 0, 0, 0) == 0
					||	regexec(&rx, copy.GetComment().c_str(), 0, 0, 0) == 0;
				}
				regfree(&rx);
			}
			
			if (found && !s.GetArtist().empty())
			{
				if (regcomp(&rx, s.GetArtist().c_str(), CaseSensitive | Config.regex_type) == 0)
					found = regexec(&rx, copy.GetArtist().c_str(), 0, 0, 0) == 0;
				regfree(&rx);
			}
			if (found && !s.GetTitle().empty())
			{
				if (regcomp(&rx, s.GetTitle().c_str(), CaseSensitive | Config.regex_type) == 0)
					found = regexec(&rx, copy.GetTitle().c_str(), 0, 0, 0) == 0;
				regfree(&rx);
			}
			if (found && !s.GetAlbum().empty())
			{
				if (regcomp(&rx, s.GetAlbum().c_str(), CaseSensitive | Config.regex_type) == 0)
					found = regexec(&rx, copy.GetAlbum().c_str(), 0, 0, 0) == 0;
				regfree(&rx);
			}
			if (found && !s.GetFile().empty())
			{
				if (regcomp(&rx, s.GetFile().c_str(), CaseSensitive | Config.regex_type) == 0)
					found = regexec(&rx, copy.GetName().c_str(), 0, 0, 0) == 0;
				regfree(&rx);
			}
			if (found && !s.GetComposer().empty())
			{
				if (regcomp(&rx, s.GetComposer().c_str(), CaseSensitive | Config.regex_type) == 0)
					found = regexec(&rx, copy.GetComposer().c_str(), 0, 0, 0) == 0;
				regfree(&rx);
			}
			if (found && !s.GetPerformer().empty())
			{
				if (regcomp(&rx, s.GetPerformer().c_str(), CaseSensitive | Config.regex_type) == 0)
					found = regexec(&rx, copy.GetPerformer().c_str(), 0, 0, 0) == 0;
				regfree(&rx);
			}
			if (found && !s.GetGenre().empty())
			{
				if (regcomp(&rx, s.GetGenre().c_str(), CaseSensitive | Config.regex_type) == 0)
					found = regexec(&rx, copy.GetGenre().c_str(), 0, 0, 0) == 0;
				regfree(&rx);
			}
			if (found && !s.GetDate().empty())
			{
				if (regcomp(&rx, s.GetDate().c_str(), CaseSensitive | Config.regex_type) == 0)
					found = regexec(&rx, copy.GetDate().c_str(), 0, 0, 0) == 0;
				regfree(&rx);
			}
			if (found && !s.GetComment().empty())
			{
				if (regcomp(&rx, s.GetComment().c_str(), CaseSensitive | Config.regex_type) == 0)
					found = regexec(&rx, copy.GetComment().c_str(), 0, 0, 0) == 0;
				regfree(&rx);
			}
		}
		else
		{
			if (!s.Any().empty())
				any_found =
					copy.GetArtist() == s.Any()
				||	copy.GetTitle() == s.Any()
				||	copy.GetAlbum() == s.Any()
				||	copy.GetName() == s.Any()
				||	copy.GetComposer() == s.Any()
				||	copy.GetPerformer() == s.Any()
				||	copy.GetGenre() == s.Any()
				||	copy.GetDate() == s.Any()
				||	copy.GetComment() == s.Any();
			
			if (found && !s.GetArtist().empty())
				found = copy.GetArtist() == s.GetArtist();
			if (found && !s.GetTitle().empty())
				found = copy.GetTitle() == s.GetTitle();
			if (found && !s.GetAlbum().empty())
				found = copy.GetAlbum() == s.GetAlbum();
			if (found && !s.GetFile().empty())
				found = copy.GetName() == s.GetFile();
			if (found && !s.GetComposer().empty())
				found = copy.GetComposer() == s.GetComposer();
			if (found && !s.GetPerformer().empty())
				found = copy.GetPerformer() == s.GetPerformer();
			if (found && !s.GetGenre().empty())
				found = copy.GetGenre() == s.GetGenre();
			if (found && !s.GetDate().empty())
				found = copy.GetDate() == s.GetDate();
			if (found && !s.GetComment().empty())
				found = copy.GetComment() == s.GetComment();
		}
		
		if (CaseSensitive || MatchToPattern)
			copy.NullMe();
		(*it)->CopyPtr(0);
		
		if (found && any_found)
		{
			Song *ss = Config.search_in_db ? *it : new Song(**it);
			w->AddOption(std::make_pair(static_cast<Buffer *>(0), ss));
			list[it-list.begin()] = 0;
		}
		found = 1;
		any_found = 1;
	}
	if (Config.search_in_db) // free song list only if it's database
		FreeSongList(list);
}

std::string SearchEngine::SearchEngineOptionToString(const std::pair<Buffer *, MPD::Song *> &pair, void *)
{
	if (!Config.columns_in_search_engine)
		return pair.second->toString(Config.song_list_format);
	else
		return Playlist::SongInColumnsToString(*pair.second, &Config.song_columns_list_format);
}

