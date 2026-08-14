#ifndef SERVER
/**
 *  Replace vanilla's hint pages with this mod's own.
 *
 *  Vanilla loads scripts/data/hints.json and rotates a page every 25 s on the loading screen, plus
 *  one more each time LoadProgressUpdate reports PROGRESS_START (dayzgame.c:2041). All of that
 *  machinery - the slideshow, the prev/next arrows, the "n / m" paging label, the styled panel - is
 *  worth keeping, so only the data source and the text formatting are overridden.
 *
 *  This file lives in 3_Game, NOT 5_Mission where the commented-out original sat. UiHintPanel is
 *  declared in 3_Game and the loading screen's panel is constructed from inside the DayZGame
 *  constructor (dayzgame.c:841), long before any mission module is loaded.
 *
 *  UiHintPanel rather than UiHintPanelLoading, so the in-game Esc menu's panel (ingamemenu.c:49)
 *  gets the same content. That is what the original did.
 */
modded class UiHintPanel
{
    protected override void LoadContentList()
    {
        array<ref HintPage> pages = new array<ref HintPage>;

        LoadHintFile(DAYZBR_HINTS_PATH, pages);

        //--- A hint must never name a key that does not exist, so the addon-specific pages only
        //--- come along when their addon is compiled in. Same guard idiom as every VigridMapAPI /
        //--- VigridPartyAPI call site.
        #ifdef VIGRID_MAP
        LoadHintFile(DAYZBR_HINTS_MAP_PATH, pages);
        #endif

        #ifdef VIGRID_PARTY
        LoadHintFile(DAYZBR_HINTS_PARTY_PATH, pages);
        #endif

        //--- Fewer than two pages HANGS THE GAME. RandomizePageIndex spins on
        //--- `while (m_PageIndex == m_PreviousRandomIndex)`, which a one-entry list can never
        //--- satisfy once that entry has been shown once. Fall back to vanilla rather than ship a
        //--- deadlock: the JSON is packed content, so getting here at all means something is wrong
        //--- with the build.
        if (pages.Count() < 2)
        {
            BattleRoyaleUtils.Warn(string.Format("[Hints] only %1 mod hint page(s) loaded, falling back to the vanilla hints", pages.Count()));
            super.LoadContentList();
            return;
        }

        BattleRoyaleUtils.Debug(string.Format("[Hints] loaded %1 mod hint pages", pages.Count()));
        m_ContentList = pages;
    }

    //! Append one JSON file's pages. A missing or malformed file warns and contributes nothing,
    //! which the caller's page-count check then turns into a clean fallback.
    protected void LoadHintFile(string path, notnull array<ref HintPage> into)
    {
        array<ref HintPage> loaded;
        string error_message;

        if (!JsonFileLoader<array<ref HintPage>>.LoadFile(path, loaded, error_message))
        {
            BattleRoyaleUtils.Warn("[Hints] " + error_message);
            return;
        }

        if (!loaded)
            return;

        foreach (HintPage page : loaded)
        {
            if (page)
                into.Insert(page);
        }
    }

    //--- The two setters below differ from vanilla only in routing the text through
    //--- BattleRoyaleKeyTokens. The rows are stringtable keys, and the keybind placeholders live
    //--- INSIDE the translation, so the key has to be resolved in script before the token can be
    //--- found - SetText("#STR_...") would localise too late and print a raw READY_KEY. Doing it
    //--- per populate rather than once at load is also what lets a rebind take effect.

    protected override void SetHintHeadline()
    {
        m_UiHeadlineLabel.SetText(BattleRoyaleKeyTokens.Localise(m_ContentList.Get(m_PageIndex).GetHeadlineText()));
    }

    protected override void SetHintDescription()
    {
        //--- Once per process, from the same context the real substitution runs in - the first
        //--- populate happens inside the DayZGame constructor, which is the one place GetUApi()
        //--- readiness was in doubt.
        BattleRoyaleKeyTokens.LogResolution();

        string description = BattleRoyaleKeyTokens.Localise(m_ContentList.Get(m_PageIndex).GetDescriptionText());

        //--- Neither half of this is checkable by reading the code: a stringtable row that failed to
        //--- load comes back as the raw "#STR_BR_HINT_..." key, and an unresolved keybind comes back
        //--- as a bare READY_KEY. Both render as perfectly ordinary-looking text. One trace line
        //--- distinguishes them from a correct page without anyone having to squint at a loading
        //--- screen. Trace, so it is on in a diag build and off on a live server.
        BattleRoyaleUtils.Trace("[Hints] page " + m_PageIndex.ToString() + ": " + description);

        m_UiDescLabel.SetText(description);
        m_UiDescLabel.Update();
        m_SpacerFrame.Update();
    }
}
#endif
