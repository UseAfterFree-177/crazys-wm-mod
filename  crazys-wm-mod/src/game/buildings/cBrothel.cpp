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

#include <sstream>
#include <algorithm>
#include "character/cCustomers.h"
#include "CLog.h"
#include "xml/util.h"

#include "buildings/cBrothel.h"
#include "cGangs.h"
#include "XmlMisc.h"
#include "cMovieStudio.h"
#include "cArena.h"
#include "cCentre.h"
#include "cClinic.h"
#include "cHouse.h"
#include "cFarm.h"
#include "cObjectiveManager.hpp"
#include "Game.hpp"
#include "cJobManager.h"
#include "cTariff.h"

// fwd-declare the relevant settings
namespace settings {
    extern const char* PREG_COOL_DOWN;
    extern const char* USER_ITEMS_AUTO_USE;
}

extern cRng					g_Dice;

// // ----- Strut sBrothel Create / destroy
sBrothel::sBrothel() :
    IBuilding(BuildingType::BROTHEL, "Brothel")
{
	m_HasGambStaff = m_HasBarStaff = m_Bar = m_GamblingHall = 0;

	m_TotalCustomers = m_RejectCustomersRestrict = m_RejectCustomersDisease = m_MiscCustomers = 0;

	//movie
	m_ShowTime = m_ShowQuality = 0;

    m_RestJob = JOB_RESTING;
    m_FirstJob = JOB_RESTING;
    m_LastJob = JOB_WHORESTREETS;
}

sBrothel::~sBrothel()			// destructor
{
}


bool sBrothel::LoadXML(tinyxml2::XMLElement * pBrothel)
{
    /// TODO why is this unused?
	// no need to init this, we just created it
	int tempInt = 0;
	// g_LogFile.write("Loading brothel");

	// load variables for sex restrictions
	load_settings_xml(*pBrothel);

	pBrothel->QueryIntAttribute("Bar", &tempInt); m_Bar = tempInt; tempInt = 0;
	pBrothel->QueryIntAttribute("GamblingHall", &tempInt); m_GamblingHall = tempInt; tempInt = 0;
	pBrothel->QueryIntAttribute("HasBarStaff", &tempInt); m_HasBarStaff = tempInt; tempInt = 0;
	pBrothel->QueryIntAttribute("HasGambStaff", &tempInt); m_HasGambStaff = tempInt; tempInt = 0;
	pBrothel->QueryIntAttribute("ShowQuality", &m_ShowQuality);
	pBrothel->QueryIntAttribute("ShowTime", &m_ShowTime);

	m_Finance.loadGoldXML(pBrothel->FirstChildElement("Gold"));

    load_girls_xml(*pBrothel);
	return true;
}


void sBrothel::save_additional_xml(tinyxml2::XMLElement& root) const
{
    root.SetAttribute("Bar", m_Bar);
    root.SetAttribute("GamblingHall", m_GamblingHall);
    root.SetAttribute("HasBarStaff", m_HasBarStaff);
    root.SetAttribute("id", m_id);
    root.SetAttribute("HasGambStaff", m_HasGambStaff);
    root.SetAttribute("ShowQuality", m_ShowQuality);
    root.SetAttribute("ShowTime", m_ShowTime);

    m_Finance.saveGoldXML(root);
}

void sBrothel::UpdateGirls(bool is_night)
{
    g_Game->job_manager().do_whorejobs(*this, is_night);
    g_Game->job_manager().do_custjobs(*this, is_night);

    // `J` When modifying Jobs, search for "J-Change-Jobs"  :  found in >> cBrothel.cpp
    bool matron = num_girls_on_job(m_MatronJob, false) >= 1;
    string MatronMsg, MatronWarningMsg;
    stringstream ss;

    string summary, msg, girlName;
    int sum = EVENT_SUMMARY;
    u_int sw = 0, psw = 0;


    /*
    *	handle any girls training during this shift
    */
    g_Game->job_manager().do_training(this, is_night);
    /*
    *	as for the rest of them...
    */
    for(auto& current : girls())
    {
        int totalPay = 0, totalTips = 0, totalGold = 0;
        bool refused = false;
        girlName = current->FullName();
        sum = EVENT_SUMMARY;

        /*
        *		ONCE DAILY processing
        *		at start of Day Shift
        */
        if (is_night == SHIFT_DAY)
        {
            // Back to work
            if (current->m_NightJob == m_RestJob && current->m_DayJob == m_RestJob && current->m_PregCooldown < g_Game->settings().get_integer(settings::PREG_COOL_DOWN) &&
                current->health() >= 80 && current->tiredness() <= 20)
            {
                if ((matron || current->m_PrevDayJob == m_MatronJob)					// do we have a director, or was she the director and made herself rest?
                    && current->m_PrevDayJob != 255 && current->m_PrevNightJob != 255)	// 255 = nothing, in other words no previous job stored
                {
                    g_Game->job_manager().HandleSpecialJobs(current, current->m_PrevDayJob, current->m_DayJob, false);
                    if (current->m_DayJob == current->m_PrevDayJob)  // only update night job if day job passed HandleSpecialJobs
                        current->m_NightJob = current->m_PrevNightJob;
                    else
                        current->m_NightJob = m_RestJob;
                    current->m_PrevDayJob = current->m_PrevNightJob = 255;
                    MatronMsg += "The Matron puts " + girlName + " back to work.\n";
                    current->m_Events.AddMessage(MatronMsg, IMGTYPE_PROFILE, EVENT_BACKTOWORK);
                    MatronMsg = "";
                }
                else
                {
                    MatronWarningMsg += "WARNING " + girlName + " is doing nothing!\n";
                    current->m_Events.AddMessage(MatronWarningMsg, IMGTYPE_PROFILE, EVENT_WARNING);
                    MatronWarningMsg = "";
                }
            }



        }

        /*
        *		EVERY SHIFT processing
        */

        // Sanity check! Don't process dead girls
        if (current->is_dead())
        {
            continue;
        }
        cGirls::UseItems(current);						// Girl uses items she has
        cGirls::CalculateAskPrice(current, true);		// Calculate the girls asking price

        /*
        *		JOB PROCESSING
        */
        u_int sw = current->get_job(is_night);

        // Sanity check! Don't process runaways
        if (sw == JOB_RUNAWAY)		// `J` added for .06.03.00
        {
            continue;
        }

        // do their job
        // advertising and whoring are handled earlier.
        if (sw == JOB_ADVERTISING || sw == JOB_WHOREGAMBHALL || sw == JOB_WHOREBROTHEL ||
            sw == JOB_BARWHORE || sw == JOB_BARMAID || sw == JOB_WAITRESS ||
            sw == JOB_SINGER || sw == JOB_PIANO || sw == JOB_DEALER || sw == JOB_ENTERTAINMENT ||
            sw == JOB_XXXENTERTAINMENT || sw == JOB_SLEAZYBARMAID || sw == JOB_SLEAZYWAITRESS ||
            sw == JOB_BARSTRIPPER || sw == JOB_MASSEUSE || sw == JOB_BROTHELSTRIPPER || sw == JOB_PIANO || sw == JOB_PEEP)
        {
            // these jobs are already done so we skip them
        }
        else {
            refused = g_Game->job_manager().do_job(*current, is_night);
        }

        totalPay += current->m_Pay;
        totalTips += current->m_Tips;
        totalGold += current->m_Pay + current->m_Tips;

        // work out the pay between the house and the girl
        CalculatePay(*current, sw);

        m_Fame += current->fame();

        /*
        *		Summary Messages
        */

        ss.str("");
        /* */if (sw == JOB_RESTING)			summary += girlName + " was resting so made no money.";
        else if (sw == JOB_MATRON && is_night == SHIFT_NIGHT)
            summary += girlName + " continued to help the other girls throughout the night.";

            // `J` temporary -1 until I reflow brothel jobs
        else if (sw == JOB_TRAINING || sw == JOB_ADVERTISING)	sum = -1;
            // WD:	No night shift summary message needed for Torturer job
        else if (sw == JOB_TORTURER && is_night == SHIFT_NIGHT)	sum = -1;

        // TODO this seems to contain some copy/paste form IBuilding::CalculatePlay
            // `J` if a slave does a job that is normally paid by you but you don't pay your slaves...
        else if (current->is_unpaid() && g_Game->job_manager().is_job_Paid_Player(sw))
        {
            summary += "\nYou own her and you don't pay your slaves.";
        }
            // WD:	Bad girl did not work. Moved from cJobManager::Preprocessing()
        else if (refused) summary += girlName + " refused to work so made no money.";
        else if (totalGold > 0)
        {
            ss << girlName << " earned a total of " << totalGold << " gold";
            u_int job = (is_night ? current->m_NightJob : current->m_DayJob);
            // if it is a player paid job and you pay her
            if ((g_Game->job_manager().is_job_Paid_Player(job) && !current->is_unpaid()))
                ss << " directly from you. She gets to keep it all.";
            else if (current->house() <= 0)				ss << " and she gets to keep it all.";
            else if (totalTips>0 && current->keep_tips())
            {
                int hpay = int(double(totalPay * double(current->house() * 0.01)));
                int gpay = totalPay - hpay;
                ss << ".\nShe keeps the " << totalTips << " she got in tips and her cut (" << 100 - current->house() << "%) of the payment amounting to " << gpay << " gold.\n \nYou got " << hpay << " gold (" << current->house() << "%).";
            }
            else
            {
                int hpay = int(double(totalGold * double(current->house() * 0.01)));
                int gpay = totalGold - hpay;
                ss << ".\nShe keeps " << gpay << " gold. (" << 100 - current->house() << "%)\nYou keep " << hpay << " gold (" << current->house() << "%).";
            }
            summary += ss.str();
        }

        else if (totalGold == 0)		summary += girlName + " made no money.";
        else if (totalGold < 0)
        {
            ss.str("");
            ss << "ERROR: She has a loss of " << totalGold << " gold\n \nPlease report this to the Pink Petal Devloment Team at http://pinkpetal.org\n \nGirl Name: " << current->FullName()
               << "\nJob: " << g_Game->job_manager().JobData[(is_night ? current->m_NightJob : current->m_DayJob)].name << "\nPay:     " << current->m_Pay << "\nTips:   " << current->m_Tips << "\nTotal: " << totalGold;
            summary += ss.str();
            sum = EVENT_DEBUG;
        }
        if (sum >= 0)	// `J` temporary -1 not to show until I reflow brothel jobs
            current->m_Events.AddMessage(summary, IMGTYPE_PROFILE, sum);

        summary = "";



        // Runaway, Depression & Drug checking
        if (runaway_check(current))
        {
            remove_girl(current);
            current->run_away();
            continue;
        }

        /*
        *		MATRON CODE START
        */

        // Lets try to compact multiple messages into one.
        MatronMsg = "";
        MatronWarningMsg = "";

        bool matron = (num_girls_on_job(JOB_MATRON, true) >= 1 || num_girls_on_job(JOB_MATRON, false) >= 1);

        if (current->tiredness() > 80)
        {
            if (matron)
            {
                if (current->m_PrevNightJob == 255 && current->m_PrevDayJob == 255)
                {
                    current->m_PrevDayJob = current->m_DayJob;
                    current->m_PrevNightJob = current->m_NightJob;
                    current->m_DayJob = current->m_NightJob = JOB_RESTING;
                    MatronWarningMsg += "Your matron takes " + girlName + " off duty to rest due to her tiredness.\n";
                }
                else
                {
                    if (g_Dice.percent(70))
                    {
                        MatronMsg += "Your matron helps " + girlName + " to relax.\n";
                        current->tiredness(-5);
                    }
                }
            }
            else
                MatronWarningMsg += "CAUTION! This girl desperately need rest. Give her some free time\n";
        }

        if (current->happiness() < 40 && matron && g_Dice.percent(70))
        {
            MatronMsg = "Your matron helps cheer up " + girlName + " after she feels sad.\n";
            current->happiness(5);
        }

        if (current->health() < 40)
        {
            if (matron)
            {
                if (current->m_PrevNightJob == 255 && current->m_PrevDayJob == 255)
                {
                    current->m_PrevDayJob = current->m_DayJob;
                    current->m_PrevNightJob = current->m_NightJob;
                    current->m_DayJob = current->m_NightJob = JOB_RESTING;
                    MatronWarningMsg += girlName + " is taken off duty by your matron to rest due to her low health.\n";
                }
                else
                {
                    MatronMsg = "Your matron helps heal " + girlName + ".\n";
                    current->health(5);
                }
            }
            else
            {
                MatronWarningMsg = "DANGER " + girlName + "'s health is very low!\nShe must rest or she will die!\n";
            }
        }

        if (!MatronMsg.empty())
        {
            current->m_Events.AddMessage(MatronMsg, IMGTYPE_PROFILE, is_night);
            MatronMsg = "";
        }

        if (!MatronWarningMsg.empty())
        {
            current->m_Events.AddMessage(MatronWarningMsg, IMGTYPE_PROFILE, EVENT_WARNING);
            MatronWarningMsg = "";
        }
        /*
        *		MATRON CODE END
        */

        // Do item check at the end of the day
        if (is_night == SHIFT_NIGHT)
        {
            // Myr: Automate the use of a number of different items. See the function itself for more comments.
            //      Enabled or disabled based on config option.
            if (g_Game->settings().get_bool(settings::USER_ITEMS_AUTO_USE)) g_Game->player().apply_items(*current);

            // update for girls items that are not used up
            do_daily_items(*current);					// `J` added

            // Natural healing, 2% health and 2% tiredness per day
			current->upd_base_stat(STAT_HEALTH, 2, false);
			current->upd_base_stat(STAT_TIREDNESS, -2, false);
        }

        // Level the girl up if nessessary
        cGirls::LevelUp(current);
    }
}

bool sBrothel::runaway_check(sGirl * girl)
{
/*
	*	nothing interesting happens here unless the girl is miserable
	*
	*	WD: added m_DaysUnhappy tracking
	*/

    bool flightrisk = (girl->has_active_trait("Kidnapped") || girl->has_active_trait("Emprisoned Customer"));

    if (flightrisk && girl->happiness() > 50)	// Girls here totally against their will are more likely to try to get away
    {
        girl->m_DaysUnhappy--;					// and they don't reset days to 0 but instead reduce day count
        if (girl->m_DaysUnhappy < 0)
            girl->m_DaysUnhappy = 0;			// until it gets to 0
        return false;
    }
    else if ((girl->has_active_trait("Homeless") || girl->has_active_trait("Adventurer")) && girl->happiness() > 10)
    {	// homeless girls and adventurers know they can survive on their own so are more likely to runaway
        if (girl->m_DaysUnhappy > 3)
            girl->m_DaysUnhappy /= 2;		// they don't reset days to 0 but instead divide day count in half
        else girl->m_DaysUnhappy--;			// or just lower by 1
        return false;
    }
    else if (girl->happiness() > 10)
    {
        girl->m_DaysUnhappy = 0;
        return false;
    }

    /*
    *	`J` this was only adding up for free girls
    *	I made it add up for all girls
    *	and free girls become unhappy faster
    */
    girl->m_DaysUnhappy++;
    if (!girl->is_slave()) girl->m_DaysUnhappy++;
    /*
    *	now there's a matron on duty, she has a chance of fending off
    *	bad things.
    *
    *	previously, it was a 75% chance if a matron was employed
    *	so since we have two shifts, let's have a 35% chance per
    *	shift with a matron
    *
    *	with matrons being girls now, we have some opportunities
    *	for mischief here. For instance, is there still a matron skill?
    *	this should depend on that, if so. Also on how motivated the
    *	matron is. An unhappy matron, or one who hates the PC
    *	may be inclined to turn a blind eye to runaway attempts
    */
    //	int matron_chance = brothel->matron_count() * 35;
    int matron_chance = matron_count() * 35;

    if (g_Dice.percent(matron_chance)) return false;	// if there is a matron 70%

    if (girl->m_DayJob == JOB_REHAB && (num_girls_on_job(JOB_COUNSELOR, true) > 0) || (num_girls_on_job(JOB_COUNSELOR, false) > 0))
    {
        if (g_Dice.percent(70)) return false;
    }

    /*
    *	mainly here, we're interested in the chance that she might run away
    */
    if (girl->disobey_check(ACTION_GENERAL))	// check if the girl will run away
    {
        if (g_Dice.percent(g_Game->job_manager().guard_coverage() - girl->m_DaysUnhappy)) return false;

        girl->m_Events.AddMessage("She ran away.", IMGTYPE_PROFILE, EVENT_DANGER);
        girl->set_stat(STAT_TIREDNESS, 0);
        girl->set_stat(STAT_HEALTH, 100);
        girl->m_RunAway = 6;
        stringstream smess;
        smess << girl->FullName() << " has run away.\nSend your goons after her to attempt recapture.\nShe will escape for good after 6 weeks.\n";
        g_Game->push_message(smess.str(), COLOR_RED);
        return true;
    }

    if (girl->m_Money <= 50) { return false; }
    if (g_Dice.percent(80 - girl->m_DaysUnhappy)) { return false; }
    /*
    *	if she is unhappy she may turn to drugs
    */
    bool starts_drugs = false;
    //Crazy changed it to this might not be the best // `J` made it better :p
    string drug;
    int i = 0;
    if (girl->happiness() <= 20 && girl->has_active_trait( "Former Addict"))
    {
        while (!starts_drugs && i<10)		// `J` She will try to find a drug she used to be addicted to
        {									// and if she can't find it in 10 tries she will take what is available
            int d = g_Dice % 8;				// with a slight advantage to alcohol and fairy dust
            switch (d)
            {
            case 1:			drug = "Shroud Addict";			break;	// 12.5%
            case 2: case 3:	drug = "Fairy Dust Addict";		break;	// 25%
            case 4:			drug = "Viras Blood Addict";	break;	// 12.5%
            default:		drug = "Alcoholic";				break;	// 50%
            }
            if (girl->has_dormant_trait(drug.c_str()))
            {
                starts_drugs = true;
            }
            i++;
        }
    }
    else if (girl->happiness() <= 3 && g_Dice.percent(50) && !girl->has_active_trait( "Viras Blood Addict"))
    {
        drug = "Viras Blood Addict";
    }
    else if (girl->happiness() <= 5 && g_Dice.percent(50) && !girl->has_active_trait( "Shroud Addict"))
    {
        drug = "Shroud Addict";
    }
    else if (girl->happiness() <= 8 && g_Dice.percent(50) && !girl->has_active_trait( "Fairy Dust Addict"))
    {
        drug = "Fairy Dust Addict";
    }
    else if (girl->happiness() <= 10 && !girl->has_active_trait( "Alcoholic"))
    {
        drug = "Alcoholic";
    }

    /*
    *	if she Just Said No then we're done
    */
    if (drug.empty())
    {
        return false;
    }

    girl->gain_trait(drug.c_str());
    girl->lose_trait("Former Addict");

    /*
    *	otherwise, report the sad occurrence
    */
    stringstream ss;
    ss << "This girl's unhappiness has turned her into " << (drug == "Alcoholic" ? "an" : "a") << " " << drug << ".";
    girl->m_Events.AddMessage(ss.str(), IMGTYPE_PROFILE, EVENT_WARNING);
    return false;
}

void sBrothel::Update()
{
    stringstream ss;
    // reset the data
    m_Happiness = m_MiscCustomers = m_TotalCustomers = 0;
    m_RejectCustomersRestrict = m_RejectCustomersDisease = 0;

#pragma region //	Start of Turn Girl Setup	//

    BeginWeek();

#pragma endregion
#pragma region //	Day Shift			//

    // Moved to here so Security drops once per day instead of everytime a girl works security -PP
    m_SecurityLevel -= 10;
    m_SecurityLevel -= girls().size();	//`J` m_SecurityLevel is extremely over powered.
    // Reducing it's power a lot.
    if (m_SecurityLevel <= 0) m_SecurityLevel = 0;	 // crazy added

    // Generate customers for the brothel for the day shift and update girls
    g_Game->job_manager().do_advertising(*this, false);
    g_Game->customers().GenerateCustomers(*this, false);
    m_TotalCustomers += g_Game->GetNumCustomers();

    UpdateGirls(false);

#pragma endregion
#pragma region //	Night Shift			//

    // update the girls and satisfy the customers for this brothel during the night
    g_Game->job_manager().do_advertising(*this, true);
    g_Game->customers().GenerateCustomers(*this, true);
    m_TotalCustomers += g_Game->GetNumCustomers();
    UpdateGirls(true);


#pragma endregion
#pragma region //	Shift Summary			//

    // get the misc customers
    m_TotalCustomers += m_MiscCustomers;

    ss.str("");
    ss << m_TotalCustomers << " customers visited the building.";
    if (m_RejectCustomersRestrict > 0) ss << "\n \n" << m_RejectCustomersRestrict << " were turned away because of your sex restrictions.";
    if (m_RejectCustomersDisease > 0) ss << "\n \n" << m_RejectCustomersDisease << " were turned away because they had an STD.";
    m_Events.AddMessage(ss.str(), IMGTYPE_PROFILE, EVENT_BROTHEL);

    // empty rooms cost 2 gold to maintain
    m_Finance.building_upkeep(g_Game->tariff().empty_room_cost(*this));

    // update brothel stats
    if (!girls().empty())
        m_Fame = (total_fame() / girls().size());
    if (m_Happiness > 0 && g_Game->GetNumCustomers())
        m_Happiness = min(100, m_Happiness / m_TotalCustomers);


    // advertising costs are set independently for each brothel
    m_Finance.advertising_costs(g_Game->tariff().advertising_costs(m_AdvertisingBudget));

    ss.str("");
    ss << "Your advertising budget for this brothel is " << m_AdvertisingBudget << " gold.";
    if (g_Game->tariff().advertising_costs(m_AdvertisingBudget) != m_AdvertisingBudget)
    {
        ss << " However, due to your configuration, you instead had to pay " <<
           g_Game->tariff().advertising_costs(m_AdvertisingBudget) << " gold.";
    }
    m_Events.AddMessage(ss.str(), IMGTYPE_PROFILE, EVENT_BROTHEL);

    // `J` include antipreg potions in summary
    ss.str("");
    if (m_AntiPregPotions > 0 || m_AntiPregUsed > 0)
    {
        int num = m_AntiPregPotions;
        int used = m_AntiPregUsed;
        bool stocked = m_KeepPotionsStocked;
        bool matron = num_girls_on_job(m_MatronJob, false) >= 1;
        bool skip = false;	// to allow easy skipping of unneeded lines
        bool error = false;	// in case there is an error this makes for easier debugging

        // first line: previous stock
        if (stocked && num > 0)		ss << "You keep a regular stock of " << num << " Anti-Pregnancy potions in this brothel.\n \n";
        else if (num + used > 0)	ss << "You " << (used > 0 ? "had" : "have") << " a stock of " << (num + used) << " Anti-Pregnancy potions in this brothel.\n \n";
        else { skip = true;			ss << "You have no Anti-Pregnancy potions in this brothel."; }

        // second line: number used
        /* */if (skip){}	// skip the rest of the lines
        else if (used == 0)			{ skip = true;	ss << "None were used.\n \n"; }
        else if (num == 0)			{ skip = true;	ss << "All have been used.\n \n"; }
        else if (used > 0 && stocked)				ss << used << " were " << (used > num ? "needed" : "used") << " this week.\n \n";
        else if (used > 0 && num > 0 && !stocked)	ss << used << " were used this week leaving " << num << " in stock.\n \n";
        else
        {	// `J` put this in just in case I missed something
            ss << "error code::  BAP02|" << m_AntiPregPotions << "|" << m_AntiPregUsed << "|" << m_KeepPotionsStocked << "  :: Please report it to pinkpetal.org so it can be fixed";
            error = true;
        }

        // third line: budget
        if (!skip && stocked)
        {
            int cost = 0;
            if (used > num)
            {
                ss << used - num << " more than were in stock were needed so an emergency restock had to be made.\n";
                ss << "Normally they cost " << g_Game->tariff().anti_preg_price(1) << " gold, but our supplier charges five times the normal price for unscheduled deliveries.\n \n";
                cost += g_Game->tariff().anti_preg_price(num);
                cost += g_Game->tariff().anti_preg_price(used - num) * 5;
            }
            else
            {
                cost += g_Game->tariff().anti_preg_price(used);
            }

            ss << "Your budget for Anti-Pregnancy potions for this brothel is " << cost << " gold.";

            if (matron && used > num)
            {
                int newnum = (((used / 10) + 1) * 10) + 10;

                AddAntiPreg(newnum - num);
                ss << "\n \nThe Matron of this brothel has increased the quantity of Anti-Pregnancy potions for further orders to " << m_AntiPregPotions << ".";
            }
        }
        if (error) {
            g_LogFile.log(ELogLevel::ERROR, ss.str());
        }
        m_Events.AddMessage(ss.str(), IMGTYPE_PROFILE, EVENT_BROTHEL);
    }

#pragma endregion
    EndWeek();
}


// add the girls accommodation and food costs to the upkeep
void do_food_and_digs(IBuilding * brothel, sGirl * girl)
{
	// `J` new code for .06.01.18
	stringstream ss;

	// Gold per accommodation level
	int gold = (girl->is_slave() ? 5 : 20) * (girl->m_AccLevel + 1);
	brothel->m_Finance.girl_support(gold);

	int preferredaccom = cGirls::PreferredAccom(girl);	// what she wants/expects
	int mod = girl->m_AccLevel - preferredaccom;

	/*   if (acc == 0)	return "Bare Bones";
	else if (acc == 1)	return "Very Poor";
	else if (acc == 2)	return "Poor";
	else if (acc == 3)	return "Adequate";
	else if (acc == 4)	return "Comfortable";
	else if (acc == 5)	return "Nice";
	else if (acc == 6)	return "Good";
	else if (acc == 7)	return "Great";
	else if (acc == 8)	return "Wonderful";
	else if (acc == 9)	return "High Class";
	*/
	// bsin added Sanity for .06.02.30
	int hapA, hapB, lovA, lovB, hatA, hatB, feaA, feaB, sanA, sanB;		// A should always be lower than B
	if (mod < -9) mod = -9;	if (mod > 9) mod = 9;
	switch (mod)	// happiness, love, hate, fear
	{
	case -9:	hapA = -24;	hapB = -7;	lovA = -14;	lovB = -3;	hatA = 6;	hatB = 22;	feaA = 5;	feaB = 12;	sanA = -7;	sanB = 2;	break;
	case -8:	hapA = -19;	hapB = -6;	lovA = -11;	lovB = -3;	hatA = 5;	hatB = 18;	feaA = 4;	feaB = 9;	sanA = -6;	sanB = 2;	break;
	case -7:	hapA = -16;	hapB = -5;	lovA = -9;	lovB = -3;	hatA = 4;	hatB = 14;	feaA = 3;	feaB = 7;	sanA = -5;	sanB = 1;	break;
	case -6:	hapA = -13;	hapB = -4;	lovA = -7;	lovB = -2;	hatA = 4;	hatB = 10;	feaA = 2;	feaB = 5;	sanA = -4;	sanB = 1;	break;
	case -5:	hapA = -10;	hapB = -3;	lovA = -6;	lovB = -2;	hatA = 3;	hatB = 7;	feaA = 1;	feaB = 4;	sanA = -3;	sanB = 1;	break;
	case -4:	hapA = -8;	hapB = -2;	lovA = -5;	lovB = -1;	hatA = 2;	hatB = 5;	feaA = 0;	feaB = 3;	sanA = -2;	sanB = 0;	break;
	case -3:	hapA = -6;	hapB = -1;	lovA = -4;	lovB = 0;	hatA = 1;	hatB = 4;	feaA = 0;	feaB = 2;	sanA = -1;	sanB = 0;	break;
	case -2:	hapA = -4;	hapB = 0;	lovA = -3;	lovB = 0;	hatA = 0;	hatB = 3;	feaA = 0;	feaB = 1;	sanA = 0;	sanB = 0;	break;
	case -1:	hapA = -2;	hapB = 1;	lovA = -2;	lovB = 1;	hatA = -1;	hatB = 2;	feaA = 0;	feaB = 0;	sanA = 0;	sanB = 0;	break;
	case 0:		hapA = -1;	hapB = 3;	lovA = -1;	lovB = 2;	hatA = -1;	hatB = 1;	feaA = 0;	feaB = 0;	sanA = 0;	sanB = 1;	break;
	case 1:		hapA = 0;	hapB = 5;	lovA = -1;	lovB = 3;	hatA = -1;	hatB = 0;	feaA = 0;	feaB = 0;	sanA = 0;	sanB = 1;	break;
	case 2:		hapA = 1;	hapB = 8;	lovA = 0;	lovB = 3;	hatA = -3;	hatB = 0;	feaA = 0;	feaB = 0;	sanA = 0;	sanB = 1;	break;
	case 3:		hapA = 2;	hapB = 11;	lovA = 0;	lovB = 4;	hatA = -5;	hatB = -1;	feaA = -1;	feaB = 0;	sanA = 0;	sanB = 2;	break;
	case 4:		hapA = 3;	hapB = 14;	lovA = 1;	lovB = 4;	hatA = -6;	hatB = -1;	feaA = -1;	feaB = 0;	sanA = 0;	sanB = 2;	break;
	case 5:		hapA = 4;	hapB = 16;	lovA = 1;	lovB = 5;	hatA = -7;	hatB = -1;	feaA = -1;	feaB = 0;	sanA = 0;	sanB = 3;	break;
	case 6:		hapA = 5;	hapB = 18;	lovA = 2;	lovB = 5;	hatA = -7;	hatB = -2;	feaA = -2;	feaB = 0;	sanA = -1;	sanB = 3;	break;
	case 7:		hapA = 5;	hapB = 19;	lovA = 2;	lovB = 6;	hatA = -8;	hatB = -2;	feaA = -2;	feaB = 0;	sanA = -1;	sanB = 4;	break;
	case 8:		hapA = 5;	hapB = 20;	lovA = 2;	lovB = 7;	hatA = -9;	hatB = -3;	feaA = -3;	feaB = 0;	sanA = -1;	sanB = 4;	break;
	case 9:		hapA = 5;	hapB = 21;	lovA = 2;	lovB = 8;	hatA = -10;	hatB = -3;	feaA = -3;	feaB = 0;	sanA = -2;	sanB = 5;	break;
	default: break;
	}
	if (girl->happiness() < 20 - mod)			// if she is unhappy, her mood will go down
	{
		/* */if (mod < -6){ hapA -= 7;	hapB -= 3;	lovA -= 4;	lovB -= 1;	hatA += 2;	hatB += 5;	feaA += 2;	feaB += 5; }
		else if (mod < -3){ hapA -= 5;	hapB -= 2;	lovA -= 2;	lovB -= 1;	hatA += 1;	hatB += 3;	feaA += 1;	feaB += 3; }
		else if (mod < 0){ hapA -= 3;	hapB -= 1;	lovA -= 1;	lovB -= 0;	hatA += 0;	hatB += 2;	feaA += 0;	feaB += 2; }
		else if (mod < 1){ hapA -= 2;	hapB -= 0;	lovA -= 1;	lovB -= 0;	hatA += 0;	hatB += 1;	feaA += 0;	feaB += 1; }
		else if (mod < 4){ hapA -= 2;	hapB -= 0;	lovA -= 1;	lovB -= 0;	hatA += 0;	hatB += 1;	feaA -= 1;	feaB += 1; }
		else if (mod < 7){ hapA -= 1;	hapB -= 0;	lovA -= 1;	lovB -= 0;	hatA += 0;	hatB += 0;	feaA -= 1;	feaB += 0; }
	}
	else if (!g_Dice.percent(girl->happiness()))	// if she is not happy, her mood may go up or down
	{
		/* */if (mod < -6){ hapA -= 3;	hapB += 1;	lovA -= 3;	lovB += 0;	hatA -= 0;	hatB += 4;	feaA -= 2;	feaB += 3; }
		else if (mod < -3){ hapA -= 2;	hapB += 1;	lovA -= 2;	lovB += 0;	hatA -= 0;	hatB += 3;	feaA -= 1;	feaB += 2; }
		else if (mod < 0){ hapA -= 1;	hapB += 2;	lovA -= 1;	lovB += 1;	hatA -= 1;	hatB += 2;	feaA -= 1;	feaB += 2; }
		else if (mod < 1){ hapA -= 1;	hapB += 2;	lovA -= 1;	lovB += 1;	hatA -= 1;	hatB += 1;	feaA -= 1;	feaB += 1; }
		else if (mod < 4){ hapA += 0;	hapB += 2;	lovA -= 0;	lovB += 1;	hatA -= 1;	hatB += 1;	feaA -= 1;	feaB += 0; }
		else if (mod < 7){ hapA += 0;	hapB += 3;	lovA += 0;	lovB += 1;	hatA -= 1;	hatB -= 0;	feaA -= 0;	feaB += 0; }
	}
	else										// otherwise her mood can go up
	{
		/* */if (mod < -6){ hapA -= 1;	hapB += 2;	lovA -= 1;	lovB += 1;	hatA -= 1;	hatB -= 1;	feaA -= 1;	feaB += 1; }
		else if (mod < -3){ hapA += 0;	hapB += 2;	lovA += 0;	lovB += 1;	hatA -= 2;	hatB -= 0;	feaA -= 2;	feaB -= 0; }
		else if (mod < 0){ hapA += 0;	hapB += 3;	lovA += 0;	lovB += 1;	hatA -= 2;	hatB -= 0;	feaA -= 2;	feaB -= 0; }
		else if (mod < 1){ hapA += 0;	hapB += 5;	lovA += 0;	lovB += 1;	hatA -= 2;	hatB -= 1;	feaA -= 2;	feaB -= 0; }
		else if (mod < 4){ hapA += 1;	hapB += 7;	lovA += 0;	lovB += 2;	hatA -= 3;	hatB -= 1;	feaA -= 3;	feaB -= 0; }
		else if (mod < 7){ hapA += 2;	hapB += 8;	lovA += 1;	lovB += 3;	hatA -= 4;	hatB -= 1;	feaA -= 3;	feaB -= 1; }
	}
	if (girl->health() < 25)					// if she is injured she may be scared because of her surroundings
	{
		/* */if (mod < -6){ hapA -= 6;	hapB -= 2;	lovA -= 4;	lovB -= 1;	hatA += 3;	hatB += 4;	feaA += 2;	feaB += 4; sanA -= 4;  sanB -= 2;}
		else if (mod < -3){ hapA -= 4;	hapB -= 1;	lovA -= 3;	lovB -= 1;	hatA += 2;	hatB += 3;	feaA += 1;	feaB += 3; sanA -= 2;  sanB -= 1;}
		else if (mod < 0){	hapA -= 2;	hapB -= 1;	lovA -= 1;	lovB += 0;	hatA += 1;	hatB += 2;	feaA += 0;	feaB += 2; sanA -= 1;  sanB -= 0;}
		else if (mod < 1){	hapA -= 1;	hapB += 1;	lovA -= 0;	lovB += 0;	hatA -= 0;	hatB += 1;	feaA -= 1;	feaB += 1; sanA += 0;  sanB += 1;}
		else if (mod < 4){	hapA += 0;	hapB += 4;	lovA += 0;	lovB += 1;	hatA -= 1;	hatB += 0;	feaA -= 2;	feaB += 1; sanA += 1;  sanB += 2;}
		else if (mod < 7){	hapA += 2;	hapB += 8;	lovA += 1;	lovB += 1;	hatA -= 1;	hatB += 0;	feaA -= 3;	feaB += 0; sanA += 2;  sanB += 4;}
	}

	if (girl->is_slave())						// slaves get half as much from their mods
	{
		hapA /= 2;	hapB /= 2;	lovA /= 2;	lovB /= 2;	hatA /= 2;	hatB /= 2;	feaA /= 2;	feaB /= 2;
	}

	int hap = g_Dice.bell(hapA, hapB);
	int lov = g_Dice.bell(lovA, lovB);
	int hat = g_Dice.bell(hatA, hatB);
	int fea = g_Dice.bell(feaA, feaB);
	int san = g_Dice.bell(sanA, sanB);

	girl->happiness(hap);
	girl->pclove(lov);
	girl->pchate(hat);
	girl->pcfear(fea);
	girl->sanity(san);


	// after all the happy, love fear and hate are done, do some other checks.

#if 0
	if (girl->pchate() > girl->pcfear())		// if she hates you more than she fears you, she will disobey more
	{
		girl->obedience(g_Dice.bell(mod, 0));
		girl->spirit(g_Dice.bell(-1, 2));
	}
	else										// otherwise she will obey more in hopes of getting an upgrade
	{
		girl->obedience(g_Dice.bell(0, -mod));
		girl->spirit(g_Dice.bell(-2, 1));
	}
#endif





	int chance = 1 + (mod < 0 ? -mod : mod);
	if (!g_Dice.percent(chance)) return;
	// Only check if a trait gets modified if mod is far from 0

	bool b_intelligence = g_Dice.percent(girl->intelligence());
	bool b_confidence = g_Dice.percent(girl->confidence());
	bool b_spirit = g_Dice.percent(girl->spirit());
	bool b_refinement = g_Dice.percent(girl->refinement());
	bool b_dignity = g_Dice.percent(girl->dignity());

	if (b_refinement && b_dignity && b_confidence &&
		mod >= 0 && girl->m_AccLevel >= 5 && girl->lose_trait("Homeless", true, girl->m_AccLevel))
	{
		ss << girl->FullName() << " has gotten used to better surroundings and has lost the \"Homeless\" trait.";
	}
	else if (b_intelligence && b_spirit && b_confidence && mod >= 2 &&
	    girl->lose_trait("Masochist", true, girl->m_AccLevel - 7))
	{
		ss << girl->FullName() << " seems to be getting used to being treated well and has lost the \"Masochist\" trait.";
	}
	else if (!b_dignity && !b_spirit && !b_confidence && mod <= -1 && girl->gain_trait("Masochist", 3 - mod))
	{
        ss << girl->FullName()
           << " seems to be getting used to being treated poorly and has become a \"Masochist\".";
	}
	else if (mod < 0 && girl->lose_trait("Optimist", true, 3))
	{
		ss << girl->FullName() << " has lost her \"Optimistic\" outlook on life.";
	}
	else if (mod > 0 && girl->gain_trait("Optimist", 3))
	{
		ss << girl->FullName() << " has started to view the world from a more \"Optimistic\" point of view.";
	}
	else if (mod > 0 && g_Dice.percent(3) && girl->lose_trait("Pessimist", true, 3))
	{
		ss << girl->FullName() << " has lost her \"Pessimistic\" way of viewing the world around her.";
	}
	else if (mod < 0 && girl->gain_trait("Pessimist", 3))
	{
		ss << girl->FullName() << " has started to view the world from a more \"Pessimistic\" point of view.";
	}

	if (ss.str().length() > 0)	girl->m_Events.AddMessage(ss.str(), IMGTYPE_PROFILE, EVENT_GOODNEWS);
}

// ----- Stats

void updateGirlTurnBrothelStats(sGirl* girl)
{
	/*
	*	WD: Update each turn the stats for girl in brothel
	*
	*	Uses scaling formula in the form of
	*		bonus = (60 - STAT_HOUSE) / div
	*
	*				div =
	*		STAT	30	20	15
	*		0		2	3	4
	*		1		1	2	3
	*		10		1	2	3
	*		11		1	2	3
	*		20		1	2	2
	*		21		1	1	2
	*		30		1	1	2
	*		31		0	1	1
	*		40		0	1	1
	*		41		0	0	1
	*		50		0	0	0
	*		51		0	0	0
	*		60		0	0	0
	*		61		-1	-1	-1
	*		70		-1	-1	-1
	*		71		-1	-1	-1
	*		80		-1	-1	-2
	*		81		-1	-2	-2
	*		90		-1	-2	-2
	*		91		-2	-2	-3
	*		100		-2	-2	-3
	*
	*
	*/

	// Sanity check. Abort on dead girl
	if (girl->is_dead()) { return; }

	stringstream ss;
	string girlName = girl->FullName();
	int statHouse = girl->house();
	int bonus = (60 - statHouse) / 30;

	if (girl->is_slave())
	{
		if (bonus > 0)						// Slaves don't get penalties
		{
			girl->obedience(bonus);			// bonus vs house stat	0: 31-60, 1: 01-30, 2: 00
			girl->pcfear(-bonus);
			girl->pchate(-bonus);
			bonus = (60 - statHouse) / 15;
			girl->happiness(bonus);			// bonus vs house stat	0: 46-60, 1: 31-45, 2: 16-30, 3: 01-15, 4: 00
		}
	}
	else									// Free girls
	{
		girl->obedience(bonus);				// bonus vs house stat	-2: 91-100, -1: 61-90, 0: 31-60, 1: 01-30, 2: 00

		if (bonus > 0)						// no increase for hate or fear
		{
			girl->pcfear(-bonus);
			girl->pchate(-bonus);
		}

		bonus = (60 - statHouse) / 15;
		girl->happiness(bonus);				// bonus vs house stat	-3: 91-100, -2: 76-90, -1: 61-75, 0: 46-60, 1: 31-45, 2: 16-30, 3: 01-15, 4: 00
	}
}

// ----- Combat

bool cBuildingManager::NameExists(string name) const
{
	return has_any_girl_with(HasName(std::move(name)));
}

bool cBuildingManager::SurnameExists(string name) const
{
    for(auto& current : m_Buildings)
    {
        for(sGirl* currentGirl : current->girls())
        {
			if (name == currentGirl->Surname())
				return true;
		}
	}
	return false;
}

tinyxml2::XMLElement * cBuildingManager::SaveXML(tinyxml2::XMLElement& root) const
{
    auto& elBrothels = PushNewElement(root, "Buildings");
    for(const auto& current : m_Buildings)
    {
        g_LogFile.log(ELogLevel::DEBUG,"Saving building (", current->type_str(), "): ", current->name());
        auto& elBrothel = PushNewElement(elBrothels, current->type_str());
        current->save_xml(elBrothel);
    }

    return &elBrothels;
}

tinyxml2::XMLElement * cBuildingManager::LoadBuildingsXML(tinyxml2::XMLElement& root, const char * element_name, const char * manager_name)
{
    auto mgr_xml = manager_name ? root.FirstChildElement(manager_name) : &root;
    if (mgr_xml == nullptr)
    {
        return nullptr;
    }

    auto* pBrothels = mgr_xml->FirstChildElement(element_name);
    if (pBrothels)
    {
        for (auto& brothel : IterateChildElements(*pBrothels)) {
            auto current = create_building(brothel.Value());
            if(current) {
                g_LogFile.log(ELogLevel::DEBUG, "Loading ", brothel.Value());
                current->load_xml(brothel);
                AddBuilding(std::move(current));
            } else {
                g_LogFile.log(ELogLevel::WARNING, "Skipping XML node ", brothel.Value());
            }
        }
    }
    return mgr_xml;
}

IBuilding& cBuildingManager::AddBuilding(std::unique_ptr<IBuilding> building)
{
    m_Buildings.emplace_back(std::move(building));
    return *m_Buildings.back();
}

int cBuildingManager::total_girls() const
{
    int total = 0;
    for(auto& building : m_Buildings) {
        total += building->num_girls();
    }
    return total;
}

bool iequals(const string& a, const string& b);

std::unique_ptr<IBuilding> cBuildingManager::create_building(std::string type) const
{
    // TODO cmp based on std::string is very wasteful here!
    if(iequals(type, "Arena")) {
        return std::make_unique<sArena>();
    } else  if ( iequals(type, "Clinic") ) {
        return std::make_unique<sClinic>();
    } else  if ( iequals(type, "Farm") ) {
        return std::make_unique<sFarm>();
    } else  if ( iequals(type, "HOUSE") ) {
        return std::make_unique<sHouse>();
    } else  if ( iequals(type, "Centre") ) {
        return std::make_unique<sCentre>();
    } else  if ( iequals(type, "MovieStudio") ) {
        return std::make_unique<sMovieStudio>();
    } else  if ( iequals(type, "Brothel") ) {
        return std::make_unique<sBrothel>();
    }

    return nullptr;
}

const IBuilding& cBuildingManager::get_building(int pos) const
{
    return *m_Buildings.at(pos);
}

IBuilding& cBuildingManager::get_building(int pos)
{
    return *m_Buildings.at(pos);
}

int cBuildingManager::num_buildings() const
{
    return m_Buildings.size();
}

IBuilding * cBuildingManager::building_with_type(BuildingType type, int index)
{
    int counter = 0;
    for(auto& b : m_Buildings) {
        if(b->type() == type) {
            if(counter == index)
                return b.get();
            counter += 1;
        }
    }

    return nullptr;
}

void cBuildingManager::LoadXML(tinyxml2::XMLElement& root)
{
    // Legacy support: delete after a few versions
    /*
    LoadBuildingsXML(root, "Clinics", "Clinic_Manager");
    LoadBuildingsXML(root, "Arenas", "Arena_Manager");
    LoadBuildingsXML(root, "Centres", "Centre_Manager");
    LoadBuildingsXML(root, "Houses", "House_Manager");
    LoadBuildingsXML(root, "Farms", "Farm_Manager");
    LoadBuildingsXML(root, "MovieStudios", "MovieStudio_Manager");
     */

    // this is the new code
    LoadBuildingsXML(root, "Buildings");
}


int get_total_player_girls()
{
    int total = g_Game->buildings().total_girls();
    total += g_Game->dungeon().GetNumGirls();
    return total;
}

int get_total_player_monster_girls()
{
    int total = 0;
    for (int i = 0; i < g_Game->dungeon().GetNumGirls(); i++)
    {
        sDungeonGirl* dgirl = g_Game->dungeon().GetGirl(i);
        if (dgirl->m_Girl->has_active_trait("Not Human"))
            total++;
    }

    total += g_Game->buildings().total_girls_with(is_nonhuman);
    return total;
}

sGirl* random_girl_on_job(const cBuildingManager& mgr, JOBS job, bool at_night)
{
    auto all_girls = mgr.all_girls_with( HasJob(job, at_night) );
    if(all_girls.empty())
        return nullptr;
    return all_girls[g_Dice % all_girls.size()];
}


sGirl* random_girl_on_job(const IBuilding& building, JOBS job, bool at_night)
{
    auto all_girls = building.find_all_girls( HasJob(job, at_night) );
    if(all_girls.empty())
        return nullptr;
    return all_girls[g_Dice % all_girls.size()];
}

int cBuildingManager::num_buildings(BuildingType type) const
{
    return std::count_if(begin(m_Buildings), end(m_Buildings), [type](const std::unique_ptr<IBuilding>& b){ return b->type() == type; });
}

bool cBuildingManager::has_building(BuildingType type) const
{
    return std::any_of(begin(m_Buildings), end(m_Buildings), [type](const std::unique_ptr<IBuilding>& b){ return b->type() == type; });
}

IBuilding * cBuildingManager::random_building_with_type(BuildingType type)
{
    RandomSelector<IBuilding> selector;
    for(auto& bld : m_Buildings) {
        if(bld->type() == type) {
            selector.process(bld.get());
        }
    }
    return selector.selection();
}

std::size_t cBuildingManager::find(const IBuilding * target) const
{
    auto found = std::find_if(begin(m_Buildings), end(m_Buildings),
            [target](const std::unique_ptr<IBuilding>& b){ return b.get() == target; });
    if(found == m_Buildings.end()) {
        throw std::logic_error("Invalid building");
    }
    return std::distance(begin(m_Buildings), found);
}

IBuilding& cBuildingManager::AddBuilding(const BrothelCreationData& data)
{
    auto building = create_building(building_type_to_str(data.type));
    building->m_NumRooms = data.rooms;
    building->m_MaxNumRooms = data.maxrooms;
    building->set_background_image(data.background);
    return AddBuilding(std::move(building));
}
