#ifndef SERVER
#ifndef NO_GUI
modded class DayZPlayerImplement
{
	int position_top = -1;

	override void ShowDeadScreen(bool show, float duration)
	{
        array<string> funny_strings = {
			"Next time, try to aim better...",
			"Just so you know, you're allergic to bullets.",
			"Looks like your battle royale career just hit a respawn checkpoint.",
			"Remember, bushes aren't bulletproof. (or are they?)",
			"If at first, you don't succeed, respawn and try again.",
			"Pro tip: Running in a straight line isn't the best strategy.",
			"Remember, it's not about how many kills you get, but how stylishly you avoid death.",
			"Embrace the chaos, but don't forget to reload.",
			"Just when you thought it was safe to loot...",
			"In this game, karma has a faster firing rate than any weapon.",
			"You've been royally outplayed! Time to queue up for another shot.",
			"Well, that didn't go as planned. Don't worry, the battleground always gives second chances... sort of."
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
