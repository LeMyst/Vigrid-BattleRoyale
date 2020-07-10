
class BattleRoyaleDebug extends BattleRoyaleState {
	
	protected vector v_Center;
	protected float f_Radius;
	
	void BattleRoyaleDebug()
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleDebug::Constructor()");
		#endif
		
		v_Center = "14829.2 72.3148 14572.3";
		f_Radius = 50;
	}
	
	//returns true when this state is complete
	override void IsComplete()
	{
		if(!IsActive())
			return true;
		
		if(GetPlayers().Count() > 9)
			return true;
		
		return false;
	}
	
	vector GetCenter()
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleDebug::GetCenter()");
		#endif
		
		return v_Center;
	}
	float GetRadius()
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleDebug::GetRadius()");
		#endif
		
		return f_Radius;
	}
	
	override array<PlayerBase> RemoveAllPlayers()
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleDebug::RemoveAllPlayers()");
		#endif
		
		array<PlayerBase> players = super.RemoveAllPlayers();
		foreach(PlayerBase player : players)
		{
			player.SetAllowDamage(true); //leaving debug state = disable god mode
		}
		return players;
	}
	override void RemovePlayer(PlayerBase player)
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleDebug::RemovePlayer()");
		#endif
		
		if(player)
			player.SetAllowDamage(true); //leaving debug state = disable god mode
		super.RemovePlayer(player);
	}
	override void AddPlayer(PlayerBase player)
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleDebug::AddPlayer()");
		#endif
		
		if(player)
			player.SetAllowDamage(false); //all players in debug are god mode TODO: ensure that dayz expansion doesn't override this
		super.AddPlayer(player);
	}
	
	override void OnPlayerTick(PlayerBase player, float timeslice)
	{
	
		vector playerPos = player.GetPosition();
		float distance = vector.Distance(playerPos, v_Center);
		if(distance > f_Radius)
		{
			#ifdef BR_BETA_LOGGING
			BRPrint("BattleRoyaleDebug::OnPlayerTick() => Snapping player back to debug zone");
			#endif
			player.SetPosition(v_Center);
		}

	}
}