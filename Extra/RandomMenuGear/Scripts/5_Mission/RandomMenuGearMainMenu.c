/**
 *  Re-rolls the outfit every time the main menu is shown - coming back from the server browser,
 *  options, credits, the character menu, or a disconnect. Vanilla OnShow() calls
 *  OnChangeCharacter(false), which does not recreate the character, so the IntroSceneCharacter hook
 *  never fires on this path and the gear would otherwise stay put.
 *
 *  This chains cleanly onto the mod's own `modded class MainMenu`
 *  (Scripts/Client/5_Mission/Mission/MainMenu.c) - that one does not override OnShow().
 */
modded class MainMenu
{
	override void OnShow()
	{
		super.OnShow();

		if (!m_ScenePC)
			return;

		if (!m_ScenePC.GetIntroCharacter())
			return;

		RandomMenuGear.Apply(m_ScenePC.GetIntroCharacter().GetCharacterObj());
	}
}
