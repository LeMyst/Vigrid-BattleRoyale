#ifndef SERVER
/**
 *  Vigrid Map - pointer handling for the map widget.
 *
 *  MapWidget pans and zooms natively: vanilla's own MapHandler implements neither and the vanilla
 *  map still drags and wheels, and the Battle Royale spawn-selection menu relies on the same thing.
 *  So this handler must add clicks WITHOUT touching OnMouseWheel or the drag - overriding the wheel
 *  kills zoom outright.
 *
 *  Both guards below are load-bearing, and both are copied from BRMapHandler where they were
 *  already paid for:
 *
 *    - the start-position test rejects a press-move-release, which is a pan. Without it every
 *      single pan of the map drops a marker where the drag ended;
 *    - the debounce swallows the second half of a double-click, which would otherwise place twice.
 */
class VigridMapMenuHandler : ScriptedWidgetEventHandler
{
    protected Widget m_Root;
    protected int m_LastClickMs;
    protected vector m_PressPos;

    void VigridMapMenuHandler(Widget w)
    {
        m_Root = w;
        m_Root.SetHandler(this);
        m_LastClickMs = 0;
    }

    override bool OnMouseButtonDown(Widget w, int x, int y, int button)
    {
        m_PressPos = Vector(x, y, 0);

        return super.OnMouseButtonDown(w, x, y, button);
    }

    override bool OnMouseButtonUp(Widget w, int x, int y, int button)
    {
        //--- A pan is a press, a move, then a release. Anything that moved is not a click.
        if (m_PressPos != Vector(x, y, 0))
            return super.OnMouseButtonUp(w, x, y, button);
        if (GetGame().GetTime() - m_LastClickMs < VIGRID_MAP_CLICK_DEBOUNCE_MS)
            return super.OnMouseButtonUp(w, x, y, button);

        m_LastClickMs = GetGame().GetTime();

        VigridMapMenu menu = VigridMapMenu.Cast(GetGame().GetUIManager().FindMenu(MENU_VIGRID_MAP));
        if (!menu)
            return super.OnMouseButtonUp(w, x, y, button);

        //--- Raw screen pixels, which is what ScreenToMap consumes. Converting here would mean
        //--- doing it twice, since the menu needs the widget to do the conversion anyway.
        if (button == MouseState.LEFT)
            menu.OnMapLeftClick(Vector(x, y, 0));
        else if (button == MouseState.RIGHT)
            menu.OnMapRightClick();

        return super.OnMouseButtonUp(w, x, y, button);
    }
}
#endif
