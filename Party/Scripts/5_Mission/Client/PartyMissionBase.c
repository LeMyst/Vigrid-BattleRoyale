#ifndef SERVER
/**
 *  Vigrid Party - menu registration.
 *
 *  The Battle Royale mod also overrides CreateScriptedMenu (for the spawn selection screen). Both
 *  overrides fall through to super when the id is not theirs, which is what lets the two coexist -
 *  if either one ever stops calling super, the other's menu silently stops opening.
 */
modded class MissionBase
{
    override UIScriptedMenu CreateScriptedMenu(int id)
    {
#ifndef NO_GUI
        if (id == MENU_VIGRID_PARTY)
        {
            VigridPartyLog.Trace("CreateScriptedMenu: MENU_VIGRID_PARTY");
            UIScriptedMenu menu = new VigridPartyMenu();
            menu.SetID(id);
            return menu;
        }
#endif

        return super.CreateScriptedMenu(id);
    }
}
#endif
