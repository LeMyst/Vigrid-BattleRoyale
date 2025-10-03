modded class InGameMenu
{
	override protected void OpenFeedback()
	{
		GetGame().OpenURL( GITHUB_URL );
	}
}