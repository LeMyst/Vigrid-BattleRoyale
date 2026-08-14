#ifndef SERVER
/**
 *  Uppercase placeholder tokens that stand in for a live keybind inside a localised string.
 *
 *  A stringtable row cannot know which key an input is bound to - the player may have rebound it -
 *  so the rows carry a token (READY_KEY, MAP_KEY, ...) and the client swaps in the real button name
 *  at the moment the text is shown. BattleRoyaleRPC.NotificationMessage had this inline for two
 *  tokens; the loading-screen hints need nine, so it lives here and both call sites share it.
 *
 *  Every token resolves through a fallback, because the lookup can legitimately come up empty:
 *
 *  - InputUtils.GetButtonNameFromInput dereferences GetUApi().GetInputByName() with NO null check
 *    (P:\scripts\3_game\tools\inpututils.c:17), so asking for an input that does not exist - a MAP_KEY
 *    token on a server built without Extra/Map - would fault rather than return "". We check first.
 *  - The loading screen's hint panel is built from inside the DayZGame constructor, where GetUApi()
 *    readiness is not something we can rely on. The first populate may fall back; every later page
 *    turn re-resolves, so a live bind is picked up as soon as one is available.
 *
 *  The fallback string is the key from this mod's shipped Inputs.xml presets, so the worst case is
 *  a hint that names the default key rather than the rebound one - never a raw READY_KEY on screen.
 */
class BattleRoyaleKeyToken
{
    string token;       //placeholder as it appears in the stringtable row
    string input_name;  //UA input to read the bind from
    string fallback;    //shipped default, used when the input cannot be resolved

    void BattleRoyaleKeyToken(string a_token, string a_input_name, string a_fallback)
    {
        token = a_token;
        input_name = a_input_name;
        fallback = a_fallback;
    }
}

class BattleRoyaleKeyTokens
{
    //--- ORDER MATTERS. Substitution is a plain string replace, so a token that is a substring of
    //--- another has to come second: MAP_KEY is a substring of MINIMAP_KEY, and replacing MAP_KEY
    //--- first would turn "MINIMAP_KEY" into "MINIM" plus a key name.
    static ref array<ref BattleRoyaleKeyToken> s_Tokens;

    static void Init()
    {
        if (s_Tokens)
            return;

        s_Tokens = new array<ref BattleRoyaleKeyToken>;

        //--- Battle Royale (Data/Inputs.xml). The admin spectate set is deliberately absent - these
        //--- tokens are used by player-facing text, and F3/F5/F6 are refused for anyone outside
        //--- admins_steamid64 anyway.
        s_Tokens.Insert(new BattleRoyaleKeyToken("READY_KEY",       "UADayZBRReadyUp",             "F1"));
        s_Tokens.Insert(new BattleRoyaleKeyToken("UNSTUCK_KEY",     "UADayZBRUnstuck",             "F2"));
        s_Tokens.Insert(new BattleRoyaleKeyToken("LEADERBOARD_KEY", "UADayZBRLeaderboard",         "F4"));

        //--- Extra/Map. MINIMAP_KEY before MAP_KEY, see the note above.
        s_Tokens.Insert(new BattleRoyaleKeyToken("MINIMAP_KEY",     "UAVigridMapMinimapToggle",    "N"));
        s_Tokens.Insert(new BattleRoyaleKeyToken("COMPASS_KEY",     "UAVigridMapCompassToggle",    "K"));
        s_Tokens.Insert(new BattleRoyaleKeyToken("MAP_KEY",         "UAVigridMapToggle",           "M"));

        //--- Party. PING_CLEAR_KEY before PING_KEY for the same reason.
        s_Tokens.Insert(new BattleRoyaleKeyToken("PING_CLEAR_KEY",  "UAVigridPartyPingClear",      "Y"));
        s_Tokens.Insert(new BattleRoyaleKeyToken("PING_KEY",        "UAVigridPartyPing",           "T"));
        s_Tokens.Insert(new BattleRoyaleKeyToken("PARTY_KEY",       "UAVigridPartyMenu",           "P"));
    }

    //! Live button name for one token, or its shipped default when the input cannot be resolved.
    //! b_Fallback reports which of the two happened - see LogResolution for why that matters.
    //! It is a second return value in all but name, and Substitute clobbers it on every token, so
    //! NEVER read it anywhere but on the line immediately after a ResolveKey call.
    static bool b_Fallback;

    static string ResolveKey(BattleRoyaleKeyToken entry)
    {
        b_Fallback = true;

        if (!entry)
            return "";

        if (!GetUApi())
            return entry.fallback;

        UAInput ua_input = GetUApi().GetInputByName(entry.input_name);
        if (!ua_input)
            return entry.fallback;

        string button = InputUtils.GetButtonNameFromInput(entry.input_name, EInputDeviceType.MOUSE_AND_KEYBOARD);
        if (button == "")
            return entry.fallback;

        b_Fallback = false;
        return button;
    }

    //--- Every fallback string is deliberately identical to the shipped Inputs.xml preset, which
    //--- makes the two paths INDISTINGUISHABLE on screen for a player who has rebound nothing: a
    //--- lookup that silently never worked reads exactly like one that did. It would only surface
    //--- as "the hints ignore my rebinds", which nobody reports. So say which path each token took,
    //--- once per process.
    static bool b_Logged;

    static void LogResolution()
    {
        if (b_Logged)
            return;

        b_Logged = true;
        Init();

        foreach (BattleRoyaleKeyToken entry : s_Tokens)
        {
            string resolved = ResolveKey(entry);

            string source = "live";
            if (b_Fallback)
                source = "FALLBACK";

            //Built in steps on purpose: a single expression of this shape hits the EnfusionScript
            //"Formula too complex" ceiling, and it is a hard compile error the packer does not catch.
            string line = "[Hints] " + entry.token;
            line = line + " -> " + resolved;
            line = line + " (" + source + ", " + entry.input_name + ")";
            BattleRoyaleUtils.Debug(line);
        }
    }

    //! Replace every known token in an ALREADY LOCALISED string. Passing a raw "#STR_..." key does
    //! nothing useful - the tokens live inside the translation, not in the key.
    static string Substitute(string text)
    {
        if (text == "")
            return text;

        Init();

        string result = text;
        foreach (BattleRoyaleKeyToken entry : s_Tokens)
        {
            if (!result.Contains(entry.token))
                continue;

            result.Replace(entry.token, ResolveKey(entry));
        }

        return result;
    }

    //! Localise a "#STR_..." key (leaving a plain string alone) and then substitute.
    static string Localise(string text)
    {
        if (text == "")
            return text;

        string localised = text;
        if (localised.IndexOf("#") == 0)
            localised = Widget.TranslateString(localised);

        return Substitute(localised);
    }
}
#endif
