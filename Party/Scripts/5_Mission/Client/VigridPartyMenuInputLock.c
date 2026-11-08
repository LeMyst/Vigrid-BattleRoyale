#ifndef SERVER
/**
 *  Vigrid Party - freeze the player under a modal menu, but leave push-to-talk live.
 *
 *  A menu's own LockControls (super.OnShow) takes input focus per device, and that is NOT enough on
 *  its own: mouse look is read engine-side and kept turning the camera behind the menu, and
 *  movement stays live (vanilla lets you walk with the inventory open). Vanilla's answer is an
 *  input exclude group. Its Esc menu uses "menu", which also silences push-to-talk, so players
 *  could not talk to each other while a party or spawn screen was up.
 *
 *  {"menu_move", "movement"} is "menu" minus voice, built only from vanilla groups
 *  (P:\bin\specific.xml). "menu_move" - vanilla's own "menu with movement enabled" - covers aiming,
 *  actions, hotkeys, stances, optics, gestures, the inventory and UAGear (so Tab cannot open the
 *  inventory over the menu), but not UAVoiceOverNet; "movement" adds the player freeze. Neither
 *  names UAUIBack, F4, P or any other mod bind.
 *
 *  OWNED GROUPS ONLY. Excludes are a SET, not a counter (MissionGameplay.AddActiveInputExcludes):
 *  the first Remove clears a group no matter who added it. So Take() records only the groups it
 *  actually added, and Release() gives back exactly those - closing a menu cannot unfreeze
 *  something vanilla, or another menu, still holds.
 *
 *  Release() passes bForceSupress = true, as vanilla's Continue() does, so the key that closed the
 *  menu cannot leak through as a gameplay press.
 *
 *  A copy of the Battle Royale mod's menu input lock: Party may not reference a symbol from that
 *  mod, so it carries its own. Keep the two in step.
 */
class VigridPartyMenuInputLock
{
    protected ref array<string> m_Owned;

    void VigridPartyMenuInputLock()
    {
        m_Owned = new array<string>();
    }

    void Take()
    {
        Mission mission = GetGame().GetMission();
        if (!mission)
            return;

        //--- Already holding: a second Take() must not record groups it did not add.
        if (m_Owned.Count() > 0)
            return;

        array<string> wanted = {"menu_move", "movement"};
        foreach (string group : wanted)
        {
            if (!mission.IsInputExcludeActive(group))
                m_Owned.Insert(group);
        }

        if (m_Owned.Count() > 0)
            mission.AddActiveInputExcludes(m_Owned);
    }

    void Release()
    {
        if (m_Owned.Count() == 0)
            return;

        Mission mission = GetGame().GetMission();
        if (mission)
            mission.RemoveActiveInputExcludes(m_Owned, true);

        m_Owned.Clear();
    }
}
#endif
