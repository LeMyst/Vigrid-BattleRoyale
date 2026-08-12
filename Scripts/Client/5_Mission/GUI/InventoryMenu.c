#ifndef SERVER
/**
 *  Stop vanilla's inventory teardown knocking the current menu off the stack when a player dies.
 *
 *  THE BUG, in vanilla. On every local death the DayZPlayer GetOnDeathStart invoker fires
 *  InventoryMenu.OnPlayerDeath (P:/scripts/5_mission/gui/inventorymenu.c:170-178), which calls
 *  super.OnPlayerDeath() -> UIScriptedMenu.Close() (uiscriptedmenu.c:608-611).
 *
 *  MissionGameplay only ever CREATES that menu, with a null parent (missiongameplay.c:212), and most
 *  of the time it is never shown - so GetParentMenu() is NULL. Closing a menu assigns the engine's
 *  m_pCurrentMenu from that parent, so closing a parentless one nulls it, taking whatever genuinely
 *  WAS on top down with it.
 *
 *  Bohemia know about this: MissionGameplay.DestroyInventory (missiongameplay.c:1186-1196) does
 *
 *      if (!m_InventoryMenu.GetParentMenu() && GetUIManager().GetMenu() != m_InventoryMenu)
 *          m_InventoryMenu.SetParentMenu(GetUIManager().GetMenu()); //hack; guarantees the
 *                                                                   //'m_pCurrentMenu' will be set
 *                                                                   //to whatever is on top currently
 *
 *  ...and then closes it. OnPlayerDeath has no such guard. This applies the same one.
 *
 *  WHY THIS MOD CARES. The Battle Royale death screen (MENU_BR_DEAD) opens a few milliseconds
 *  earlier in the same death sequence, so it is what gets knocked off. Its widget tree was parented
 *  to the workspace by CreateWidgets, so it keeps drawing and still LOOKS open - but the engine
 *  ticks and routes input only to m_pCurrentMenu, so Update() stops, OnClick is never dispatched,
 *  the cursor is never re-asserted, and FindMenu(MENU_BR_DEAD) returns NULL. That is one cause for
 *  every symptom: dead buttons, a vanishing mouse, and a frozen countdown.
 *
 *  It presented as an intermittent regression because it is a race - OpenDeathScreen runs from the
 *  GUI call queue while the death invoker runs from the entity command handler, and whichever lands
 *  last wins.
 *
 *  Deliberately keeps super's teardown rather than skipping it: the inventory still closes and
 *  hides exactly as vanilla intends. Only the stale current-menu pointer is corrected.
 */
modded class InventoryMenu
{
    override void OnPlayerDeath()
    {
        UIManager ui_manager = GetGame().GetUIManager();

        if (ui_manager && !GetParentMenu() && ui_manager.GetMenu() != this)
            SetParentMenu(ui_manager.GetMenu());

        super.OnPlayerDeath();
    }
}
#endif
