#ifndef SERVER
/**
 *  Pointer handling for the spawn-selection map.
 *
 *  MapWidget pans and zooms natively, so this handler adds clicks WITHOUT touching OnMouseWheel or
 *  the drag - overriding the wheel kills zoom outright. Extra/Map/'s VigridMapMenuHandler is the
 *  same construct for the same reason; the two are deliberate copies, since that addon may not
 *  reference a BattleRoyale symbol.
 *
 *  Both guards below are load-bearing:
 *
 *    - the press-position test rejects a press-move-release, which is a PAN. Without it every drag
 *      of the map selects a spawn where the drag ended. It is exact on purpose - a tolerance would
 *      have to be larger than a pan's first frame of travel to be worth anything, and that is
 *      already several pixels;
 *    - the debounce swallows the second half of a double-click, which would otherwise select twice.
 */
class BRMapHandler : ScriptedWidgetEventHandler
{
	protected Widget m_Root;

	protected int last_click_on_map;
	protected vector v_StartClickPos;

	void BRMapHandler(Widget w)
	{
		m_Root = w;
		m_Root.SetHandler(this);
		last_click_on_map = 0;
	}

	// When the player clicks on the map
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		v_StartClickPos = Vector(x, y, 0);

		return super.OnMouseButtonDown(w, x, y, button);
	}

	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		//--- Selecting is a LEFT click. Without this the right button - which is how the map is
		//--- panned - armed and fired a spawn selection too.
		if (button != MouseState.LEFT)
			return super.OnMouseButtonUp(w, x, y, button);

		if (GetGame().GetTime() - last_click_on_map < BR_MAP_CLICK_DEBOUNCE_MS || v_StartClickPos != Vector(x, y, 0))
		{
			return super.OnMouseButtonUp(w, x, y, button);
		}
		last_click_on_map = GetGame().GetTime();

		BattleRoyaleUtils.Trace("BRMapHandler::OnMouseButtonUp");

		SpawnSelectionMenu m = SpawnSelectionMenu.Cast(g_Game.GetUIManager().FindMenu(MENU_SPAWN_SELECTION));
		if (!m)
			return super.OnMouseButtonUp(w, x, y, button);

		m.SelectSpawnPoint(Vector(x, y, 0));

		return super.OnMouseButtonUp(w, x, y, button);
	}
};
