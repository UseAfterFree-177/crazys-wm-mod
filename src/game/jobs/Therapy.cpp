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

#include "GenericJob.h"
#include "cJobManager.h"
#include "cGirls.h"
#include "character/sGirl.hpp"
#include "IBuilding.hpp"
#include <sstream>
#include "character/predicates.h"
#include "Game.hpp"
#include "cGirlGangFight.h"
#include "buildings/cBuildingManager.h"

struct sRemoveTrait {
    const char* Trait;
    const char* Message;
};

struct sTherapyData {
    const char*  TherapyMessage;    //!< The message used to indicate that the surgery was performed
    const char*  NoNeedMessage;     //!< The message sent when no therapy is needed
    const char*  ContinueMessage;   //!< The message sent when further therapy is needed
    const char*  DeathMessage;      //!< The message if she dies
    const char*  NoCounselorMessage;//!< The message if there is no counselor
    const char*  ReleaseMessage;    //!< The message when the therapy is finished
    const char*  TreatmentName;     //!< Therapy or Rehab
    int          Duration;          //!< How many days until finished.
    int          BasicFightChance;  //!< Chance to fight with the counselor
    int          SuccessBonus;      //!< Bonus for enjoyment and happiness once therapy was a success
    int          HealthDanger;      //!< How much health does she lose TODO does it make sense that she loses health in therapy but not without therapy???
    std::vector<sRemoveTrait> TraitRemove;   //!< The traits that this therapy can remove
};


class TherapyJob : public IGenericJob {
public:
    explicit TherapyJob(JOBS id, sTherapyData data) : IGenericJob(id), m_TherapyData(data) {}

    bool DoWork(sGirl& girl, bool is_night) final;
protected:
    // common data
    sTherapyData m_TherapyData;

    virtual void FightEvent(sGirl& girl, bool is_night);
    virtual void OnFinish(sGirl& girl) {}
    bool needs_therapy(const sGirl& girl) const;
    double GetPerformance(const sGirl& girl, bool estimate) const override;
};

bool TherapyJob::DoWork(sGirl& girl, bool is_night) {
    auto brothel = girl.m_Building;
#pragma region //    Job setup                //
    Action_Types actiontype = ACTION_WORKTHERAPY;
    // if she was not in thearpy yesterday, reset working days to 0 before proceding
    if (girl.m_YesterDayJob != job()) { girl.m_WorkingDay = girl.m_PrevWorkingDay = 0; }
    girl.m_DayJob = girl.m_NightJob = job();    // it is a full time job

    if (!needs_therapy(girl))
    {
        ss << m_TherapyData.NoNeedMessage;
        if (!is_night)    girl.AddMessage(ss.str(), IMGTYPE_PROFILE, EVENT_WARNING);
        girl.FullJobReset(JOB_CENTREREST);
        girl.m_PrevWorkingDay = girl.m_WorkingDay = 0;
        return false; // not refusing
    }
    bool hasCounselor = brothel->num_girls_on_job(JOB_COUNSELOR, is_night) > 0;
    if (!hasCounselor)
    {
        ss << m_TherapyData.NoCounselorMessage;
        girl.AddMessage(ss.str(), IMGTYPE_PROFILE, EVENT_WARNING);
        return false;    // not refusing
    }

    if (chance(m_TherapyData.BasicFightChance) || girl.disobey_check(actiontype, job))    // `J` - yes, OR, not and.
    {
        FightEvent(girl, is_night);
    }
    ss << m_TherapyData.TherapyMessage << "\n \n";

    cGirls::UnequipCombat(&girl);    // not for patient

    int enjoy = 0;
    int msgtype = is_night;

#pragma endregion
#pragma region //    Count the Days                //

    if (!is_night) girl.m_WorkingDay++;

    girl.happiness(uniform(-20, 10));
    girl.spirit(uniform(-5, 5));
    girl.mana(uniform(-5, 5));

    // `J` % chance a counselor will save her if she almost dies
    int healthmod = uniform(-m_TherapyData.HealthDanger, 2);
    if (girl.health() + healthmod < 1 && chance(95 + (girl.health() + healthmod)) &&
        (brothel->num_girls_on_job(JOB_COUNSELOR, true) > 0 || brothel->num_girls_on_job(JOB_COUNSELOR, false) > 0))
    {    // Don't kill the girl from therapy if a Counselor is on duty
        girl.set_stat(STAT_HEALTH, 1);
        girl.pcfear(5);
        girl.pclove(-10);
        girl.pchate(10);
        ss << "She almost died in " << m_TherapyData.TreatmentName << " but the Counselor saved her.\n";
        ss << "She hates you a little more for forcing this on her.\n \n";
        msgtype = EVENT_DANGER;
        enjoy -= 2;
    }
    else
    {
        girl.health(healthmod);
        enjoy += (healthmod / 5) + 1;
    }

    if (girl.health() < 1)
    {
        ss << m_TherapyData.DeathMessage;
        msgtype = EVENT_DANGER;
    }

    if (girl.m_WorkingDay >= m_TherapyData.Duration && is_night)
    {
        enjoy += uniform(0, m_TherapyData.SuccessBonus);
        girl.upd_Enjoyment(ACTION_WORKCOUNSELOR, uniform(-2, 4));    // `J` She may want to help others with their problems
        girl.happiness(uniform(0, m_TherapyData.SuccessBonus));

        ss << "The " << m_TherapyData.TreatmentName << " is a success.\n";
        msgtype = EVENT_GOODNEWS;

        RandomSelector<sRemoveTrait> selector;
        for(auto& t : m_TherapyData.TraitRemove) {
            if(girl.has_active_trait(t.Trait)) {
                selector.process(&t);
            }
        }

        if(auto sel = selector.selection()) {
            if (girl.lose_trait( sel->Trait))
            {
                ss << sel->Message << "\n";
            }
        }

        OnFinish(girl);

        if (needs_therapy(girl))
        {
            ss << "\n" << m_TherapyData.ContinueMessage;
        }
        else // get out of therapy
        {
            ss << "\n" << m_TherapyData.ReleaseMessage;
            girl.FullJobReset(JOB_CENTREREST);
            girl.m_PrevWorkingDay = girl.m_WorkingDay = 0;
        }
    }
    else
    {
        ss << "The " << m_TherapyData.TreatmentName << " is in progress (" << (m_TherapyData.Duration - girl.m_WorkingDay) << " day remaining).";
    }

#pragma endregion
#pragma region    //    Finish the shift            //

    // Improve girl
    girl.AddMessage(ss.str(), IMGTYPE_PROFILE, msgtype);
    girl.upd_Enjoyment(actiontype, enjoy);

#pragma endregion
    return false;
}

bool TherapyJob::needs_therapy(const sGirl& girl) const {
    return std::any_of(begin(m_TherapyData.TraitRemove), end(m_TherapyData.TraitRemove),
     [&](const sRemoveTrait& t){
         return girl.has_active_trait(t.Trait);
     });
}

void TherapyJob::FightEvent(sGirl& girl, bool is_night) {
    ss << " fought with her counselor and did not make any progress this week.";
    girl.AddMessage(ss.str(), IMGTYPE_PROFILE, EVENT_NOWORK);
    girl.upd_Enjoyment(ACTION_WORKTHERAPY, -1);
    if (is_night) girl.m_WorkingDay--;
}

double TherapyJob::GetPerformance(const sGirl& girl, bool estimate) const {
    if(!needs_therapy(girl)) return -1000;
    double p = 100;
    for(auto& t : m_TherapyData.TraitRemove) {
        if(girl.has_active_trait(t.Trait)) p += 100;
    }
    return p;
}


struct AngerManagement : public TherapyJob {
    using TherapyJob::TherapyJob;
    void FightEvent(sGirl& girl, bool is_night) override;
};

void AngerManagement::FightEvent(sGirl& girl, bool is_night) {
    girl.upd_Enjoyment(ACTION_WORKTHERAPY, -1);
    if (is_night) girl.m_WorkingDay--;
    if (chance(10))
    {
        girl.upd_Enjoyment(ACTION_WORKTHERAPY, -5);
        bool runaway = false;
        // if there is no counselor, it should not get to here
        sGirl* counselor = random_girl_on_job(*girl.m_Building, JOB_COUNSELOR, is_night);
        ss << "\n \n${name} fought hard with her counselor " << counselor->FullName();
        /// TODO Gangs and Security
        auto winner = GirlFightsGirl(*counselor, girl);
        if (winner != EFightResult::VICTORY)    // the patient won
        {
            ss << " and won.\n \n";
            girl.upd_Enjoyment(ACTION_COMBAT, 5);
            counselor->upd_Enjoyment(ACTION_WORKCOUNSELOR, -5);
            counselor->upd_Enjoyment(ACTION_COMBAT, -2);

            if (chance(10))    // and ran away
            {
                runaway = true;
                stringstream smess;
                smess << girl.FullName() << " fought with her counselor and ran away.\nSend your goons after her to attempt recapture.\nShe will escape for good after 6 weeks.\n";
                g_Game->push_message(smess.str(), COLOR_RED);
                girl.run_away();
                return;
            }
        }
        else    // the counselor won
        {
            ss << " and lost.\n \n";
            girl.upd_Enjoyment(ACTION_WORKTHERAPY, -5);
            girl.upd_Enjoyment(ACTION_COMBAT, -5);
            counselor->upd_Enjoyment(ACTION_WORKCOUNSELOR, -1);
            counselor->upd_Enjoyment(ACTION_COMBAT, 2);
        }
        stringstream ssc;
        ssc << "${name} had to defend herself from " << girl.FullName() << " who she was counseling.\n";
        if (runaway) ss << "${name} ran out of the Counceling Centre and has not been heard from since.";
        counselor->AddMessage(ssc.str(), IMGTYPE_COMBAT, EVENT_WARNING);
    }
    else
    {
        ss << " fought with her counselor and did not make any progress this week.";
    }
    girl.AddMessage(ss.str(), IMGTYPE_PROFILE, EVENT_NOWORK);
}

struct Rehab : public TherapyJob {
    using TherapyJob::TherapyJob;
    void OnFinish(sGirl& girl) override;
};

void Rehab::OnFinish(sGirl& girl) {
    girl.add_temporary_trait("Former Addict", 40);
}

std::vector<std::unique_ptr<IGenericJob>> GetTherapyJobs() {
    std::vector<std::unique_ptr<IGenericJob>> jobs;
    jobs.push_back(std::make_unique<AngerManagement>(JOB_ANGER, sTherapyData{
        "${name} underwent therapy for anger issues.",
        "${name} doesn't need anger management so she was sent to the waiting room.",
        "She should stay in anger management to treat her other anger issues.",
        "She died in anger management.",
        "$[name} has no counselor to help her on the ${shift} Shift.",
        "She has been released from therapy.",
        "therapy",
        3, 20, 10, 10,
        {{"Aggressive", "She is no longer Aggressive."},
         {"Tsundere", "She is no longer a Tsundere."},
         {"Yandere", "She is no longer a Yandere."}}}));
    jobs.push_back(std::make_unique<TherapyJob>(JOB_EXTHERAPY, sTherapyData{
        "${name} underwent therapy for extreme mental issues.",
        "${name} doesn't need extreme therapy for anything so she was sent to the waiting room.",
        "She should stay in extreme therapy to treat her other disorders.",
        "She died in therapy.",
        "$[name} has no counselor to help her on the ${shift} Shift.",
        "She has been released from therapy.",
        "therapy",
        3, 5, 5, 4,
        {{"Mind Fucked", "She is no longer mind fucked."},
        {"Broken Will", "She is no longer has a broken will."}}
    }));
    jobs.push_back(std::make_unique<TherapyJob>(JOB_THERAPY, sTherapyData{
        "${name} underwent therapy for mental issues.",
        "${name} doesn't need therapy for anything so she was sent to the waiting room.",
        "She should stay in therapy to treat her other disorders.",
        "She died in therapy.",
        "$[name} has no counselor to help her on the ${shift} Shift.",
        "She has been released from therapy.",
        "therapy",
        3, 10, 5, 4,
        {{"Nervous", "She is no longer nervous all the time."},
         {"Dependant", "She is no longer Dependant on others."},
         {"Pessimist", "She is no longer a Pessimist about everything."}}
    }));

    jobs.push_back(std::make_unique<TherapyJob>(JOB_THERAPY, sTherapyData{
        "${name} underwent rehab for her addiction.",
        "${name} is not addicted to anything so she was sent to the waiting room.",
        "She should stay in rehab to treat her other addictions.",
        "${name} died in rehab.",
        "${name} sits in rehab doing nothing. You must assign a counselor to treat her.",
        "She has been released from rehab.",
        "rehab",
        3, 50, 10, 10,
        {{"Smoker", "She is no longer a smoker."},
         {"Cum Addict", "She is no longer a cum addict."},
         {"Alcoholic", "She is no longer an alcoholic."},
         {"Fairy Dust Addict", "She is no longer a fairy dust addict."},
         {"Shroud Addict", "She is no longer a shroud addict."},
         {"Viras Blood Addict", "She is no longer a viras blood addict."}
        }
    }));

    return std::move(jobs);
}
