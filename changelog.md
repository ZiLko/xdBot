# v2.0.0-beta.4

* Fixed dual bug when replaying a macro.
* Fixed not being able to import .json macros (with the import button).

# v2.0.0-beta.3

* Fixed rotation bug.
* Fixed crash on load macro if node ids wasnt installed.
* Fixed a few undisclosed bugs.
* Added Alt modifier to default keybinds.

# v2.0.0-beta.2

* A bunch of small changes that are not even worth putting here.

# v2.0.0-beta.1

* Remove NodeIDs dependency.

# v2.0.0-alpha.14

* Actually remove Click on Steps incompatibility.

# v2.0.0-alpha.13

* Fixed Macro Editor and Noclip Settings popups not changing color.
* Remove click on steps incompatibility and replaced it for a warning.

# v2.0.0-alpha.12

* Fixed speedhack bug.
* Added Noclip settings.

# v2.0.0-alpha.11

* Fixed bug when toggling "Fix Shaders".
* Fixed bug when replaying macros on the first attempt.
* Fixed bug where the player wouldn't do the first jump of the macro.
* Fixed render song being offsync when starting from a startpos.
* Fixed render recording frames while player is dead.
* Added Macro Editor.
* Added macro count in load macro layer.
* Added render video args.
* Added render restore default settings button. 
* Added incompatibility warning if BetterInputs is installed.
* Added click on steps incompatibility just in case lol.
* Added GDH Tps Bypass incompatibility.
* Improved .xd macros compatibility a bit.
* Improved layout mode load time.
* Made layout mode hide solid objects with negative scales.
* Removed JSON toggle (now it always saves as JSON).
* Slightly decreased respawn lag while recording.

# v2.0.0-alpha.10

* Fixed crash when trying to record or play a macro.

# v2.0.0-alpha.9

* Removed tps bypass detector because it was NOT detecting.
* Added Prism Menu TPS bypass to incompatibilities;

# v2.0.0-alpha.8

* Fixed freezing when recording in platformer mode.
* Fixed crash when opening menu after switching scene while it was open.
* Fixed crash when opening the menu after closing it with back button in android.

# v2.0.0-alpha.7

* Fix QOLMod tps bypass detector being inverted.

# v2.0.0-alpha.6

* Fixed koad.
* Fixed load layer button touch priority bug.
* Fixed "always practice fixes" not working (no one noticed).
* Fixed clear button not working.
* Added sort direction button in load layer.
* Added extension text when loading macros.
* Added TPS / Physics bypass **DETECTOR**.
* Made macros save as .gdr.json when saving as json.
* Improved incompatibility system.
* Removed Android custom keybinds dependency.
* Removed Windows-only settings on Android.
* Removed Old Loading setting.

# v2.0.0-alpha.5

* Fixed crash with death replay on platformer mode.
* Fixed level restarting when toggling playing on after finishing the level.
* Fixed layout mode removing deco objects that are linked to a teleport orb or portal.
* Fixed crash when loading a macro in android.
* Fixed some inconsistent casing xd.
* Fixed default clickbot click sound only sounding on the left.
* Made layout mode remove objects with "NoTouch" on.

# v2.0.0-alpha.4

* Fixed visual bug in load macro list with happy textures mod.
* Fixed screen freezing when opening a level while frame stepper was on.
* Fixed pause not unpausing visually when frame stepper was on.
* Fixed editor playtest freezing when frame stepper was on.
* Fixed occasional crash when typing in any of the render inputs.
* Fixed crash when rendering.
* Fixed bug where layout mode would remove orange portal.
* Added page indicator dots.
* Added Button Editor for Android users.
* Added speedhack and coin finder to auto safe mode too.
* Made it so frame stepper auto disables when opening a level to avoid confusion.

# v2.0.0-alpha.3

* Fixed crash on android when respawning from a checkpoint.
* Fixed renderer song offset.

# v2.0.0-alpha.2

* Fixed percentage bug.
* Fixed crash on checkpoint restart for some users.
* Fixed one of the crashes when searching for a macro.
* Fixed checkpoint incompatibility with QOLMod.
* Fixed crash when loading a macro on Android.
* Fixed robot sprite transferring to player 2 when show trajectory is on.
* Fixed speed changing when show trajectory is on.
* Fixed player sprite flipping vertically when show trajectory goes through a gravity portal.
* Fixed music sounding strange when rendering.
* Fixed bug where replaying macros only worked in the same session.
* Fixed macro breaking when respawning in speed portals.
* Fixed .xd macros loading incorrectly.
* Fixed error when saving or loading for some users.
* Fixed frame stepper advancing two frames instead of one.
* Fixed frame stepper behaving strangely with speedhack on.
* Fixed frame stepper not freezing robot/spider animation.
* Fixed layout mode occasionally breaking rotate triggers.
* Fixed menu color looking weird for the first time after applying changes.
* Fixed some touch priority issue after loading a macro.
* Added Coin Finder.
* Added JSON saving toggle.
* Added render codec info popup.
* Added button to open menu in mod settings.
* Added song re-sync when toggling frame stepper off.
* Added ffmpeg download link popup.
* Removed "Lock Delta FPS" setting because it was unnecessary.
* Improved layout mode load time.
* Changed how frame stepper keybinds work, now one key toggles and another advances frame.
* Slightly increased frame advance repeat speed when holding.

# v2.0.0-alpha.1

* Completely rewrote the bot.
* Now saves macros as '.gdr' instead of '.xd' (you are still able to load '.xd' files, but not save them).
* Added Practice Pixes.
* Added Auto Saving.
* Added Renderer.
* Added a basic ClickBot.
* Added Seed Modifier.
* Added Show Trajectory.
* Added Frame Offset setting.
* Added Customizable Keybinds.
* Plus every feature from xdBot v1 (except frame fixes).

# v1.6.1

* geode 3.0.0 beta 1

# v1.6.0

* Frame fixes for android
* Pc macros work on android
* Old macros dont work cus i changed the frames thing
* broke settings button position
* got rid of a bunch of stuff
* geode 3.0.0 alpha 2

# v1.5.5

* Fixed spider animation not showing when playing a frame fixes macro.

# v1.5.4

* Added layout mode.
* Added search button.
* Slightly improved respawn lag when recording.
* Made safe mode better in Android (thanks to viper).

# v1.5.3

* Fixed settings button position.
* Fixed player death being called twice or something.

# v1.5.2

* Remove Discord.

# v1.5.1

 * Temporarily disabled Ignore Inputs on Android to fix major bugs, will re-implement in the future.

# v1.5.0

 * Added NoClip setting.
 * Added Instant Respawn setting.
 * Added Instant Mirror Portal setting.
 * Added Instant Disable Death Effect setting.
 * Added Ignore Inputs setting.
 * Added Clear All Macros button.
 * Added ignore click on play on Android.
 * Added Auto Safe Mode on Android.
 * Added a warning when saving a macro with an existing name.
 * Made Android buttons slightly bigger.
 * Fixed frame label sometimes not showing.
 * Fixed some platformer macro bugs.
 * Slightly improved Android performace when loading frame fixes macros.
 * Possibly fixed one more crash on restart.
 
# v1.4.8

 * Fixed some macro bugs on Android.
 * Added Auto Safe Mode setting.

# v1.4.7

 * Fixed speedhack audio staying after toggling play macro.
 * Fixed macro not recording a release when you place a checkpoint while holding.

# v1.4.6

 * Fixed occasional crash on restart.
 * Fixed macros ignoring frame fixes when loaded.
 * Made it so speedhack sets to 1 when you play a macro to avoid confusion.
 * Made it so xdBot button always appears at the end screen if the setting is enabled.

# v1.4.5

 * Fixed some Android macros crashing.
 * Fixed frame stepper button being misaligned on Android.
 * Fixed music unsyncing when resuming the game.
 * Fixed music unsyncing when using speedhack.
 * Added Speedhack Audio setting.
 * Added Show Button at End setting.
 * Added ignore click on play on Windows.
 * You can now hold the frame stepper key on Windows.
 * Made speedhack limit 2 instead of 1.
 * Speedhack now also works while playing a level.

# v1.4.4

 * Fixed Android macros recording incorrectly.

# v1.4.3

 * Re-worked FPS selector.
 * Fixed frame stepper buttons conflicting with platformer buttons on Android.
 * Fixed incompatibility with BetterPause.
 

# v1.4.2

 * Added FPS selector to both platforms. Higher FPS might make the game slow down on Android.
 * Fixed not being able to hold at the beginning of an attempt on Android.
 * Moved the disable button away from the advance frame one to prevent misclicks on Android.
 * Removed size macro info because it was too cluttered.

# v1.4.1

 * Fixed layering issues with labels and 2.2 shaders.
 * Fixed playing label not working on Windows.
 * Fixed Vanilla macros not saving correctly.
 * Fixed Android buttons staying after completing a level.
 * Fixed spider teleport animation not showing on Android.

# v1.4.0

 * Added android support.
 * Slightly improved respawn lag while recording.
 * Changed the menu button sprite.
 * Added discord button.

# v1.3.10
 
 * Fixed macros working different when loaded.

# v1.3.9

 * Fixed crash i think xd
 * You can now add inputs to a macro by recording over it (again).

# v1.3.8

 * Fixed occasional crash on restart again.

# v1.3.7

 * Added bugs.
 * Fixed mysterious bug.
 * Removed some features temporarily to fix mysterious bug.

# v1.3.6

 * Air update.

# v1.3.5

 * Fixed macros saving with "WA" at the end lmao.
 * Fixed Delete Macro pop up title being Load Macro instead of Delete Macro.
 * Fixed macros not saving if the path contains non-english characters. 
 * Fixed crash when opening the pause menu.

# v1.3.4

 * Fixed occasional crash when restarting in practice mode.
 * Added auto music sync (bad) while playing a macro.
 * User inputs now are ignored while playing a macro.
 * You can now add inputs to a macro by recording over it (macro merger soon).

# v1.3.3

 * Added override macro mode setting.
 * Fixed labels not disappearing after disabling recording or playing.
 * Previous version never came out but fixed a crash xd.

# v1.3.2

 * Added version label on the menu.
 * Saved macros list is now in the correct alphabetical order.

# v1.3.1

 * Fixed rotation bug on input fix and frame fix modes.
 * Added "Auto-Enable Play" setting.
 * Speedhack shortcut value now saves after restarting the game.
 * Fixed frame stepper advancing two frames instead of one.
 * Added toggleable frame and macro state labels.
 * Vanilla and Frame Fix settings now also change the way the macro plays.

# v1.3.0

 * Removed lock delta (it was useless).
 * Added clicks to the macro info label.
 * Added Input Fixes and Frame Fixes. Input Fixes is on by default, "vanilla" option disables it.
 * Fixed teleport orb bug for real.

# v1.2.1

 * Added (forced) safe mode when playing a macro.

# v1.2.0

 * Added speedhack key shortcut.
 * Added Frame Stepper.

# v1.1.1

 * Small bugfixes.

# v1.1.0

 * Added speedhack.
 * Added lock delta.
 * Bugfixes.

# v1.0.2

 * Reworked the macro saving and loading system.

# v1.0.12

 * Fixed teleport orb not working while playing a macro.
 * Now targetting correct geode version.

# v1.0.1

 * Fixed GUI breaking on different aspect ratios.
 * Improved macro recording accuracy.
 * Made code look less ugly.

# v1.0.0

 * First release
