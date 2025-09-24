#ifndef SERVER
#ifndef NO_GUI
modded class DayZPlayerImplement
{
	int position_top = -1;

	override void ShowDeadScreen(bool show, float duration)
	{
        array<string> funny_strings = {
			"Next time, try to aim better...",
			"Congrats, you just found out bullets don’t mix well with you.",
			"Looks like your battle royale career just hit a respawn checkpoint.",
			"Newsflash: bushes don’t stop bullets. (or are they?)",
			"Respawn isn’t a strategy, it’s your lifestyle.",
			"Running in a straight line? Bold choice. Stupid, but bold.",
			"Forget kills, you’ve mastered the art of dying creatively.",
			"Reloading is important — you’d know if you lived long enough to try it.",
			"Loot all you want; it won’t save you from incompetence.",
			"Karma shoots faster than you — and with way better aim.",
			"You've been royally outplayed! Time to queue up for another shot.",
			"Well, that didn't go as planned. Don't worry, the battleground always gives second chances... sort of.",
			"You thought he was aiming at the sky? Nope, it was at you.",
        };

		if (show && IsPlayerSelected())
		{
			string message = "";
			if (!GetGame().GetMission().IsPlayerRespawning())
			{
				string funny_string = funny_strings.GetRandomElement();
				message = "You failed in position #" + position_top + "!\r\n" + funny_string;
			}

			GetGame().GetUIManager().ScreenFadeIn(duration, message, FadeColors.DARK_RED, FadeColors.WHITE);
		}
		else
		{
			super.ShowDeadScreen(show, duration);
		}

		// 15s timer before leaving server
		if (show)
		{
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(LeaveServer, 15000, false);
		}
	}

	void LeaveServer()
	{
		GetGame().GetMission().AbortMission();
	}
}
