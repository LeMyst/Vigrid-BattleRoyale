#ifdef SERVER
class BattleRoyaleSpawnSelection: BattleRoyaleState
{
	int i_SpawnSelectionDuration = 30; // Duration in seconds
	int i_ExtraScreenTime = 3; // Extra time before the screen closes and switches to the next state
	ref Timer m_SpawnSelectionTimer;

	private BattleRoyaleConfig m_Config;
    private BattleRoyaleGameData m_GameSettings;

    void BattleRoyaleSpawnSelection()
    {
    	m_Config = BattleRoyaleConfig.GetConfig();

        m_GameSettings = m_Config.GetGameData();
    }

    override void Activate()
    {
        super.Activate();

        // Disable user input on all clients (is this needed?)
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetInput", new Param1<bool>(true), true);

        // Send RPC to all players to show spawn selection UI
        ref BattleRoyalePlayArea spawn_area = BattleRoyaleZone.GetZone(GetDynamicStartingZone(m_Players.Count())).GetArea();
        vector v_FirstZoneCenter = spawn_area.GetCenter();
        float f_FirstZoneRadius = spawn_area.GetRadius();
        float f_SpawnSelectionRadius = m_GameSettings.spawn_selection_radius;

        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "ShowSpawnSelection", new Param4<int, float, vector, float>(i_SpawnSelectionDuration, f_SpawnSelectionRadius, v_FirstZoneCenter, f_FirstZoneRadius), true);

        // Add timer to deactivate this state after a certain time
        m_SpawnSelectionTimer = AddTimer(i_SpawnSelectionDuration + i_ExtraScreenTime, this, "OnSpawnSelectionTimeout", NULL, false);

        // Listen to player spawn selection
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "OnPlayerSpawnSelected", this);
    }

    override void Deactivate()
    {
        // Stop the spawn selection timer if it's running
        if (m_SpawnSelectionTimer && m_SpawnSelectionTimer.IsRunning())
		{
			m_SpawnSelectionTimer.Stop();
		}

		// Remove the RPC listener
		GetRPCManager().RemoveRPC( RPC_DAYZBRSERVER_NAMESPACE, "OnPlayerSpawnSelected" );

		// Send RPC to all players to hide spawn selection UI
		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "HideSpawnSelection", NULL, true);

        super.Deactivate();
    }

    override bool IsComplete()
    {
        return super.IsComplete();
    }

    override string GetName()
    {
        return "Spawn Selection State";
    }

    void OnSpawnSelectionTimeout()
	{
		// Deactivate this state and move to the next state
		Deactivate();
	}

	void OnPlayerSpawnSelected(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<vector> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ PLAYER SPAWN SELECTION RPC");
			return;
		}
		if ( type == CallType.Server )
		{
			BattleRoyaleUtils.Trace("Player selected spawn point: " + data.param1.ToString());
			PlayerBase pbTarget = PlayerBase.Cast(target);
			if(pbTarget)
			{
				BattleRoyaleUtils.Trace("Player " + pbTarget.GetIdentity().GetName() + " selected spawn point: " + data.param1.ToString());
				pbTarget.SetSpawnPos(data.param1); // Set the spawn position for the player

#ifdef SCHANAMODPARTY
				BattleRoyaleUtils.Trace("Test if player is in a group");
				if(GetGroup(pbTarget))
				{
					BattleRoyaleUtils.Trace("Player " + pbTarget.GetIdentity().GetName() + " is in a group, sharing spawn point with group");

					array<PlayerBase> groupMembers = GetGroup(pbTarget).GetMembers();
					foreach (PlayerBase member : groupMembers)
					{
						if (member != pbTarget)
						{
							GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "ShareSpawnPoint", new Param2<PlayerBase, vector>(pbTarget, data.param1), true, member.GetIdentity(), member);
						}
					}
				}
				else
				{
					BattleRoyaleUtils.Trace("Player " + pbTarget.GetIdentity().GetName() + " is not in a group, not sharing spawn point");
				}
#endif
			}
			else
			{
				BattleRoyaleUtils.Trace("Player is NULL in OnPlayerSpawnSelected");
			}
		}
	}
}
