#ifndef SERVER
modded class DayZPlayerImplement
{
	int position_top = -1;

	void ShowDeadScreen(bool show, float duration)
	{
		if (show && IsPlayerSelected())
		{
			string message = "";
			if (!GetGame().GetMission().IsPlayerRespawning())
			{
				message = "You finished in position #" + position_top + "!";
			}

			GetGame().GetUIManager().ScreenFadeIn(duration, message, FadeColors.BLACK, FadeColors.WHITE);
		}
		else
		{
			super.ShowDeadScreen(show, duration);
		}
	}
}
