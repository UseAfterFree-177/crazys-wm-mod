/*
* Copyright 2009, 2010, The Pink Petal Development Team.
* The Pink Petal Devloment Team are defined as the game's coders
* who meet on http://pinkpetal.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "buildings/cBrothel.h"
#include "cScreenGirlDetails.h"
#include "interface/cWindowManager.h"
#include "cTariff.h"
#include "cJobManager.h"
#include "cGangs.h"
#include "Game.hpp"
#include "CLog.h"
#include "combat/combat.h"
#include "sConfig.h"
#include "character/traits/ITraitSpec.h"

extern	int	g_TalkCount;
extern bool g_AllTogle;
extern cConfig cfg;

extern	bool	g_CTRLDown;

static stringstream ss;

static int ImageNum = -1;
static int DetailLevel = 0;
static bool Day0Night1 = SHIFT_DAY;

cScreenGirlDetails::cScreenGirlDetails() : cGameWindow("girl_details_screen.xml")
{
}

void cScreenGirlDetails::set_ids()
{
	back_id			/**/ = get_id("BackButton", "Back");
	girlname_id		/**/ = get_id("GirlName");
	girldesc_id		/**/ = get_id("GirlDescription");
	girlimage_id	/**/ = get_id("GirlImage");
	more_id			/**/ = get_id("MoreButton");
	antipreg_id		/**/ = get_id("UseAntiPregToggle");
	prev_id			/**/ = get_id("PrevButton","Prev");
	next_id			/**/ = get_id("NextButton","Next");
	inventory_id	/**/ = get_id("InventoryButton");
	senddungeon_id	/**/ = get_id("SendDungeonButton");
	reldungeon_id	/**/ = get_id("ReleaseDungeonButton");
	interact_id		/**/ = get_id("InteractButton");
	interactc_id	/**/ = get_id("InteractCount");
	takegold_id		/**/ = get_id("TakeGoldButton");
	accomup_id		/**/ = get_id("AccomUpButton");//
	accomdown_id	/**/ = get_id("AccomDownButton");//
	accom_id		/**/ = get_id("AccomSlider");
	accomval_id		/**/ = get_id("AccomValue");
	houseperc_id	/**/ = get_id("HousePercSlider");
	housepercval_id	/**/ = get_id("HousePercValue");
	gallery_id		/**/ = get_id("GalleryButton");
	jobtypehead_id	/**/ = get_id("JobTypeHeader");
	jobtypelist_id	/**/ = get_id("JobTypeList");
	jobhead_id		/**/ = get_id("JobHeader");
	joblist_id		/**/ = get_id("JobList");
	day_id			/**/ = get_id("DayButton");
	night_id		/**/ = get_id("NightButton");
	traithead_id	/**/ = get_id("TraitHeader");
	traitlist_id	/**/ = get_id("TraitList");
	traitdesc_id	/**/ = get_id("TraitDescription");

	SetButtonNavigation(back_id, "<back>");
	SetButtonNavigation(gallery_id, "Gallery");
	SetButtonHotKey(gallery_id, SDLK_SPACE);
	SetButtonCallback(day_id, [this]( ) { set_shift(SHIFT_DAY); });
	SetButtonCallback(night_id, [this]( ) { set_shift(SHIFT_NIGHT); });

	SetButtonCallback(next_id, [this]( ) { NextGirl(); });
	SetButtonCallback(prev_id, [this]( ) { PrevGirl(); });
	SetButtonHotKey(prev_id, SDLK_a);
	SetButtonHotKey(next_id, SDLK_d);

	SetButtonCallback(interact_id, [this]( ) { do_interaction(); });
	SetButtonCallback(senddungeon_id, [this]( ) { send_to_dungeon(); });
	SetButtonCallback(reldungeon_id, [this]( ) { release_from_dungeon(); });
	SetButtonCallback(takegold_id, [this]( ) { take_gold(m_SelectedGirl); });
	SetButtonCallback(accomup_id, [this]( ) { update_accomodation(1); });
	SetButtonCallback(accomdown_id, [this]( ) { update_accomodation(-1); });
	SetButtonCallback(more_id, [this]( ) {
        if (DetailLevel == 0)		{ DetailLevel = 1; EditTextItem(cGirls::GetMoreDetailsString(GetGraphics(), m_SelectedGirl), girldesc_id); }
        else if (DetailLevel == 1)	{ DetailLevel = 2; EditTextItem(cGirls::GetThirdDetailsString(m_SelectedGirl), girldesc_id); }
        else						{ DetailLevel = 0; EditTextItem(cGirls::GetDetailsString(GetGraphics(), m_SelectedGirl), girldesc_id); }
	});
	SetButtonCallback(inventory_id, [this]( ) {
        if (m_SelectedGirl)
        {
            if (m_SelectedGirl->is_dead()) return;
            g_AllTogle = true;
            push_window("Item Management");
            return;
        }
	});

	SetListBoxSelectionCallback(traitlist_id, [this](int selection){
        EditTextItem((selection != -1 ? m_TraitInfoCache.at(selection).trait->desc() : ""), traitdesc_id);
	});
    SetListBoxSelectionCallback(jobtypelist_id, [this](int selection){
        RefreshJobList();
    });
    SetListBoxSelectionCallback(joblist_id, [this](int selection){
        if (selection != -1) { on_select_job(selection, g_CTRLDown);  }
    });

    SetCheckBoxCallback(antipreg_id, [this](int on) {
        m_SelectedGirl->m_UseAntiPreg = on;
    });

    SetSliderCallback(houseperc_id, [this](int value) { set_house_percentage(value); });
    SetSliderCallback(accom_id, [this](int value) { set_accomodation(value); });
}

void cScreenGirlDetails::init(bool back)
{
    m_Refresh = false;
    m_SelectedGirl = &active_girl();
    m_TraitInfoCache = m_SelectedGirl->get_trait_info();
	Focused();

	if (m_SelectedGirl->is_dead())
	{
		// `J` instead of removing dead girls from the list which breaks the game, just skip past her in the list.
		NextGirl();								// `J` currently this prevents scrolling backwards past her - need to fix that.
		//		selected_girl = remove_selected_girl();
		if (m_SelectedGirl == nullptr || m_SelectedGirl->is_dead())
		{
            pop_window();
			return;
		}
	}

	int job = (Day0Night1 ? m_SelectedGirl->m_NightJob : m_SelectedGirl->m_DayJob);

    EditTextItem(m_SelectedGirl->FullName(), girlname_id);

	string detail;
	if (DetailLevel == 0)		detail = cGirls::GetDetailsString(GetGraphics(), m_SelectedGirl);
	else if (DetailLevel == 1)	detail = cGirls::GetMoreDetailsString(GetGraphics(), m_SelectedGirl);
	else						detail = cGirls::GetThirdDetailsString(m_SelectedGirl);
	EditTextItem(detail, girldesc_id);
	
    if (lastsexact != -1)
    {
        PrepareImage(girlimage_id, m_SelectedGirl, lastsexact, true, ImageNum);
        lastsexact = -1;
    }
    else
    {
        PrepareImage(girlimage_id, m_SelectedGirl, IMGTYPE_PROFILE, true, ImageNum);
    }


	SliderRange(houseperc_id, 0, 100, m_SelectedGirl->house(), 10);
	ss.str("");	ss << "House Percentage: " << SliderValue(houseperc_id) << "%";
	EditTextItem(ss.str(), housepercval_id);

	ClearListBox(jobtypelist_id);

	// `J` Replacing accom buttons with slider
	if (accomdown_id != -1) DisableWidget(accomdown_id, m_SelectedGirl->m_AccLevel < 1);
	if (accomup_id != -1) DisableWidget(accomup_id, m_SelectedGirl->m_AccLevel > 9);
	if (accom_id != -1)
	{
		SliderRange(accom_id, 0, 9, m_SelectedGirl->m_AccLevel, 1);
		SliderMarker(accom_id, cGirls::PreferredAccom(m_SelectedGirl));
	}
	if (accomval_id != -1)
	{
		ss.str(""); ss << "Accommodation: " << cGirls::Accommodation(SliderValue(accom_id));
		if (cfg.debug.log_extradetails())
		{
			int val = SliderValue(accom_id) - cGirls::PreferredAccom(m_SelectedGirl);
			if (val != 0) ss << "  ( " << (val > 0 ? "+" : "") << val << " )";
		}
		EditTextItem(ss.str(), accomval_id);
	}
    DisableWidget(interact_id, (g_TalkCount <= 0));
	if (interactc_id >= 0)
	{
		ss.str(""); ss << "Interactions Left: ";
		if (g_Game->allow_cheats()) ss << "Infinate Cheat";
		else if (g_TalkCount <= 0) ss << "0 (buy in House screen)";
		else ss << g_TalkCount;
		EditTextItem(ss.str(), interactc_id);
	}
    DisableWidget(takegold_id, (m_SelectedGirl->m_Money <= 0));
    DisableWidget(accomup_id, (m_SelectedGirl->m_AccLevel >= 9));
    DisableWidget(accomdown_id, (m_SelectedGirl->m_AccLevel <= 0));
	SetCheckBox(antipreg_id, (m_SelectedGirl->m_UseAntiPreg));

    IBuilding* pBuilding = m_SelectedGirl->m_Building;
    DisableWidget(reldungeon_id, m_SelectedGirl->m_DayJob != JOB_INDUNGEON);
    DisableWidget(senddungeon_id, m_SelectedGirl->m_DayJob == JOB_INDUNGEON);

	ClearListBox(joblist_id);
	bool HideDNButtons = false;

	if(m_SelectedGirl->m_DayJob == JOB_INDUNGEON)
    {  // if in dungeon, effectively disable job lists
        HideDNButtons = true;
        ClearListBox(joblist_id);
        AddToListBox(jobtypelist_id, -1, "Languishing in dungeon");
        SetSelectedItemInList(jobtypelist_id, 0, false);
    } else if(pBuilding) {
        switch (pBuilding->type()) {
        case BuildingType::ARENA:
            AddToListBox(jobtypelist_id, JOBFILTER_ARENASTAFF,
                         g_Game->job_manager().JobFilters[JOBFILTER_ARENASTAFF].Name);
            AddToListBox(jobtypelist_id, JOBFILTER_ARENA, g_Game->job_manager().JobFilters[JOBFILTER_ARENA].Name);
            RefreshJobList();
            if (g_Game->job_manager().job_filter(JOBFILTER_ARENA, (JOBS)job))
                SetSelectedItemInList(jobtypelist_id, JOBFILTER_ARENA);
            else // if (g_Game->job_manager().job_filter(JOBFILTER_ARENASTAFF, (JOBS)job))
                SetSelectedItemInList(jobtypelist_id, JOBFILTER_ARENASTAFF);
            break;
        case BuildingType::CLINIC:
            AddToListBox(jobtypelist_id, JOBFILTER_CLINIC, g_Game->job_manager().JobFilters[JOBFILTER_CLINIC].Name);
            AddToListBox(jobtypelist_id, JOBFILTER_CLINICSTAFF, g_Game->job_manager().JobFilters[JOBFILTER_CLINICSTAFF].Name);
            RefreshJobList();
            if (g_Game->job_manager().job_filter(JOBFILTER_CLINIC, (JOBS)job))
                SetSelectedItemInList(jobtypelist_id, JOBFILTER_CLINIC);
            else SetSelectedItemInList(jobtypelist_id, JOBFILTER_CLINICSTAFF);
            break;
        case BuildingType::CENTRE:
            AddToListBox(jobtypelist_id, JOBFILTER_COMMUNITYCENTRE, g_Game->job_manager().JobFilters[JOBFILTER_COMMUNITYCENTRE].Name);
            AddToListBox(jobtypelist_id, JOBFILTER_COUNSELINGCENTRE, g_Game->job_manager().JobFilters[JOBFILTER_COUNSELINGCENTRE].Name);
            RefreshJobList();
            if (g_Game->job_manager().job_filter(JOBFILTER_COUNSELINGCENTRE, (JOBS)job))
                SetSelectedItemInList(jobtypelist_id, JOBFILTER_COUNSELINGCENTRE);
            else SetSelectedItemInList(jobtypelist_id, JOBFILTER_COMMUNITYCENTRE);
            break;
        case BuildingType::HOUSE:
            AddToListBox(jobtypelist_id, JOBFILTER_HOUSE, g_Game->job_manager().JobFilters[JOBFILTER_HOUSE].Name);
            AddToListBox(jobtypelist_id, JOBFILTER_HOUSETTRAINING, g_Game->job_manager().JobFilters[JOBFILTER_HOUSETTRAINING].Name);
            RefreshJobList();
            if (g_Game->job_manager().job_filter(JOBFILTER_HOUSETTRAINING, (JOBS)job))
                SetSelectedItemInList(jobtypelist_id, JOBFILTER_HOUSETTRAINING);
            else SetSelectedItemInList(jobtypelist_id, JOBFILTER_HOUSE);
            break;
        case BuildingType::STUDIO:
            Day0Night1 = SHIFT_NIGHT;
            HideDNButtons = true;
            // `J` When adding new Studio Scenes, search for "J-Add-New-Scenes"  :  found in >> cScreenGirlDetails.cpp
            AddToListBox(jobtypelist_id, JOBFILTER_STUDIOCREW, g_Game->job_manager().JobFilters[JOBFILTER_STUDIOCREW].Name);
            AddToListBox(jobtypelist_id, JOBFILTER_STUDIONONSEX, g_Game->job_manager().JobFilters[JOBFILTER_STUDIONONSEX].Name);
            AddToListBox(jobtypelist_id, JOBFILTER_STUDIOSOFTCORE, g_Game->job_manager().JobFilters[JOBFILTER_STUDIOSOFTCORE].Name);
            AddToListBox(jobtypelist_id, JOBFILTER_STUDIOPORN, g_Game->job_manager().JobFilters[JOBFILTER_STUDIOPORN].Name);
            AddToListBox(jobtypelist_id, JOBFILTER_STUDIOHARDCORE, g_Game->job_manager().JobFilters[JOBFILTER_STUDIOHARDCORE].Name);
            AddToListBox(jobtypelist_id, JOBFILTER_RANDSTUDIO, g_Game->job_manager().JobFilters[JOBFILTER_RANDSTUDIO].Name);
            RefreshJobList();
            /* */if (g_Game->job_manager().job_filter(JOBFILTER_STUDIONONSEX, (JOBS)job))		SetSelectedItemInList(jobtypelist_id, JOBFILTER_STUDIONONSEX);
            else if (g_Game->job_manager().job_filter(JOBFILTER_STUDIOSOFTCORE, (JOBS)job))	SetSelectedItemInList(jobtypelist_id, JOBFILTER_STUDIOSOFTCORE);
            else if (g_Game->job_manager().job_filter(JOBFILTER_STUDIOPORN, (JOBS)job))		SetSelectedItemInList(jobtypelist_id, JOBFILTER_STUDIOPORN);
            else if (g_Game->job_manager().job_filter(JOBFILTER_STUDIOHARDCORE, (JOBS)job))	SetSelectedItemInList(jobtypelist_id, JOBFILTER_STUDIOHARDCORE);
            else if (g_Game->job_manager().job_filter(JOBFILTER_RANDSTUDIO, (JOBS)job))		SetSelectedItemInList(jobtypelist_id, JOBFILTER_RANDSTUDIO);
            else SetSelectedItemInList(jobtypelist_id, JOBFILTER_STUDIOCREW);
            break;
        case BuildingType::FARM:
            AddToListBox(jobtypelist_id, JOBFILTER_FARMSTAFF, g_Game->job_manager().JobFilters[JOBFILTER_FARMSTAFF].Name);
            AddToListBox(jobtypelist_id, JOBFILTER_LABORERS, g_Game->job_manager().JobFilters[JOBFILTER_LABORERS].Name);
            AddToListBox(jobtypelist_id, JOBFILTER_PRODUCERS, g_Game->job_manager().JobFilters[JOBFILTER_PRODUCERS].Name);
            RefreshJobList();
            if (g_Game->job_manager().job_filter(JOBFILTER_LABORERS, (JOBS)job))
                SetSelectedItemInList(jobtypelist_id, JOBFILTER_LABORERS);
            else if (g_Game->job_manager().job_filter(JOBFILTER_PRODUCERS, (JOBS)job))
                SetSelectedItemInList(jobtypelist_id, JOBFILTER_PRODUCERS);
            else SetSelectedItemInList(jobtypelist_id, JOBFILTER_FARMSTAFF);
        case BuildingType::BROTHEL:
            // if not in dungeon, set up job lists
            // add the job filters
            //	for(int i=0; i<NUMJOBTYPES; i++)  // loop through all job types
            for (unsigned int i = 0; i <= JOBFILTER_BROTHEL; i++)  // temporary limit to job types shown
            {
                AddToListBox(jobtypelist_id, i, g_Game->job_manager().JobFilters[i].Name);
            }

            // set the job filter
            int jobtype = 0;
            for (unsigned int i = 0; i < NUMJOBTYPES; i++)
            {
                if (g_Game->job_manager().job_filter(i, (JOBS)job))
                {
                    jobtype = i;
                    break;
                }
            }
            SetSelectedItemInList(jobtypelist_id, jobtype);
            RefreshJobList();
        }
    }

    HideWidget(day_id, HideDNButtons);
    HideWidget(night_id, HideDNButtons);
	if (!HideDNButtons)
	{
        DisableWidget(day_id, (Day0Night1 == SHIFT_DAY));
        DisableWidget(night_id, (Day0Night1 == SHIFT_NIGHT));
	}

	ClearListBox(traitlist_id);
	for (int i = 0; i < m_TraitInfoCache.size(); ++i)
	{
	    auto& trait = m_TraitInfoCache[i];
	    if(!trait.active)
	        continue;
        ss.str(""); ss << trait.trait->display_name();
        if (trait.remaining_time > 0) ss << "   (" << trait.remaining_time << ")";
        AddToListBox(traitlist_id, i, ss.str());
	}
	EditTextItem("", traitdesc_id);
}

void cScreenGirlDetails::set_accomodation(int value)
{
    m_SelectedGirl->m_AccLevel = value;
    SliderRange(accom_id, 0, 9, m_SelectedGirl->m_AccLevel, 1);
    if (accomval_id != -1)
    {
        ss.str(""); ss << "Accommodation: " << cGirls::Accommodation(value);
        if (cfg.debug.log_extradetails())
        {
            int val = SliderValue(accom_id) - cGirls::PreferredAccom(m_SelectedGirl);
            if (val != 0) ss << "  ( " << (val > 0 ? "+" : "") << val << " )";
        }
        EditTextItem(ss.str(), accomval_id);
    }
    init(false);
}

void cScreenGirlDetails::set_house_percentage(int value)
{
    m_SelectedGirl->house(value);
    ss.str("");
    ss << "House Percentage: " << value << "%";
    EditTextItem(ss.str(), housepercval_id);
    if (DetailLevel == 0)								// Rebelliousness might have changed, so update details
    {
        string detail = cGirls::GetDetailsString(GetGraphics(), m_SelectedGirl);
        EditTextItem(detail, girldesc_id);
    }
}

void cScreenGirlDetails::on_select_job(int selection, bool fulltime)
{// `J` When modifying Jobs, search for "J-Change-Jobs"  :  found in >>
    int old_job = (Day0Night1 ? m_SelectedGirl->m_NightJob : m_SelectedGirl->m_DayJob);
    // handle special job requirements and assign - if HandleSpecialJobs returns true, the job assignment was modified or cancelled
    if (g_Game->job_manager().HandleSpecialJobs(m_SelectedGirl, selection, old_job, Day0Night1, fulltime))
    {
        selection = (Day0Night1 ? m_SelectedGirl->m_NightJob : m_SelectedGirl->m_DayJob);
        SetSelectedItemInList(joblist_id, selection, false);
    }
    // refresh job worker counts for former job and current job
    if (old_job != selection)
    {
        stringstream text;
        text << g_Game->job_manager().JobData[old_job].name << " (" << m_SelectedGirl->m_Building->num_girls_on_job((JOBS)old_job, Day0Night1) << ")";
        SetSelectedItemText(joblist_id, old_job, text.str());
        text.str("");
        text << g_Game->job_manager().JobData[selection].name << " (" << m_SelectedGirl->m_Building->num_girls_on_job((JOBS)selection, Day0Night1) << ")";
    }
    RefreshJobList();
}

void cScreenGirlDetails::update_accomodation(int accadj)
{
    m_SelectedGirl->m_AccLevel += accadj;
    if (m_SelectedGirl->m_AccLevel > 9) m_SelectedGirl->m_AccLevel = 9;
    else if (m_SelectedGirl->m_AccLevel < 0) m_SelectedGirl->m_AccLevel = 0;
    if (accomval_id != -1) EditTextItem("Accommodation: " + cGirls::Accommodation(m_SelectedGirl->m_AccLevel),
                                        accomval_id);
    init(false);
}

void cScreenGirlDetails::release_from_dungeon()
{
    if (!m_SelectedGirl) return;
    IBuilding& current_brothel = active_building();
    if (current_brothel.free_rooms() <= 0)
    {
        g_Game->push_message("The current brothel has no more room.\nBuy a new one, get rid of some girls, or change the brothel you are currently managing.", 0);
    }
    else
    {
        sGirl* nextGirl = get_next_girl();
        auto tempGirl = g_Game->dungeon().RemoveGirl(m_SelectedGirl);
        current_brothel.add_girl(tempGirl.release());

        if (g_Game->dungeon().GetNumGirls() == 0)
        {
            m_SelectedGirl = nullptr;
            pop_window();
        }
        else m_SelectedGirl = nextGirl;
    }
    init(false);
}

void cScreenGirlDetails::send_to_dungeon()
{
    if (!m_SelectedGirl) return;
    sGirl* nextGirl = get_next_girl();
    if(g_Game->dungeon().SendGirlToDungeon(*m_SelectedGirl)) {
        // if this was successful
    }
    set_active_girl(nextGirl);

    // cannot call init() directly, because we would enable the release button directly under the pressed mouse
    m_Refresh = true;
}

void cScreenGirlDetails::do_interaction()
{
    if (!m_SelectedGirl) return;
    g_Game->TalkToGirl(*m_SelectedGirl);
    init(false);
}

void cScreenGirlDetails::set_shift(int shift)
{
    Day0Night1 = shift;
    DisableWidget(day_id, shift == SHIFT_DAY);
    DisableWidget(night_id, shift == SHIFT_NIGHT);
    init(false);
}

void cScreenGirlDetails::RefreshJobList()
{
	ClearListBox(joblist_id);
	int job_filter = GetSelectedItemFromList(jobtypelist_id);
	if (job_filter == -1) return;

	string text;
	// populate Jobs listbox with jobs in the selected category
	for (auto i : g_Game->job_manager().JobFilters[job_filter].Contents)
	{
		if (g_Game->job_manager().JobData[i].name.empty()) continue;
		std::stringstream btext;
        btext << g_Game->job_manager().JobData[i].name;
        btext << " (";
        if(m_SelectedGirl->m_Building) {
            btext << m_SelectedGirl->m_Building->num_girls_on_job((JOBS)i, Day0Night1);
        } else {
            btext << '?';
        }
        btext << ")";
		AddToListBox(joblist_id, i, btext.str());
	}
	if (m_SelectedGirl)
	{
		int sel_job = Day0Night1 ? m_SelectedGirl->m_NightJob : m_SelectedGirl->m_DayJob;
		SetSelectedItemInList(joblist_id, sel_job, false);
	}
}

void cScreenGirlDetails::PrevGirl()
{
    set_active_girl(get_prev_girl());
    init(false);
}

void cScreenGirlDetails::NextGirl()
{
    auto candidate = get_next_girl();
    set_active_girl(candidate);
    // if we did change the girl, update screen
    if(m_SelectedGirl != &active_girl()) {
        init(false);
    }
}

sGirl *cScreenGirlDetails::get_prev_girl()		// return previous girl in the sorted list
{
	if (m_SelectedGirl->m_DayJob == JOB_INDUNGEON)
    {
        return g_Game->dungeon().GetGirl(g_Game->dungeon().GetGirlPos(m_SelectedGirl) - 1)->m_Girl.get();
    }
    else
    {
        auto source_building = m_SelectedGirl->m_Building;

        auto index = source_building->get_girl_index(*m_SelectedGirl);
        if(index == -1) {
            g_LogFile.log(ELogLevel::ERROR, "Girl is not in building she claims she is!");
            return m_SelectedGirl;
        } else if (index == 0) {
            return source_building->get_girl(source_building->num_girls() - 1);
        }
        return source_building->get_girl(index - 1);
    }
}

sGirl *cScreenGirlDetails::get_next_girl()		// return next girl in the sorted list
{
    if (m_SelectedGirl->m_DayJob == JOB_INDUNGEON)
    {
        return g_Game->dungeon().GetGirl(g_Game->dungeon().GetGirlPos(m_SelectedGirl) + 1)->m_Girl.get();
    }
    else
    {
        auto source_building = m_SelectedGirl->m_Building;
        auto index = source_building->get_girl_index(*m_SelectedGirl);
        if(index == -1) {
            g_LogFile.log(ELogLevel::ERROR, "Girl is not in building she claims she is!");
            return m_SelectedGirl;
        } else if (index == source_building->num_girls() - 1) {
            return source_building->get_girl(0);
        }
        return source_building->get_girl(index + 1);
    }
}

bool cScreenGirlDetails::do_take_gold(sGirl *girl, string &message)	// returns TRUE if the girl won
{
	const int GIRL_LOSES = false;
	const int GIRL_WINS = true;
	bool girl_win_flag = GIRL_WINS;

	// she thinks about escape
	auto result = AttemptEscape(*girl);
	if (result == EGirlEscapeAttemptResult::STOPPED_BY_GOONS)
	{		// put her in the dungeon
		message += "She puts up a fight but your goons manage to subdue her and you take her gold anyway.";
		return girl_win_flag;
	} else if (result == EGirlEscapeAttemptResult::SUBMITS ) {
        message += "She quietly allows you to take her gold.";
        return GIRL_LOSES;	// no fight -> girl lose
	} else if(result == EGirlEscapeAttemptResult::STOPPED_BY_PLAYER) {
        /*
        *	from here on down, the girl won against the goons
        */
        // TODO need to know whether there was a GANG fight
        message += "She puts up a fight and your goons cannot stop her, ";
        message += "but you defeat her yourself and take her gold.";
        return false;	// girl did not win, after all
    } else {
        /*
        *	Looks like she won: put her out of the brothel
        *	and post her as a runaway
        */
        message += "after defeating you as well she escapes to the outside.\n";

        sGirl* nextGirl = get_next_girl();
        girl->run_away();

        stringstream smess;
        smess << girl->FullName() << " has run away";
        g_Game->push_message(smess.str(), 1);

        m_SelectedGirl = nextGirl;
        init(false);

        if (m_SelectedGirl == nullptr) pop_window();

        return true;	// the girl still won
	}
}

void cScreenGirlDetails::take_gold(sGirl *girl)
{
	string message;
	bool girl_win = do_take_gold(girl, message);
	/*
	*	if the girl won, then we're pretty much sorted
	*	display the message and return
	*/
	if (girl_win)
	{
		g_Game->push_message(message, 0);
        init(false);
		return;
	}
	/*
	*	so the girl lost: take away her money now
	*/
	g_Game->gold().misc_credit(girl->m_Money);
	girl->m_Money = 0;
	/*
	*	she isn't going to like this much
	*	but it's not so bad for slaves
	*/
	if (girl->is_slave())
	{
		girl->confidence(-1);
		girl->obedience(5);
		girl->spirit(-2);
		girl->pchate(5);
		girl->pclove(-5);
		girl->pcfear(5);
		girl->happiness(-20);
	}
	else
	{
		girl->confidence(-5);
		girl->obedience(5);
		girl->spirit(-10);
		girl->pchate(30);
		girl->pclove(-30);
		girl->pcfear(10);
		girl->happiness(-50);
	}
	/*
	*	and queue the message again
	*/
	g_Game->push_message(message, 0);
    init(false);
	return;
}

void cScreenGirlDetails::OnKeyPress(SDL_keysym keysym)
{
    auto key = keysym.sym;

    if (key == SDLK_h || key == SDLK_j)
    {
        int mod = (key == SDLK_h ? -1 : 1);
        m_SelectedGirl->house(mod);
        SliderValue(houseperc_id, m_SelectedGirl->house());
        ss.str(""); ss << "House Percentage: " << m_SelectedGirl->house() << "%";
        EditTextItem(ss.str(), housepercval_id);
        // Rebelliousness might have changed, so update details
        if (DetailLevel == 0)
        {
            string detail = cGirls::GetDetailsString(GetGraphics(), m_SelectedGirl);
            EditTextItem(detail, girldesc_id);
        }
    }
    if (key == SDLK_s)
    {
        if (keysym.mod & KMOD_SHIFT)
        {
            DetailLevel = 2;
            EditTextItem(cGirls::GetThirdDetailsString(m_SelectedGirl), girldesc_id);
        }
        else
        {
            if (DetailLevel == 0)		{ DetailLevel = 1; EditTextItem(cGirls::GetMoreDetailsString(GetGraphics(), m_SelectedGirl), girldesc_id); }
            else						{ DetailLevel = 0; EditTextItem(cGirls::GetDetailsString(GetGraphics(), m_SelectedGirl), girldesc_id); }
        }
    }

}

void cScreenGirlDetails::process()
{
    if(m_Refresh) {
        init(false);
    }
}
