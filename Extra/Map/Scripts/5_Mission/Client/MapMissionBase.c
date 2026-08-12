#ifndef SERVER
/**
 *  Vigrid Map - menu registration.
 *
 *  The third override of CreateScriptedMenu in this mod set: the Battle Royale mod has one for the
 *  spawn selection screen and the party addon has one for the party menu. All three fall through to
 *  super when the id is not theirs, which is what lets them coexist - if any one of them ever stops
 *  calling super, the others' menus silently stop opening.
 */
modded class MissionBase
{
    override UIScriptedMenu CreateScriptedMenu(int id)
    {
#ifndef NO_GUI
        if (id == MENU_VIGRID_MAP)
        {
            VigridMapLog.Trace("CreateScriptedMenu: MENU_VIGRID_MAP");
            UIScriptedMenu menu = new VigridMapMenu();
            menu.SetID(id);
            return menu;
        }
#endif

        return super.CreateScriptedMenu(id);
    }
}
#endif
