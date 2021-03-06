# TODO
## Engine
* [ ] Make use of SDL2 Textures instead of Surfaces
* [ ] file system abstraction (physfs? / boost::filesystem?)
* [ ] libarchive for archive handling
* [ ] Integrate boost::stacktrace for better debugging
* [ ] Async loading of animations
* [ ] for logging? https://github.com/gabime/spdlog

## Code
* [ ] Move all remaining game code out of the user interface
* [ ] Replace image name based tags by an xml file
* [ ] Improve image paths handling / image selection
* [ ] Reduce usage of sConfig
* [ ] Improve performance of animations (upscale only when blitting?)
* [ ] Pre-generate list of all images for each girl on game start
* [ ] De-interlacing
* [ ] Convert job functions to classes
  - [ ] General
  - [ ] Bar
  - [ ] Gambling
  - [ ] Strip Club
  - [ ] Brothel
  - [ ] House
  - [ ] Sex Training
  - [ ] Arena
  - [ ] Studio
  - [ ] Centre

## Scripting
* [ ] Extend lua scripting to cover everything the old scripts did
* [ ] New Commands:
  - [ ] Player inventory
* [ ] New Triggers:
  - [ ] OnBought

## UI
* [ ] Per building/global anti preg setting
* [ ] Game-Mode files
* [ ] separate logical and actual render size and make configurable
* [ ] Active Building Widget
* [ ] LoadGame delete button
* [ ] LoadGame more info

## Bugs
* [ ] some parts of the old scripts are still missing
* [ ] Scrollbar release mouse while outside problem
* [ ] Pregnancies of runaway/prison girls currently don't work
* [ ] GetRandomRivalToSabotage function is wrong
* [ ] "ERROR: Cannot find the torturer!"
* [ ] "Could not select the target. If you encounter this error, "
* [ ] "ERROR: Selected girl is a runaway"
* [ ] Clicking the item filter in the inventory screen sets focus to left inventory
* [ ] async scripts and messages may get scrambled
* [ ] Bought a dead girl from the market?
* [ ] ffmpeg frame duration off by one
* [ ] bugs in shop/inventory screen
  - [ ] "your inventory is full", but seems to buy anyway
  - [ ] "your inventory is full" for sell button
* [ ] gracefully handle missing/corrupt config file
* [ ] numeric columns may contain some textual data (i.e. Health column: DEAD) that causes an exception during sort


## Changes due to job text xml
* BarCook
  - removed performance influence of `pre` message
  - non-trait messages can no longer appear when trait is present