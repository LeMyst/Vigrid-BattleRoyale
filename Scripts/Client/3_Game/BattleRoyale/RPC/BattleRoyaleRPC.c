#ifndef SERVER
class BattleRoyaleRPC
{
	void BattleRoyaleRPC()
	{
		BattleRoyaleUtils.Trace("BattleRoyaleClient::Init");

		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetPlayerCount", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetFade", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetInput", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "AddPlayerKill", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "StartMatch", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetCountdownSeconds", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "UpdateCurrentPlayArea", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "UpdateFuturePlayArea", this );
//#ifdef SPECTATOR
//		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "ActivateSpectatorCamera", this );
//		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "UpdateEntityHealth", this );
//		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "UpdateMapEntityData", this );
//#endif
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetTopPosition", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "ShowWinScreen", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "ChatLog", this );

		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "NotificationMessage", this );

		BattleRoyaleUtils.Trace("BattleRoyaleClient::Init - Done");
	}

	void ~BattleRoyaleRPC()
	{

	}

	private static ref BattleRoyaleRPC m_Instance;
	static BattleRoyaleRPC GetInstance()
	{
		if ( m_Instance == NULL )
		{
			m_Instance = new BattleRoyaleRPC();
		}

		return m_Instance;
	}

	void Reset()
	{
		nb_players = 0;
		nb_groups = 0;
		fade_state = false;
		input_state = false;
		player_kills = 0;
		match_started = false;
		countdown_seconds = 0;
		current_play_area_center = "0 0 0";
		current_play_area_radius = 0.0;
		future_play_area_center = "0 0 0";
		future_play_area_radius = 0.0;
		top_position = 0;
		winner_screen = false;
	}

	// Set the number of players and groups

	int nb_players = 0;
	int nb_groups = 0;

	void SetPlayerCount(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Param2<int, int> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ SETPLAYERCOUNT RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace(string.Format("SetPlayerCount: %1 %2", data.param1, data.param2));
			nb_players = data.param1;
			nb_groups = data.param2;
		}
	}

	// Set the fade state

	bool fade_state = false;

	void SetFade(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Param1<bool> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ SETFADE RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace(string.Format("SetFade: %1", data.param1));
			if ( data.param1 )
			{
				fade_state = true;
			}
			else
			{
				fade_state = false;
			}
		}
	}

	// Set the input state

	bool input_state = false;

	void SetInput(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Param1<bool> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ SETINPUT RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace(string.Format("SetInput: %1", data.param1));
			input_state = data.param1;
		}
	}

	// Add a player kill

	int player_kills = 0;

	void AddPlayerKill(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Param1<int> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ ADDPLAYERKILL RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace("AddPlayerKill");
			player_kills += 1;
		}
	}

	// Start the match

	bool match_started = false;

	void StartMatch(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace("StartMatch");
			match_started = true;
		}
	}

	// Set the countdown seconds

	int countdown_seconds = 0;

	void SetCountdownSeconds(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Param1<int> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ SETCOUNTDOWNSECONDS RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace(string.Format("SetCountdownSeconds: %1", data.param1));
			countdown_seconds = data.param1;
		}
	}

	// Update the current play area

	vector current_play_area_center = "0 0 0";
	float current_play_area_radius = 0.0;

	void UpdateCurrentPlayArea(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Param2<vector, float> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ UPDATECURRENTPLAYAREA RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace(string.Format("UpdateCurrentPlayArea: %1 %2", data.param1, data.param2));
			current_play_area_center = data.param1;
			current_play_area_radius = data.param2;
		}
	}

	// Update the future play area

	vector future_play_area_center = "0 0 0";
	float future_play_area_radius = 0.0;
	bool b_ArtillerySound = false;

	void UpdateFuturePlayArea(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Param3<vector, float, bool> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ UPDATEFUTUREPLAYAREA RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace(string.Format("UpdateFuturePlayArea: %1 %2 %3", data.param1, data.param2, data.param3));
			future_play_area_center = data.param1;
			future_play_area_radius = data.param2;
			b_ArtillerySound = data.param3;
		}
	}

	// Set the top position

	int top_position = 0;

	void SetTopPosition(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Param1<int> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ SETTOPPOSITION RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace(string.Format("SetTopPosition: %1", data.param1));
			top_position = data.param1;
		}
	}

	// Show the win screen

	bool winner_screen = false;

	void ShowWinScreen(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace("ShowWinScreen");
			winner_screen = true;
		}
	}

	void ChatLog(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Param2<string, string> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ CHATLOG RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			Print("[ChatLog] " + data.param1);

			GetGame().Chat("S:" + data.param1, data.param2);
		}
	}

	void NotificationMessage(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Param7<string, float, string, string, string, string, string> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ NOTIFICATIONMESSAGE RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace(string.Format("NotificationMessage: %1 %2 %3 %4 %5 %6 %7", data.param1, data.param2, data.param3, data.param4, data.param5, data.param6, data.param7));
			StringLocaliser message = new StringLocaliser(data.param1, data.param3, data.param4, data.param5, data.param6, data.param7);
			string translated_message = message.Format();

			// Special case
			// Finish the translation client side to get the correct key
			if (translated_message.Contains("READY_KEY"))
			{
				translated_message.Replace("READY_KEY", InputUtils.GetButtonNameFromInput("UADayZBRReadyUp", EInputDeviceType.MOUSE_AND_KEYBOARD));
			}
			if (translated_message.Contains("UNSTUCK_KEY"))
			{
				translated_message.Replace("UNSTUCK_KEY", InputUtils.GetButtonNameFromInput("UADayZBRUnstuck", EInputDeviceType.MOUSE_AND_KEYBOARD));
			}

			ExpansionNotification(DAYZBR_MSG_TITLE, translated_message, DAYZBR_MSG_IMAGE, COLOR_EXPANSION_NOTIFICATION_INFO, data.param2).Create();
		}
	}
}