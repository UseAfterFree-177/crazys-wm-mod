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

#ifndef WM_JOBS_CRAFTING_H
#define WM_JOBS_CRAFTING_H

#include "GenericJob.h"
#include <sstream>
#include <vector>


struct sCraftingJobData {
    Image_Types  Image;             //!< The image type used in the job event.
    Action_Types Action;            //!< Secondary action (besides WORKMOVIE) that can gain enjoyment. Set to NUM_ACTIONTYPES to disable
    const char*  TraitMod;          //!< Name of the trait modifier
    std::vector<StatSkill> PrimaryGains;    // primary skill and stat gains
    std::vector<StatSkill> SecondaryGains;  // primary skill and stat gains
    int          Wages;             //!< Default wages for that job
    const char*  MsgWork;           //!< Default message for doing the work
    const char*  MsgRepair;         //!< Message for doing repairs
    const char*  MsgProduce;        //!< Message for production
    std::vector<sTraitChange> TraitChanges;
};

class GenericCraftingJob : public cBasicJob {
public:
    explicit GenericCraftingJob(JOBS id, sCraftingJobData data) : cBasicJob(id), m_CraftingData(std::move(data)) {
        set_performance_data(m_CraftingData.TraitMod, m_CraftingData.PrimaryGains, m_CraftingData.SecondaryGains);
    }

    bool DoWork(sGirl& girl, bool is_night) final;
protected:
    bool WorkCrafting(sGirl& girl, bool is_night);

    // common data
    sCraftingJobData m_CraftingData;

    // shift processing data
    int enjoy;
    int jobperformance;
    int craftpoints;
private:
    virtual void performance_msg();
    virtual void DoWorkEvents(sGirl& girl);
};

#endif //WM_JOBS_CRAFTING_H
