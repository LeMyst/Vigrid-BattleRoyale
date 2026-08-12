/**
 *  Vigrid Map Satellite - turns on the satellite layer of the in-game map.
 *
 *  This is Expansion Navigation's map-control block, copied into `class MapDefaults` and nowhere
 *  else. THE ONE RULE: do not add a `class RscMapControl` here. Doing that is what hard-froze every
 *  client on 2026-08-07 - proven by bisection, see below - and it is the only thing about this file
 *  that is dangerous. The values themselves are not.
 *
 *  WHY THIS EXISTS. The satellite map everyone was used to came from @DayZ-Expansion-Navigation, not
 *  from the terrain. Vanilla ships maxSatelliteAlpha = 0, which disables the satellite layer outright
 *  and leaves the procedurally drawn contour map. So unloading Navigation, which the Extra/Map addon
 *  makes possible, silently swapped the map back to topo. This file is those numbers.
 *
 *  Note the satellite imagery is the TERRAIN's own, already shipped with the world
 *  (dz\worlds\<world>\data\layers\s_XXX_YYY_lco.paa). Nothing is bundled here: this is ten numbers,
 *  not a gigabyte of tiles.
 *
 *  ------------------------------------------------------------------------------------------------
 *  THE 2026-08-07 FREEZE, AND WHAT ACTUALLY CAUSED IT
 *  ------------------------------------------------------------------------------------------------
 *
 *  SOLVED 2026-08-08, BY BISECTION. THE CAUSE WAS A PARENTLESS `class RscMapControl`.
 *
 *  The version that froze declared, alongside its MapDefaults:
 *
 *      class RscMapControl                    //--- <-- NO `: MapDefaults`
 *      {
 *          maxSatelliteAlpha = 1.0;
 *          alphaFadeStartScale = 1;
 *          alphaFadeEndScale = 1;
 *      };
 *
 *  Vanilla declares `class RscMapControl: MapDefaults {};`. Redeclaring it WITHOUT the parent does
 *  not merge into the existing definition, it REPLACES it - so the real map control ended up with
 *  the satellite layer switched on and every other map property gone: no colours, and critically no
 *  ptsPerSquare* tessellation densities. Zero points per square is the kind of value that turns a
 *  polygon tessellator into an infinite loop, which is what a permanent hang with no error, at every
 *  zoom, on every MapWidget, while the server runs on untouched, actually looks like.
 *
 *  THE FILE'S OWN EARLIER COMMENT ASSERTED THE OPPOSITE - "an external class with no parent is a
 *  merge-patch onto the existing definition, which is what is wanted". That belief is what caused
 *  the freeze. It is wrong, and it is wrong for every config override in this repo, not just this
 *  one: if you redeclare an existing class, carry its parent or do not redeclare it.
 *
 *  Symptoms, for recognition: turned on 2026-08-07 (Sakhal, DayZDiag) and hard-froze every client
 *  the moment ANY MapWidget rendered. Not a stall - the client stopped ticking and never came back,
 *  while the SERVER carried on normally for another 30s+. Reproduced on all three map surfaces
 *  (fullscreen map, HUD minimap, spawn selection). "All clients die on the same line in the same
 *  second" is what says deterministic rather than memory pressure; it does also eat RAM (~19 GB
 *  across 4 processes), but that is a symptom, not the cause, and reading it as the cause cost a
 *  debugging cycle.
 *
 *  WHAT WAS WRONGLY BLAMED, and is now proven innocent - each tested individually, one build each,
 *  on top of an otherwise working config:
 *
 *      hypothesis                                          verdict
 *      maxUserMapAlpha = 1 (usermap layer left on)         REFUTED - no freeze
 *      scaleMin/scaleMax/scaleDefault absent               REFUTED - no freeze
 *      parentless class RscMapControl re-added             *** FREEZE REPRODUCED ***
 *
 *  So the "curated subset" theory was a red herring: BOTH keys the first attempt omitted are
 *  individually harmless. They are kept below anyway because they are Expansion's shipping values
 *  and there is no reason to deviate - but they were never the bug.
 *
 *  Also ruled out: raw data volume (Sakhal has the SMALLEST satellite set of the three terrains -
 *  1024 tiles / 90.6 MB vs Chernarus 1849 / 232.5 MB, all the same DXT1 .paa format); Sakhal being
 *  special; and DayZDiag being special. Expansion ships Navigation/Worlds/sakhal/config.cpp with
 *  only CfgWorlds location names in it, so it runs this exact global block on Sakhal for everyone.
 *
 *  ------------------------------------------------------------------------------------------------
 *  WHERE THE SETTINGS LIVE
 *  ------------------------------------------------------------------------------------------------
 *
 *  In P:\dz\data\config.cpp, at file top level and *outside* CfgWorlds:
 *
 *      class MapDefaults              { maxSatelliteAlpha = 0; alphaFadeStartScale = 1; ... }
 *      class RscMapControl: MapDefaults { };          //--- empty body, inherits everything
 *
 *  Both are GLOBAL. Nothing in a CfgWorlds entry names a map-control class, so there is no per-world
 *  variant to override - the block below covers every terrain, and reaches every MapWidget in the
 *  game: this mod's fullscreen map, its minimap, and the spawn-selection screen. Expansion agrees:
 *  it moved this block out of Navigation/Worlds/ChernarusPlus/ into Navigation/Worlds/config.cpp
 *  between v1.8.55 and v1.9.49, and declares requiredAddons[] = {"DZ_Data"}.
 *
 *  (An even earlier version of this file patched CfgWorlds <World> instead. That was inert - it wrote
 *  keys nothing reads. Recorded so it is not rediscovered.)
 *
 *  ONLY MapDefaults IS PATCHED, and after the bisection above that is not a style preference but the
 *  one hard requirement of this file. Vanilla's RscMapControl derives from MapDefaults with an empty
 *  body, so it inherits everything below for free. Expansion's own `class RscMapControl: MapDefaults`
 *  overrides a handful of COLOURS and no satellite key at all - so the earlier claim here that
 *  "Expansion patches both" was simply false, and acting on it is what froze the client.
 *
 *  WHY IT IS A RENAME AND NOT A SETTING. There is no script access to any of this. MapWidget exposes
 *  no satellite API, and maxSatelliteAlpha / userMapPath appear nowhere in P:\scripts - they are
 *  static config read at world load. So this cannot be a runtime server setting, only a config
 *  override shipped in a PBO. Rename config.cpp -> config.cpp.disabled to turn it off; _EnumPaths.bat
 *  only ever sees config.cpp, so the rename is the whole control surface. Same kill switch
 *  Extra/DisableFogChernarusPlus uses.
 *
 *  NOTE when toggling: an incremental Deploy.bat does not reliably delete the PBO of an addon you
 *  just disabled. Check %ModBuildDirectory%@Vigrid-BattleRoyale\Addons\ and remove
 *  extra_mapsatellite.pbo plus its .bisign by hand, or the old config keeps loading and you will
 *  conclude the rename did nothing.
 *
 *  Separate PBO from Extra/Map on purpose. This one is a look, toggled by an admin; the map addon is
 *  the feature. Neither should require rebuilding the other, and Extra/Map's canvas overlays draw
 *  over whatever layer the MapWidget shows, so nothing there depends on this either way.
 *
 *  Config only. No scripts, no assets, no PBO content beyond this file.
 */
class CfgPatches
{
    class Vigrid_MapSatellite
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        //--- DZ_Data is where MapDefaults is declared. No world PBO is needed: the class is global,
        //--- not per-terrain. Same requiredAddons Expansion's own Worlds/config.cpp uses.
        requiredAddons[] =
        {
            "DZ_Data"
        };
    };
};

/**
 *  Expansion Navigation v1.9.49, DayZExpansion/Navigation/Worlds/config.cpp:29-38, copied whole.
 *
 *  DO NOT CHERRY-PICK FROM THIS BLOCK. That is precisely what broke it last time. These ten values
 *  are the only combination anyone has ever shipped working; treat them as one unit. If a value here
 *  needs to change, change exactly one, rebuild, and test - see the freeze notes in the header.
 *
 *  Not copied from Expansion, because none of it is about the satellite layer: x/y/w/h (map-control
 *  geometry - our layouts declare their own) and the ptsPerSquare* / colour / font / Legend blocks
 *  (Expansion's cartographic restyle of the topo map).
 */
class MapDefaults
{
    //--- Vanilla MapDefaults declares none of these three. Expansion adds all three, and the map's
    //--- zoom range being undefined is live hypothesis H2 for the freeze - so they stay, as a set.
    //--- scaleMin = 0.001 is why Expansion's own map zoomed in absurdly far; it cannot do that here,
    //--- because VigridMapMenu.ClampZoom() re-clamps GetScale() to 0.06 .. 1.0 every frame.
    scaleMin = 0.001;
    scaleMax = 1;
    scaleDefault = 0.16;

    //--- 0 in vanilla, which is what disables the satellite layer entirely. This is the whole point
    //--- of the addon.
    maxSatelliteAlpha = 1;

    //--- Vanilla is 1. At 2 the fade is pushed past the top of the scale range, so the satellite
    //--- layer never fades out at any zoom. Expansion's value, kept for now because this file is a
    //--- verbatim copy first; dropping to 1 (satellite up close, contour map when zoomed out) is the
    //--- look actually wanted here and is the FIRST single-variable change to try.
    alphaFadeStartScale = 2;
    alphaFadeEndScale = 2;

    //--- The usermap is the hand-drawn paper map the terrain ships. Turning it OFF alongside
    //--- satellite ON is Expansion's pairing and live hypothesis H1: the version that froze left
    //--- this at vanilla's 1, so the engine was asked to composite satellite over an active usermap.
    //--- Consequence to accept: when satellite fades out, the fallback is the procedurally drawn
    //--- contour map, NOT the paper map. Satellite and usermap are alternatives here, not layers.
    userMapPath = "";
    maxUserMapAlpha = 0;
    alphaUserMapFadeStartScale = 0.34;
    alphaUserMapFadeEndScale = 0.34;

    //--- ---------------------------------------------------------------------------------------
    //--- The overlay colours. NOT decoration - these are why the satellite layer is legible.
    //--- ---------------------------------------------------------------------------------------
    //---
    //--- The map draws its procedural vector layers (forest, rocks, contours, sea, roads) ON TOP
    //--- of whatever raster layer is underneath. Vanilla tuned them to sit over the contour map,
    //--- so several are opaque or near-opaque and simply hide the satellite imagery:
    //---
    //---     colorForest       vanilla {0.36, 0.78, 0.08, 0.5}   bright green at 50% alpha
    //---     colorForestBorder vanilla {0.40, 0.80, 0.00, 1.0}   solid green outline
    //---     colorSea          vanilla {0.64, 0.76, 0.89, 1.0}   OPAQUE - hides the sea entirely
    //---     colorCountlines   vanilla {0.85, 0.80, 0.65, 1.0}   opaque contour lines
    //---
    //--- The green blocks over forest when zoomed in (reported 2026-08-08, first look at a working
    //--- satellite map) are colorForest. Expansion zeroes its alpha, and does the same for the
    //--- other overlays that compete with imagery, keeping only what still reads as useful over a
    //--- photo: main contours at half alpha, roads, rails, power lines, grid and place names.
    //---
    //--- Copied as a block for the same reason as the ten values above. These are draw-time tints
    //--- and cannot hang anything - unlike the alpha gates and the scale range, which govern how
    //--- the engine LOADS the layer - so the "do not cherry-pick" rule is about those, not these.
    //--- Still copied whole: picking one colour out leaves the rest fighting the imagery, which is
    //--- how the green got noticed in the first place.
    //---
    //--- Expansion's `Legend` icon subclasses and its `text` / `textureComboBoxColor` keys are NOT
    //--- copied - those restyle the map's icon set and have nothing to do with the raster layer.
    showCountourInterval = 1;
    colorBackground[] = {0.969, 0.957, 0.949, 1};
    colorSea[] = {0.467, 0.631, 0.851, 0.5};
    colorForest[] = {0.38, 0.47, 0.23, 0};
    colorForestBorder[] = {0, 0, 0, 0};
    colorRocks[] = {0, 0, 0, 0.3};
    colorRocksBorder[] = {0, 0, 0, 0};
    colorLevels[] = {0.286, 0.177, 0.094, 0.5};
    colorMainCountlines[] = {0.572, 0.354, 0.188, 0.5};
    colorCountlines[] = {0.572, 0.354, 0.188, 0};
    colorMainCountlinesWater[] = {0.491, 0.577, 0.702, 0.6};
    colorCountlinesWater[] = {0.491, 0.577, 0.702, 0.3};
    colorPowerLines[] = {0.1, 0.1, 0.1, 1};
    colorRailWay[] = {0.8, 0.2, 0, 1};
    //--- colorNames is Expansion's one value NOT copied here - it is near-black {0.1,0.1,0.1,0.9}
    //--- and unreadable over imagery. Set at the bottom of this class instead, with the reasoning.
    colorInactive[] = {1, 1, 1, 0};
    colorOutside[] = {0, 0, 0, 1};
    colorTracks[] = {0.84, 0.76, 0.65, 0.15};
    colorTracksFill[] = {0.84, 0.76, 0.65, 1};
    colorRoads[] = {0.7, 0.7, 0.7, 1};
    colorRoadsFill[] = {1, 1, 1, 1};
    colorMainRoads[] = {0.9, 0.5, 0.3, 1};
    colorMainRoadsFill[] = {1, 0.6, 0.4, 1};
    colorGrid[] = {0.1, 0.1, 0.1, 0.6};
    colorGridMap[] = {0.1, 0.1, 0.1, 0.6};
    colorTrails[] = {0.84, 0.76, 0.65, 0.15};
    colorTrailsFill[] = {0.84, 0.76, 0.65, 0.65};
    colorMountPoint[] = {0.45, 0.4, 0.25, 1};
    widthRailWay = 4;

    //--- Place names. Vanilla is near-black {0, 0, 0, 1} and Expansion barely changes it
    //--- ({0.1, 0.1, 0.1, 0.9}) - both are tuned for a pale contour map and are hard to read over
    //--- satellite imagery, which was the second thing reported once satellite worked.
    //---
    //--- Amber, because MapDefaults exposes NO outline or shadow key for names - only fontNames,
    //--- sizeExNames and colorNames - so the usual dark-halo-on-light-text trick is unavailable and
    //--- contrast has to come from the colour alone. Neither black nor white survives Sakhal, which
    //--- has both snowfields and dark volcanic rock and forest. Amber separates by HUE instead of
    //--- brightness: nothing in natural satellite imagery is saturated amber, so it holds up over
    //--- snow, forest, rock and sea alike. Slightly larger for the same reason.
    //---
    //--- If this ever still reads badly, the only remaining levers are size and alpha. Do not reach
    //--- for an outline; there isn't one.
    colorNames[] = {1, 0.82, 0.35, 1};
    sizeExNames = 0.045;
};

//--- NOTHING BELOW THIS LINE. In particular, DO NOT ADD `class RscMapControl`.
//--- Vanilla declares it as `class RscMapControl: MapDefaults {};` and it inherits everything
//--- above. Redeclaring it here - with or without a parent - is what froze the client, and the
//--- proof is in the header. Everything this addon needs goes in MapDefaults.
