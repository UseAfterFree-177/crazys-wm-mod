set(SCREENS_SRC
        screens/BuildingScreenManagement.cpp
        screens/cScreenBank.cpp
        screens/cScreenBrothelManagement.cpp
        screens/cScreenBuildingSetup.cpp
        screens/cScreenDungeon.cpp
        screens/cScreenGallery.cpp
        screens/cScreenGangs.cpp
        screens/cScreenGirlDetails.cpp
        screens/cScreenHouseDetails.cpp
        screens/cScreenItemManagement.cpp
        screens/cScreenLoadGame.cpp
        screens/cScreenLoadGame.hpp
        screens/cScreenMainMenu.cpp
        screens/cScreenMayor.cpp
        screens/cScreenMovieMaker.cpp
        screens/cScreenNewGame.cpp
        screens/cScreenPreparingGame.cpp
        screens/cScreenPrison.cpp
        screens/cScreenPropertyManagement.cpp
        screens/cScreenSettings.cpp
        screens/cScreenSlaveMarket.cpp
        screens/cScreenTown.cpp
        screens/cScreenTransfer.cpp
        screens/cScreenTurnSummary.cpp
        screens/cGameWindow.cpp
        screens/cScreenGameConfig.cpp)

set(JOBS_SRC
        jobs/WorkAdvertising.cpp
        jobs/Surgery.cpp
        jobs/WorkBarStripper.cpp
        jobs/WorkBeastCare.cpp
        jobs/WorkBrothelMassusse.cpp
        jobs/WorkBrothelStripper.cpp
        jobs/WorkWhore.cpp
        jobs/WorkCameraMage.cpp
        jobs/WorkCityGuard.cpp
        jobs/WorkCleaning.cpp
        jobs/WorkCombatTraining.cpp
        jobs/WorkComunityService.cpp
        jobs/WorkCounselor.cpp
        jobs/WorkCrystalPurifier.cpp
        jobs/WorkCureDiseases.cpp
        jobs/WorkCustService.cpp
        jobs/WorkEscort.cpp
        jobs/WorkExploreCatacombs.cpp
        jobs/WorkFakeOrgasm.cpp
        jobs/WorkFarmPonyGirl.cpp
        jobs/WorkFeedPoor.cpp
        jobs/WorkFightArenaGirls.cpp
        jobs/WorkFightBeast.cpp
        jobs/WorkFilmAction.cpp
        jobs/WorkFilmBeast.cpp
        jobs/WorkFilmBondage.cpp
        jobs/WorkFilmBuk.cpp
        jobs/WorkFilmChef.cpp
        jobs/WorkFilmDirector.cpp
        jobs/WorkFilmDominatrix.cpp
        jobs/WorkFilmGroup.cpp
        jobs/WorkFilmMusic.cpp
        jobs/WorkFilmOral.cpp
        jobs/WorkFilmPromoter.cpp
        jobs/WorkFilmPublicBDSM.cpp
        jobs/WorkFilmRandom.cpp
        jobs/WorkFilmSex.cpp
        jobs/WorkFilmStagehand.cpp
        jobs/WorkFilmTease.cpp
        jobs/WorkFilmThroat.cpp
        jobs/WorkFluffer.cpp
        jobs/WorkFreetime.cpp
        jobs/WorkGetAbort.cpp
        jobs/WorkHallDealer.cpp
        jobs/WorkHallEntertainer.cpp
        jobs/WorkHallXXXEntertainer.cpp
        jobs/WorkHealing.cpp
        jobs/WorkHouseCook.cpp
        jobs/WorkHousePet.cpp
        jobs/WorkHouseVacation.cpp
        jobs/WorkPeepShow.cpp
        jobs/WorkPersonalBedWarmer.cpp
        jobs/WorkPersonalTraining.cpp
        jobs/WorkRecruiter.cpp
        jobs/WorkRepairShop.cpp
        jobs/WorkSecurity.cpp
        jobs/WorkSleazyBarmaid.cpp
        jobs/WorkSleazyWaitress.cpp
        jobs/WorkSOBisexual.cpp
        jobs/WorkSOLesbian.cpp
        jobs/WorkSOStraight.cpp
        jobs/WorkTorturer.cpp
        jobs/Film.cpp
        jobs/Crafting.cpp
        jobs/GenericJob.cpp
        jobs/Matron.cpp
        jobs/Therapy.cpp
        jobs/BarJobs.cpp
        jobs/FarmJobs.cpp
        jobs/ClinicJobs.cpp
        jobs/JobData.cpp
        )

set(BUILDINGS_SRC
        buildings/cArena.cpp
        buildings/cBrothel.cpp
        buildings/cCentre.cpp
        buildings/cClinic.cpp
        buildings/cDungeon.cpp
        buildings/cFarm.cpp
        buildings/cHouse.cpp
        buildings/cMovieStudio.cpp
        buildings/queries.cpp
        buildings/cBuildingManager.cpp
        )

set(CHARACTER_SRC
        character/cCustomers.cpp
        character/cPlayer.cpp
        character/ICharacter.cpp
        character/sAttribute.cpp
        character/sGirl.cpp
        character/traits/cTraitsCollection.cpp
        character/traits/cTraitsManager.cpp
        character/traits/cTraitSpec.cpp
        character/predicates.cpp
        character/pregnancy.cpp
        character/cDynamicStat.cpp
        character/cDynamicStat.h
        character/cGirlPool.cpp)

set(SCRIPTING_SRC
        scripting/cEventMapping.cpp
        scripting/cLuaScript.cpp
        scripting/cLuaState.cpp
        scripting/cScriptManager.cpp
        scripting/sLuaGirl.cpp
        scripting/sLuaParameter.cpp
        )

set(SRC cEvents.cpp
        cGangs.cpp
        cGirlGangFight.cpp
        cGirls.cpp
        cGirlTorture.cpp
        cGold.cpp
        cInventory.cpp
        cJobManager.cpp
        jobs/TextRepo.cpp
        cNameList.cpp
        cObjectiveManager.cpp
        Constants.cpp
        cRandomGirl.cpp
        cRival.cpp
        cShop.cpp
        cTariff.cpp
        cTrainable.cpp
        Game.cpp
        gang_missions.cpp
        buildings/IBuilding.cpp
        InterfaceGlobals.cpp
        InterfaceProcesses.cpp
        Inventory.cpp
        sStorage.cpp
        XmlMisc.cpp
        cGameSettings.cpp
        combat/combat.cpp
        combat/action.cpp
        combat/combatant.cpp
        )

add_library(WmGameObjects OBJECT ${BUILDINGS_SRC} ${SCREENS_SRC} ${JOBS_SRC} ${CHARACTER_SRC} ${SCRIPTING_SRC} ${SRC})
target_link_libraries(WmGameObjects PUBLIC WMEngineObjects pthread doctest::doctest)
set_target_properties(WmGameObjects PROPERTIES CXX_STANDARD 14)
target_include_directories(WmGameObjects PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if (UNIX)
    target_compile_definitions(WmGameObjects PUBLIC LINUX=1)
endif(UNIX)

add_executable(WhoreMaster $<TARGET_OBJECTS:WmGameObjects> $<TARGET_OBJECTS:WMEngineObjects> main.cpp)
target_link_libraries(WhoreMaster PUBLIC WmGameObjects)

add_executable(WhoreMasterFuzz $<TARGET_OBJECTS:WmGameObjects> $<TARGET_OBJECTS:WMEngineObjects> fuzzmain.cpp)
target_link_libraries(WhoreMasterFuzz PUBLIC WmGameObjects)

add_executable(WhoreMasterTest $<TARGET_OBJECTS:WmGameObjects> $<TARGET_OBJECTS:WMEngineObjects> unittest.cpp)
target_link_libraries(WhoreMasterTest PUBLIC WmGameObjects)

set_target_properties(WhoreMaster WhoreMasterFuzz WhoreMasterTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../..")

