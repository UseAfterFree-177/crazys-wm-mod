/*
* Copyright 2009, 2010, The Pink Petal Development Team.
* The Pink Petal Devloment Team are defined as the game's coders
* who meet on http://pinkpetal.org     // old site: http://pinkpetal .co.cc
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
#include "Films.h"
#include "Game.hpp"
#include "character/cPlayer.h"

FilmBuk::FilmBuk() : GenericFilmJob(JOB_FILMBUKKAKE, {
        IMGTYPE_ORAL, Action_Types::NUM_ACTIONTYPES, SKILL_BDSM, 50, 5,
        FilmJobData::EVIL, SKILL_GROUP, "Bukkake",
        " was filmed being splattered with cum in bukkake scenes.",
        nullptr, {
            {true, "Masochist", 75, ACTION_SEX, "${name} has turned into a Masochist from all the degradation."},
            {true, "Cum Addict", 80, ACTION_WORKMOVIE, "${name} has become addicted to cum."}}
}) {
    set_performance_data("work.film.buk", {STAT_CHARISMA, STAT_BEAUTY}, {});
}

//Useful fn
void AndAction(stringstream *, const string&, bool);

void FilmBuk::DoScene(sGirl& girl) {
	//Lights, camera...
	AndAction(&ss, girl.FullName(), tied);

	PrintPerfSceneEval();
	ss << "\n";


	//Enjoyed? If she's deranged, she'd should have enjoyed it.
	if (girl.has_active_trait("Mind Fucked"))
	{
		result.enjoy += 16;
		ss << "Being completely mind fucked, ${name} really gets off on the depravity.\n";
	}
	else if (girl.has_active_trait("Masochist"))
	{
		result.enjoy += 13;
		ss << "${name} enjoys this. She knows it's what she deserves.\n";
	}
	else if (girl.has_active_trait("Cum Addict"))
	{
		result.enjoy += 13;
		ss << "${name} enjoys this, and spends a while licking cum off her body.\n";
	}
	else if (girl.has_active_trait("Broken Will") || girl.has_active_trait("Dependant"))
	{
		result.enjoy += 11;
		ss << "${name} accepts this. It is Master's will.\n";
	}
	else if (girl.has_active_trait("Iron Will") || girl.has_active_trait("Fearless"))
	{
		result.enjoy -= 5;
		ss << "${name} endures in stoic silence, determined not to let you see her suffer.\n";
	}

	//For final calc
	result.bonus += result.enjoy;

	// now we've got that done, most girls shouldn't like this...
	result.enjoy -= 10;
}

void FilmBuk::GainTraits(sGirl& girl, int performance) const {
    if (tied) {
        cGirls::PossiblyGainNewTrait(&girl, "Mind Fucked", 75, ACTION_SEX,
                                     "${name} has become Mind Fucked from the forced degradation.", false);
        cGirls::PossiblyLoseExistingTrait(&girl, "Iron Will", 80, ACTION_SEX,
                                          "${name}'s unwilling degradation has shattered her iron will.", false);
    }
}

bool FilmBuk::CheckRefuseWork(sGirl& girl) {
    int roll = d100();
    if (girl.has_active_trait("Cum Addict"))
    {
        ss << "Cum-craving ${name} couldn't wait to get sticky in this bukkake scene, and was sucking guys off before the lighting was even set up.";
        result.bonus += 10;
    }
    else if (roll <= 10 && !girl.has_active_trait("Mind Fucked") && girl.disobey_check(ACTION_WORKMOVIE, JOB_FILMBUKKAKE))
    {
        ss << "${name} refused to have any part in this";
        if (girl.is_slave())
        {
            if (g_Game->player().disposition() > 30)  // nice
            {
                ss << " \"filthy\" bukkake scene.\nShe was clearly upset so you allowed her the day off.";
                girl.pclove(2);
                girl.pchate(-1);
                girl.AddMessage(ss.str(), IMGTYPE_PROFILE, EVENT_NOWORK);
                return true;
            }
            else if (g_Game->player().disposition() > -30) //pragmatic
            {
                ss << " \"filthy\" bukkake scene.\nShe was clearly upset so you had your men drug her and tie her down for action.";
                girl.pclove(-1);
                girl.pchate(1);
                girl.pcfear(+1);
                g_Game->player().disposition(-1);
                tied = true;
                result.enjoy -= 2;
            }
            else if (g_Game->player().disposition() > -30)
            {
                ss << " \"filthy\" bukkake scene.\nShe was clearly upset so you had your men whip some sense into her before the scene and tie her down for action.";
                girl.pclove(-2);
                girl.pchate(+2);
                girl.pcfear(+4);
                g_Game->player().disposition(-2);
                result.enjoy -= 6;
                tied = true;
            }
        }
        else // not a slave
        {
            ss << " \"disgusting\" bukkake scene and left the set.";
            girl.AddMessage(ss.str(), IMGTYPE_PROFILE, EVENT_NOWORK);
            return true;
        }
    }
    else ss << girl.FullName() << m_FilmData.MsgWork << "\n \n";
    return false;
}

void FilmBuk::Reset() {
    GenericFilmJob::Reset();
    tied = false;
}

void AndAction(stringstream *TheAction, const string& TheHo, bool TiedUp)
{
	*TheAction << TheHo << " was ";
	*TheAction << g_Dice.select_text({ "dutifully gangbanged", "deeply probed", "effectively raped",
                                    "uncomfortably filled", "clumsily penetrated", "roughly used"});
	*TheAction << " by ";
	*TheAction << g_Dice.select_text({"40-year-old virgins", "a large gang of men", "unenthusiastic gay men",
                                      "straight-jacketed psychopaths", "real-life ogres", "drunken soldiers"});
	*TheAction << " who ";
    *TheAction << g_Dice.select_text({"vigorously", "stoutly", "casually", "lugubriously", "excitedly",
                                      "deliberately", "ludicrously"});
	*TheAction << " ";
    *TheAction << g_Dice.select_text({"ejaculated their seed", "splooged", "fired hot cum", "shot semen", "nutted",
                                      "came"});
	if (TiedUp)
	{
        *TheAction << g_Dice.select_text({
            " all over her tied, trembling body.",
            " into her pried-open mouth.",
            " on her face, breasts and inside her spread-eagle cunt.",
            " over her bound, naked body until she was nicely 'glazed'.",
            " in her tied down, squirming face.",
            " into her speculum-spread vagina.",
            " in her eyes and up her nose."
        });
	}
	else {
        *TheAction << g_Dice.select_text({
            " all over her naked body.",
            " into her waiting mouth.",
            " simultaneously in her ass, mouth and cunt.",
            " over her until she was nicely 'glazed'.",
            " in her hair and on her face.",
            " over her stomach and breasts.",
            " all in her eyes and up her nose."
        });
    }
	*TheAction << "\n \n";
}
