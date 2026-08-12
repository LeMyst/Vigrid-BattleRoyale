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
			"Newsflash: bushes don’t stop bullets. (or do they?)",
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
			//--- Stand the shock handler down while the dying body is still reachable as `this`.
			//--- ShockHandler.Update() runs from the corpse's CommandHandler and re-Start()s
			//--- PPERequester_TunnelVisionEffects every frame at 1 - EaseInQuart(shock/max), which
			//--- for a knocked-out body reads ~1.0 forever - it would stomp whatever
			//--- BattleRoyaleClient.EnterSpectate() clears. Writing FULL shock makes the handler's own
			//--- "valAdjusted <= 0 -> Stop()" branch fire instead. Safer than nulling m_ShockHandler,
			//--- which EEHitBy dereferences unguarded.
			//--- SetMasterAttenuation("") in the same breath: OnUnconsciousStart set
			//--- "UnconsciousAttenuation" and only OnUnconsciousStop clears it - the same branch a
			//--- dead player never reaches - so a spectator would hear the match through the
			//--- muffled-unconscious filter.
			PlayerBase dying_self = PlayerBase.Cast( this );
			if (dying_self)
			{
				dying_self.m_CurrentShock = dying_self.GetMaxHealth( "", "Shock" );
				dying_self.SetMasterAttenuation( "" );
			}

			string placement = "";
			string funny_string = "";
			if (!GetGame().GetMission().IsPlayerRespawning())
			{
				funny_string = funny_strings.GetRandomElement();

				//--- Neither ScreenFadeIn nor TextWidget.SetText resolves a "#KEY" the way a layout's
				//--- own `text` property does, so translate and format here.
				placement = Widget.TranslateString("#STR_BR_DEAD_POSITION");
				placement = string.Format(placement, position_top);
			}

			//--- Hand the text over through BattleRoyaleRPC rather than calling a method on the menu:
			//--- this file compiles in 4_World and DeathScreenMenu lives in 5_Mission, a later stage,
			//--- so the type is not nameable from here. EnterScriptedMenu returns the 3_Game base
			//--- type, which is.
			BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();
			if (br_rpc)
			{
				br_rpc.dead_placement = placement;
				br_rpc.dead_flavour = funny_string;
			}

			//--- A real menu, not ScreenFadeIn: the engine fade has no widget tree and so cannot
			//--- carry the Spectate / Quit buttons. The menu draws its own dark-red backdrop, which
			//--- is why the fade is not called at all on this path.
			//---
			//--- DEFERRED, not opened inline. SimulateDeath calls LockControls(true) and then this,
			//--- all inside the death command handler; building a menu in the middle of that races
			//--- the focus/cursor state LockControls is still setting up, which showed up as a
			//--- flickering cursor and buttons that swallowed clicks. Vanilla never opens this screen
			//--- inline either - every one of its own ShowDeadScreen calls goes through the GUI call
			//--- queue (ingamemenu.c:354, missiongameplay.c:1623).
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).Call(OpenDeathScreen);
		}
		else
		{
			super.ShowDeadScreen(show, duration);
		}

		//--- No auto-abort any more: the death screen owns the exit now, through its Quit button, and
		//--- the server starts spectating on its own after BR_SPECTATE_ENTRY_DELAY_MS if the player
		//--- does nothing. The old unconditional 15 s CallLater(LeaveServer) also stacked one queued
		//--- abort per call and never removed any of them (TODO.md item [2]).
	}

	/**
	 *  Open the death screen, one frame after ShowDeadScreen queued us.
	 *
	 *  Idempotent: ShowDeadScreen(true, ...) can legitimately be called more than once for one death
	 *  (vanilla re-shows it from InGameMenu, for instance), and re-entering the menu would tear down
	 *  and rebuild the widget tree under the player's cursor.
	 */
	void OpenDeathScreen()
	{
		if (!GetGame() || !GetGame().GetUIManager())
			return;

		UIManager ui_manager = GetGame().GetUIManager();

		if (ui_manager.FindMenu(MENU_BR_DEAD))
			return;

		//--- EnterScriptedMenu(id, NULL) installs a ROOT menu, and the root slot holds exactly one:
		//--- with something already there the native returns NULL without ever reaching
		//--- MissionBase.CreateScriptedMenu. That is what happened to a player who died with the
		//--- escape menu open - the log had "Failed to open MENU_BR_DEAD" and no CreateScriptedMenu
		//--- line at all - leaving them with no death screen and no way into spectate.
		//--- The escape menu does close itself on death, but from OnCommandDeathStart, a frame or two
		//--- AFTER this call-queue entry runs. Vanilla guards the same call the same way: see
		//--- MissionGameplay's map (CloseAll first) and book (GetMenu() == NULL) paths.
		if (ui_manager.GetMenu())
			ui_manager.CloseAll();

		if (!ui_manager.EnterScriptedMenu(MENU_BR_DEAD, NULL))
			BattleRoyaleUtils.Warn("Failed to open MENU_BR_DEAD");
	}

	//--- LeaveServer() is gone: nothing queues it any more. The death screen's Quit button calls
	//--- AbortMission directly, which is the same thing but on the player's own timing.
}
