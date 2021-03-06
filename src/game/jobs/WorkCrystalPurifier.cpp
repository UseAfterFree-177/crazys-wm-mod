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
#include "buildings/cMovieStudio.h"
#include "buildings/queries.h"
#include "cRng.h"
#include <sstream>
#include "cGirls.h"

// `J` Job Movie Studio - Crew
bool WorkCrystalPurifier(sGirl& girl, bool Day0Night1, cRng& rng)
{
    auto brothel = dynamic_cast<sMovieStudio*>(girl.m_Building);

    Action_Types actiontype = ACTION_WORKMOVIE;
    bool SkipDisobey = false;
    std::stringstream ss;

    // No film crew.. then go home    // `J` this will be taken care of in building flow, leaving it in for now
    if (brothel->num_girls_on_job(JOB_CAMERAMAGE, SHIFT_NIGHT) == 0 || brothel->num_girls_on_job(JOB_CRYSTALPURIFIER, SHIFT_NIGHT) == 0)
    {
        ss << "There was no crew to film the scene, so ${name} took the day off";
        girl.AddMessage(ss.str(), IMGTYPE_PROFILE, EVENT_NOWORK);
        return false;    // not refusing
    }
    else if (Num_Actress(*brothel) < 1)
    {
        ss << "There were no actresses to film, so ${name} took the day off";
        girl.AddMessage(ss.str(), IMGTYPE_PROFILE, EVENT_NOWORK);
        return false;    // not refusing
    }

    // `J` added this to allow the Director to assign someone to this job without making it permanent
    if (girl.m_DayJob == JOB_RESTING)    // the director sets the old job to dayjob when changing night job
    {
        ss << "${name} was assigned to work as a crystal purifier";
    }
    else
    {
        ss << "The Director assigned ${name}to edit the scenes for the week";
    }


    cGirls::UnequipCombat(girl);    // not for studio crew

    int roll = rng.d100();
    if (!SkipDisobey)    // `J` skip the disobey check because it has already been done in the building flow
    {
        if (roll <= 10 && girl.disobey_check(actiontype, JOB_CRYSTALPURIFIER))
        {
            if (girl.m_DayJob == JOB_RESTING)
            {
                ss << " but she refused to work.";
                girl.AddMessage(ss.str(), IMGTYPE_PROFILE, EVENT_NOWORK);
            }
            else
            {
                ss << " but ${name}";
                if (is_Actress_Job(girl.m_DayJob))    ss << " wanted to be part of the action instead of just watching it.\n";
                else if (girl.m_DayJob == JOB_PROMOTER)        ss << " preferred to sell the movies rather than edit them.\n";
                else if (girl.m_DayJob == JOB_FLUFFER)            ss << " wanted to see the action live instead of watching it afterwards.\n";
                else if (girl.m_DayJob == JOB_STAGEHAND)        ss << " wanted to clean instead of editing scenes.\n";
                else if (girl.m_DayJob == JOB_CAMERAMAGE)        ss << " preferred to film the scenes rather than edit them.\n";
                girl.AddMessage(ss.str(), IMGTYPE_PROFILE, EVENT_BACKTOWORK);
            }
            return true;
        }
    }
    ss << ".\n \n";

    int wages = 50;
    int tips = 0;
    int enjoy = 0;

    if (roll <= 10)
    {
        enjoy -= rng % 3 + 1;
        ss << "She did not like working in the studio today.\n \n";
    }
    else if (roll >= 90)
    {
        enjoy += rng % 3 + 1;
        ss << "She had a great time working today.\n \n";
    }
    else
    {
        enjoy += rng % 2;
        ss << "Otherwise, the shift passed uneventfully.\n \n";
    }
    double jobperformance = girl.job_performance(JOB_CRYSTALPURIFIER, false);
    jobperformance += enjoy * 2;

    // slave girls not being paid for a job that normally you would pay directly for do less work
    if (girl.is_unpaid())
    {
        jobperformance *= 0.9;
        wages = 0;
    }
    else    // work out the pay between the house and the girl
    {
        // `J` zzzzzz - need to change pay so it better reflects how well she edited the films
        wages += 20;
        int roll_max = girl.spirit() + girl.intelligence();
        roll_max /= 4;
        wages += 10 + rng%roll_max;
    }

    /* */if (jobperformance > 0)    ss << "She helped improve the scene " << (int)jobperformance << "% with her production skills. \n";
    else if (jobperformance < 0)    ss << "She did a bad job today, she reduced the scene quality " << (int)jobperformance << "% with her poor performance. \n";
    else /*                   */    ss << "She did not really help the scene quality.\n";

    girl.AddMessage(ss.str(), IMGTYPE_PROFILE, Day0Night1 ? EVENT_NIGHTSHIFT : EVENT_DAYSHIFT);
    brothel->m_PurifierQaulity += (int)jobperformance;
    girl.m_Tips = std::max(0, tips);
    girl.m_Pay = std::max(0, wages);

    // Improve stats
    int xp = 5, skill = 3;
    if (jobperformance > 5)    skill += 1;

    if (girl.has_active_trait("Quick Learner"))        { skill += 1; xp += 3; }
    else if (girl.has_active_trait("Slow Learner"))    { skill -= 1; xp -= 3; }

    if (rng % 2 == 1)
        girl.intelligence(rng%skill);
    girl.service(rng%skill + 1);
    girl.exp(xp);

    return false;
}

double JP_CrystalPurifier(const sGirl& girl, bool estimate)// not used
{
    double jobperformance = 0.0;
    if (estimate)// for third detail string
    {
        jobperformance +=
            (girl.intelligence() / 2) +
            (girl.spirit() / 2) +
            (girl.fame() / 2) +
            (girl.service() / 2) +
            girl.level();

    }
    else// for the actual check
    {
        int t = girl.tiredness() - 80;
        if (t > 0)
            jobperformance -= t / 3;

        jobperformance += (girl.spirit() - 50) / 10;
        jobperformance += (girl.intelligence() - 50) / 10;
        jobperformance += girl.service() / 10;
        jobperformance /= 3;
        jobperformance += girl.level();
        jobperformance += girl.fame() / 10;
        jobperformance += g_Dice % 4 - 1;    // should add a -1 to +3 random element --PP
    }

    jobperformance += girl.get_trait_modifier("work.crystalpurifier");

    return jobperformance;
}
