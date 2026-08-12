const int MENU_SPAWN_SELECTION = 75;
//--- Vanilla menu ids stop at 46 and the party addon claims 176, so the leaderboard takes 177.
const int MENU_BR_LEADERBOARD = 177;
//--- The death screen. A real menu rather than UIManager.ScreenFadeIn, because the engine fade is a
//--- proto native call taking a string and two colours - it has no widget tree and cannot host the
//--- Spectate / Quit buttons.
//--- 179 and NOT 178: Extra/Map on the map-system branch claims 178 as MENU_VIGRID_MAP. Both
//--- branches picked "the next free id" without seeing each other, and the collision is silent and
//--- vicious - the map keybind opened THIS menu, so pressing M showed the death screen and started
//--- its quit countdown on a living player. Ids are a single flat namespace shared by every addon
//--- in the mod, so check Party/ and Extra/ before taking one.
const int MENU_BR_DEAD = 179;
