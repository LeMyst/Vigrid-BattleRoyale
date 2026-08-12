/**
 *  Every menu character creation funnels through CreateNewCharacterById - both the default-character
 *  branch (CreateDefaultCharacter) and the saved-character branch (CharacterLoad). One override
 *  therefore covers the initial menu load, the prev/next character arrows, and the return from the
 *  character creation menu.
 *
 *  It does NOT cover simply re-showing the menu: MainMenu.OnShow() calls OnChangeCharacter(false),
 *  which skips creation entirely. That path is handled in RandomMenuGearMainMenu.c.
 */
modded class IntroSceneCharacter
{
	override void CreateNewCharacterById(int character_id)
	{
		super.CreateNewCharacterById(character_id);

		RandomMenuGear.Apply(GetCharacterObj());
	}
}
