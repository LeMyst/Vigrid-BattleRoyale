/**
 *  EarPlugs - one key cycles the local client through Off -> Light -> Heavy. Bus volume comes down
 *  AND a low-pass EQ goes on, so gunfire sounds muffled rather than merely distant.
 *
 *  This folder is its own PBO because the build packs every folder holding a config.cpp with no
 *  ancestor config.cpp. Keep exactly ONE config.cpp here and do not nest another one below it.
 *
 *  DISCIPLINE RULE: nothing under Extra/EarPlugs/ may reference a BattleRoyale* symbol. This addon
 *  has no API at all - nothing in the host mod needs to talk to it - so the rule costs nothing here
 *  beyond its own logger, constants, stringtable and Inputs.xml. It hooks nothing but the vanilla
 *  MissionGameplay, so it works on any DayZ server with or without Battle Royale loaded. That keeps
 *  a later extraction into a standalone @Vigrid-EarPlugs mod a build-plumbing job rather than a
 *  rewrite - and it keeps `config.cpp.disabled` working as a one-rename kill switch.
 *
 *  LICENCE NOTE. The idea comes from DaemonForge's DayZ-EarPlugs
 *  (https://github.com/DaemonForge/DayZ-EarPlugs), which is GPLv3. This repository is DSPL-SA, which
 *  adds non-commercial and DayZ-only restrictions that GPLv3 section 7 forbids adding to a combined
 *  work - so no code and no asset from that mod is present here. The volume icons in particular are
 *  theirs, which is why the indicator is a text badge built from widgets. Everything below is
 *  written against vanilla declarations under P:\ and this repo's own addons. See README.md.
 */
class CfgPatches
{
    class Vigrid_EarPlugs
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Scripts"
        };
    };
};

class CfgMods
{
    class Vigrid_EarPlugs
    {
        dir = "Extra/EarPlugs";
        credits = "Myst";
        type = "mod";
        name = "Vigrid Ear Plugs";

        //--- A fourth inputs= alongside the Battle Royale, Party and Map ones is supported:
        //--- @DayZ-Expansion ships three PBOs that each declare their own.
        inputs = "Vigrid-BattleRoyale/Extra/EarPlugs/Data/Inputs.xml";

        dependencies[] =
        {
            "Game",
            "Mission"
        };

        //--- Declared for consistency with the other Extra/ addons and unconsumed: the host mod has
        //--- no call sites to guard, because there is no API to call. defines[] are visible across
        //--- CfgMods entries, so it is there if that ever changes.
        defines[] =
        {
            "VIGRID_EARPLUGS"
        };

        //--- Only declare a script module once its folder actually exists: the engine registers
        //--- files by directory, and pointing a module at a missing folder is a load-time error.
        //--- There is no 4_World folder here on purpose - SetMasterAttenuation is called on the
        //--- local PlayerBase from 5_Mission, which needs no entity-stage code of its own.
        class defs
        {
            class gameScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Extra/EarPlugs/Scripts/3_Game"
                };
            };
            class missionScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Extra/EarPlugs/Scripts/5_Mission"
                };
            };
        };
    };
};

/**
 *  The muffle itself. Man.SetMasterAttenuation(string) - P:\scripts\3_game\entities\man.c:41 -
 *  resolves a classname under CfgSoundEffects >> AttenuationsEffects and applies a client-local EQ
 *  to the master bus. "" clears it. Vanilla drives the same slot for HouseAttenuation,
 *  CarAttenuation, UnconsciousAttenuation, BurlapSackAttenuation and FlashbangAttenuation
 *  (P:\DZ\sounds\hpp\config.cpp:2145).
 *
 *  ⚠️ THESE ARE NEW CHILD CLASSES, NOT REDECLARATIONS. CfgSoundEffects and AttenuationsEffects are
 *  parentless in vanilla, so there is no parent to carry - but the repo's config-override rule still
 *  binds for the five vanilla presets: do NOT restate BurlapSackAttenuation or any sibling here.
 *  Redeclaring an existing class without its parent REPLACES it rather than merge-patching it, and
 *  that is what hard-froze every client with `class RscMapControl` in 2026-08-07. Only add ours.
 *
 *  Reading the numbers: `center` is Hz, `bandwidth` octaves, `gain` a LINEAR multiplier - so under 1
 *  is a cut. Vanilla's values are dB conversions, and ours are written the same way with the dB in
 *  the comment, because 0.17782794 is unreadable and -15 dB is not.
 *
 *  Echo is deliberately zeroed in both. Every vanilla preset carries some, because they are all
 *  modelling something wrapped around your head - a sack, a car cabin, a concussion. Earplugs do not
 *  reverberate; they just take the top off. Delay is 1 rather than 0, matching CarAttenuation, which
 *  is the vanilla preset that also wants no echo.
 */
class CfgSoundEffects
{
    class AttenuationsEffects
    {
        //--- Light: foam-plug territory. Highs are gone, speech and footsteps still read clearly.
        class VigridEarPlugsLightAttenuation
        {
            class Equalizer0
            {
                center[] = {98, 1568, 6272, 12544};
                bandwidth[] = {2, 2, 1.8, 2};
                gain[] = {1, 0.79432821, 0.50118721, 0.25118864};        // 0 / -2 / -6 / -12 dB
            };
            class Equalizer1
            {
                center[] = {60, 500, 3000, 15000};
                bandwidth[] = {2, 2, 2, 2};
                gain[] = {1, 1, 0.70794578, 0.25118864};                 // 0 / 0 / -3 / -12 dB
            };
            class Echo
            {
                WetDryMix = 0;
                Feedback = 0;
                Delay = 1;
            };
        };

        //--- Heavy: range-defender territory. The top two octaves are essentially gone and the mids
        //--- are well down, with a touch of low-end lift - which is what plugged ears actually do,
        //--- since bone conduction keeps the bottom end while the canal is blocked.
        class VigridEarPlugsHeavyAttenuation
        {
            class Equalizer0
            {
                center[] = {98, 1568, 6272, 12544};
                bandwidth[] = {2, 2, 1.8, 2};
                gain[] = {1.1220185, 0.50118721, 0.17782794, 0.08912509}; // +1 / -6 / -15 / -21 dB
            };
            class Equalizer1
            {
                center[] = {60, 500, 3000, 15000};
                bandwidth[] = {2, 2, 2, 2};
                gain[] = {1, 0.79432821, 0.31622777, 0.08912509};        // 0 / -2 / -10 / -21 dB
            };
            class Echo
            {
                WetDryMix = 0;
                Feedback = 0;
                Delay = 1;
            };
        };
    };
};
