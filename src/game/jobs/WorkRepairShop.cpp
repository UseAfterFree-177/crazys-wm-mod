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
#include "buildings/cBuildingManager.h"
#include "cRng.h"
#include <sstream>
#include "cJobManager.h"
#include "cGirls.h"

// `J` Job Clinic - Surgery
bool WorkRepairShop(sGirl& girl, bool Day0Night1, cRng& rng)
{
    auto brothel = girl.m_Building;

    std::stringstream ss;
    cGirls::UnequipCombat(girl);    // not for patients

    if (!girl.has_active_trait("Construct") && !girl.has_active_trait("Half-Construct"))
    {
        ss << "${name} has no artificial parts so she was sent to the Healing center.";
        if (Day0Night1 == SHIFT_DAY) girl.AddMessage(ss.str(), IMGTYPE_PROFILE, EVENT_WARNING);
        if (girl.m_DayJob == JOB_GETREPAIRS)    girl.m_DayJob = JOB_GETHEALING;
        if (girl.m_NightJob == JOB_GETREPAIRS)    girl.m_NightJob = JOB_GETHEALING;
        return false;    // not refusing
    }

    int nummecs = brothel->num_girls_on_job(JOB_MECHANIC, Day0Night1);
    int numnurse = brothel->num_girls_on_job(JOB_NURSE, Day0Night1);

    // `J` base recovery copied freetime recovery
    int health = 10 + (girl.constitution() / 10);
    int tiredness = 10 + rng % 21;    // build up as positive then apply as negative
    int happy = 10 + rng % 11;
    int mana = 5 + (girl.magic() / 5);
    int libido = (girl.has_active_trait("Nymphomaniac") ? 14 : 4);

    if (nummecs + numnurse < 1)
    {
        ss << "You don't have any Mechanics or Nurses on duty so ${name} just rests in a hospital bed.";
    }
    else
    {
        ss << "${name} does nothing while ";
    }
    if (nummecs > 0)
    {
        ss << "the Mechanic" << (nummecs > 1 ? "s" : "");
        if (girl.has_active_trait("Half-Construct") && girl.m_DayJob == JOB_GETREPAIRS && girl.m_NightJob == JOB_GETREPAIRS)
        {    // if fixed by Mechanic for both shifts.
            health += 20;    // Total 40 healing per day, heals less because Mechanic does not fix living tissue.
        }
        else
        {
            health += 30;
        }
    }
    if (nummecs > 0 && numnurse > 0) { ss << " and"; }
    if (numnurse > 0)
    {
        ss << " the Nurse" << (numnurse > 1 ? "s" : "");
        health += 10;
        tiredness += 10;
        happy += 10;
        mana += (girl.magic() / 5);
    }
    if (nummecs + numnurse >= 4 && rng.percent(50))    // lots of people making sure she is in good working order
    {
        girl.constitution(1);
    }
    ss << (((nummecs > 1 && numnurse < 1) || numnurse > 1) ? " take" : " takes") << " care of her.";

    // Improve girl
    if (girl.has_active_trait("Lesbian")) libido += numnurse;
    if (girl.has_active_trait("Masochist")) libido += 1;

    girl.upd_base_stat(STAT_HEALTH, health, false);
    girl.upd_base_stat(STAT_TIREDNESS, -tiredness, false);
    girl.happiness(happy);
    girl.mana(mana);
    girl.upd_temp_stat(STAT_LIBIDO, libido);
    if (rng % 10 == 0) girl.medicine(1);    // `J` she watched what the doctors and nurses were doing

    girl.exp(1);   // Just because!

    // send her to the waiting room when she is healthy
    if (girl.health() > 90 && girl.tiredness() < 10)
    {
        if (nummecs + numnurse < 1)    ss << "\n \nShe wanders out of the Clinic when she is feeling better.";
        else                        ss << "\n \nShe has been released from the Clinic.";
        if (girl.m_DayJob == JOB_GETHEALING)      girl.m_DayJob = JOB_RESTING;
        if (girl.m_NightJob == JOB_GETHEALING)    girl.m_NightJob = JOB_RESTING;
        if (girl.m_DayJob == JOB_GETREPAIRS)      girl.m_DayJob = JOB_RESTING;
        if (girl.m_NightJob == JOB_GETREPAIRS)    girl.m_NightJob = JOB_RESTING;
    }

    girl.AddMessage(ss.str(), IMGTYPE_PROFILE, Day0Night1 ? EVENT_NIGHTSHIFT : EVENT_DAYSHIFT);

    return false;
}

double JP_RepairShop(const sGirl& girl, bool estimate)
{
    if (!girl.has_active_trait("Construct") && !girl.has_active_trait("Half-Construct")) return -1000;
    double jobperformance = 0.0;
    jobperformance += (100 - girl.health());
    jobperformance += (100 - girl.happiness());
    jobperformance += girl.tiredness();
    return jobperformance;
}
