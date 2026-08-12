/**
 *  Vigrid Map Satellite - turns on the satellite layer of the in-game map, retunes the overlay
 *  layers so they work over photographic imagery, and makes place names legible over it.
 *
 *  THE ONE RULE: do not add a `class RscMapControl` here. Doing that is what hard-froze every client
 *  on 2026-08-07 - proven by bisection, see below - and it is the only thing about this file that is
 *  dangerous. The values themselves are not.
 *
 *  WHY THIS EXISTS. The satellite map everyone was used to came from @DayZ-Expansion-Navigation, not
 *  from the terrain. Vanilla ships maxSatelliteAlpha = 0, which disables the satellite layer outright
 *  and leaves the procedurally drawn contour map. So unloading Navigation, which the Extra/Map addon
 *  makes possible, silently swapped the map back to topo.
 *
 *  Note the satellite imagery is the TERRAIN's own, already shipped with the world
 *  (dz\worlds\<world>\data\layers\s_XXX_YYY_lco.paa). Nothing is bundled here.
 *
 *  ------------------------------------------------------------------------------------------------
 *  PROVENANCE - READ BEFORE ADDING A VALUE
 *  ------------------------------------------------------------------------------------------------
 *
 *  EVERY VALUE IN THIS FILE IS DERIVED FROM VANILLA, NOT FROM EXPANSION. Earlier revisions of this
 *  file were Expansion Navigation's block "copied whole". Expansion's source is licensed
 *  CC BY-NC-ND 4.0 ((c) DayZ Expansion Mod Team) - the NoDerivatives term is squarely at odds with
 *  copying their config and altering values in it, and NonCommercial matters the moment a server
 *  takes donations. So the block was re-derived from Bohemia's own config in 2026-08-09.
 *
 *  None of Expansion's values were needed. What was actually wanted from them was a FACT about the
 *  engine - where the place-name shadow key lives - and a TECHNIQUE, "zero the alpha on the fill
 *  layers that fight a photo, keep the linear features that still read". Neither is anyone's
 *  expression. Both are applied below to vanilla's own hues.
 *
 *  THE DERIVATION RULE, which every colour below obeys:
 *
 *      KEEP BOHEMIA'S RGB. CHANGE ONLY THE ALPHA, AND ONLY WHERE A LAYER OCCLUDES THE IMAGERY.
 *
 *  Follow it if you add one. It is what keeps this file ours, and it makes the diff against
 *  P:\DZ\data\config.cpp:1717 reviewable at a glance - every line here should differ from vanilla in
 *  the fourth component and nowhere else.
 *
 *  Anything vanilla already gets right is simply NOT RESTATED. Roads, tracks, rails, power lines,
 *  the grid, colorOutside, colorMountPoint, the ptsPerSquare* tessellation densities, the fonts, the
 *  Legend block and the icon subclasses are all absent on purpose - they are inherited untouched.
 *  A shorter file is a safer one here.
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
 *  That second refutation is why scaleMin/scaleMax/scaleDefault are no longer set at all: vanilla
 *  does not declare them, VigridMapMenu.ClampZoom() re-clamps GetScale() to 0.06 .. 1.0 every frame
 *  regardless, and a build has already proven their absence harmless. Three keys we do not have to
 *  own.
 *
 *  Also ruled out: raw data volume (Sakhal has the SMALLEST satellite set of the three terrains -
 *  1024 tiles / 90.6 MB vs Chernarus 1849 / 232.5 MB, all the same DXT1 .paa format); Sakhal being
 *  special; and DayZDiag being special.
 *
 *  ------------------------------------------------------------------------------------------------
 *  WHERE THE SETTINGS LIVE
 *  ------------------------------------------------------------------------------------------------
 *
 *  Two global, top-level classes, both OUTSIDE CfgWorlds:
 *
 *      P:\DZ\data\config.cpp:1717            class MapDefaults        - the raster and vector layers
 *      P:\DZ\gear\navigation\config.cpp:14   class CfgLocationTypes   - the place-name labels
 *
 *  Nothing in a CfgWorlds entry names a map-control class, so there is no per-world variant to
 *  override: the blocks below cover EVERY terrain, and reach every MapWidget in the game - this
 *  mod's fullscreen map, its HUD minimap, the BR HUD minimap and the spawn-selection screen.
 *
 *  (An even earlier version of this file patched CfgWorlds <World> instead. That was inert - it wrote
 *  keys nothing reads. Recorded so it is not rediscovered.)
 *
 *  WHY IT IS A RENAME AND NOT A SETTING. There is no script access to any of this. MapWidget exposes
 *  no satellite API and maxSatelliteAlpha / userMapPath appear nowhere in P:\scripts - they are
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
        //--- DZ_Data declares MapDefaults; DZ_Gear_Navigation declares CfgLocationTypes. Both are
        //--- global classes, not per-terrain, so no world PBO is needed. DZ_Gear_Navigation itself
        //--- requires only DZ_Data (P:\DZ\gear\navigation\config.cpp:1-12).
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Gear_Navigation"
        };
    };
};

/**
 *  THE RASTER AND VECTOR LAYERS.
 *
 *  Diff this against P:\DZ\data\config.cpp:1717. Every colour below is vanilla's RGB with only its
 *  alpha moved - see the derivation rule in the header.
 */
class MapDefaults
{
    //--- ---------------------------------------------------------------------------------------
    //--- The satellite layer itself. This is the whole feature.
    //--- ---------------------------------------------------------------------------------------

    //--- Vanilla is 0, which disables the satellite layer outright. This single value is the addon.
    maxSatelliteAlpha = 1;

    //--- Vanilla is 1, which fades satellite out as you zoom out and falls back to the contour map.
    //--- 2 pushes the fade past the top of the scale range, so satellite renders at every zoom.
    //--- THIS IS A PACKAGE WITH THE COLOUR BLOCK BELOW: those alphas are dialled down on the
    //--- assumption that imagery is always underneath, so a fade-out would fade to a near-blank map.
    //--- Change both or neither.
    alphaFadeStartScale = 2;
    alphaFadeEndScale = 2;

    //--- The usermap is the hand-drawn paper map the terrain ships. Vanilla leaves it on at alpha 1;
    //--- compositing it under satellite gains nothing and muddies the imagery, so it is off. The
    //--- consequence to accept: satellite and usermap are alternatives here, not layers.
    //--- (userMapPath and the alphaUserMapFade* pair are already "" / 0.34 in vanilla - not restated.)
    maxUserMapAlpha = 0;

    //--- ---------------------------------------------------------------------------------------
    //--- The overlay layers. NOT decoration - these are why the satellite layer is legible.
    //--- ---------------------------------------------------------------------------------------
    //---
    //--- The map draws its procedural vector layers (forest, rocks, contours, sea) ON TOP of
    //--- whatever raster layer is underneath. Vanilla tuned them to sit over a pale contour drawing,
    //--- so several are opaque or near-opaque and simply hide the photograph. The green blocks over
    //--- forest reported 2026-08-08, on the first look at a working satellite map, were colorForest.
    //---
    //--- Fill layers that duplicate what the imagery already shows are taken to alpha 0 or near it.
    //--- Linear features that still read usefully over a photo - roads, tracks, rails, power lines,
    //--- the grid - are LEFT AT VANILLA and are not restated below.
    //---
    //--- These are draw-time tints and cannot hang anything, unlike the alpha gates above which
    //--- govern how the engine LOADS a layer. Tune them freely; just keep the RGB.

    //--- Forest: vanilla {0.36, 0.78, 0.08, 0.5}, bright green at half alpha, plus a solid green
    //--- outline. Both gone - the imagery shows the treeline far better than a flat green polygon.
    colorForest[] = {0.36, 0.78, 0.08, 0};
    colorForestBorder[] = {0.4, 0.8, 0, 0};

    //--- Rocks: vanilla fills at 0.5 and outlines at 1. Keep a hint of the fill, drop the outline.
    colorRocks[] = {0.5, 0.5, 0.5, 0.25};
    colorRocksBorder[] = {0.5, 0.5, 0.5, 0};

    //--- Sea: vanilla is FULLY OPAQUE and hides the water entirely. Enough tint left to read as
    //--- water at a glance without flattening the coastline detail.
    colorSea[] = {0.64, 0.76, 0.89, 0.4};

    //--- Contours: the minor lines are noise over a photograph, the major ones still carry the
    //--- terrain shape. Vanilla has both fully opaque.
    colorCountlines[] = {0.85, 0.8, 0.65, 0};
    colorMainCountlines[] = {0.45, 0.4, 0.25, 0.5};
    colorMainCountlinesWater[] = {0.25, 0.4, 0.5, 0.6};
    colorLevels[] = {0.65, 0.6, 0.45, 0.5};

    //--- ---------------------------------------------------------------------------------------
    //--- Grid and mountpoint labels.
    //--- ---------------------------------------------------------------------------------------
    //---
    //--- NOTE WHAT THESE DO NOT DO. colorNames / sizeExNames do NOT style town, city or village
    //--- names - those come from CfgLocationTypes below, and for a long time this file assumed
    //--- otherwise, which is why the names stayed unreadable no matter what was set here.
    //---
    //--- Amber for the same reason as the place names: it separates by HUE, not brightness, so it
    //--- survives snow, forest, rock and sea alike. Vanilla is pure black, tuned for a pale map.
    colorNames[] = {1, 0.82, 0.35, 1};
    sizeExNames = 0.045;
	class Bush
	{
		icon="\dz\gear\navigation\data\map_bush_ca.paa";
		color[]={0.14,0.25,0.09,0.80000001};
		size=14;
		importance="0.2 * 14 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class SmallTree
	{
		icon="\dz\gear\navigation\data\map_smalltree_ca.paa";
		color[]={0.14,0.25,0.09,0.80000001};
		size=12;
		importance="0.6 * 12 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Tree
	{
		icon="\dz\gear\navigation\data\map_tree_ca.paa";
		color[]={0.14,0.25,0.09,0.80000001};
		size=12;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
};

/**
 *  THE PLACE-NAME LABELS - town, city, village, and the icon markers.
 *
 *  This class is where map label styling actually lives, and finding that is what made the map
 *  readable. MapDefaults has no shadow key; CfgLocationTypes::Name does, and a drop shadow is worth
 *  more over photographic imagery than any colour choice, because it works against a light
 *  background and a dark one at the same time.
 *
 *  ONLY `Name` AND `NameIcon` ARE PATCHED, and that scoping is deliberate: both are PARENTLESS in
 *  vanilla, so there is no parent to accidentally drop - the defect that froze the client. Every
 *  other class in CfgLocationTypes derives from one of these two and inherits the changes for free.
 *
 *  IF YOU EVER TOUCH A DERIVED CLASS, RESTATE ITS PARENT - `class Village: Name`, not `class
 *  Village`. Omitting it replaces rather than merge-patches and silently strips drawStyle, texture
 *  and name. Vanilla's per-tier sizes (Capital 0.06 / City 0.05 / Village 0.04 / Local 0.03) and its
 *  importance values (7 / 6 / 5 / 2, which gate what appears at which zoom) are already sensible and
 *  are left alone.
 */
class CfgLocationTypes
{
    //--- Settlement names. Vanilla: black, no shadow, MetronBook-Bold28.
    class Name
    {
        //--- Amber, matching colorNames above so every label on the map is one colour. Neither black
        //--- nor white survives Sakhal, which has snowfields AND dark volcanic rock and forest;
        //--- nothing in natural satellite imagery is saturated amber.
        color[] = {1, 0.82, 0.35, 1};

        //--- The key this file spent a long time believing did not exist. Contrast now comes from
        //--- the shadow, and the amber above is free to be a colour rather than a contrast hack.
        shadow = 1;

        //--- Metron is a BITMAP font: raising textSize on the 28px face scales a small bitmap and
        //--- goes soft. The 58px face is the correct way to draw larger text, and it is a vanilla
        //--- asset (P:\gui\fonts\metronbook-bold58). Per-tier textSize values are inherited.
        font = "gui/fonts/MetronBook-Bold58";
    };

    //--- Icon markers (ruins, camps, hills, viewpoints, ...). Same treatment, no shadow - these are
    //--- glyphs rather than text. Note most derived classes set their own colour, so in practice
    //--- this base colour reaches `Ruin`; the font and any future key reach all of them.
    class NameIcon
    {
        color[] = {1, 0.82, 0.35, 1};
        font = "gui/fonts/MetronBook-Bold58";
    };
};

//--- NOTHING BELOW THIS LINE. In particular, DO NOT ADD `class RscMapControl`.
//--- Vanilla declares it as `class RscMapControl: MapDefaults {};` and it inherits everything
//--- above. Redeclaring it here - with or without a parent - is what froze the client, and the
//--- proof is in the header. Everything this addon needs goes in MapDefaults.
