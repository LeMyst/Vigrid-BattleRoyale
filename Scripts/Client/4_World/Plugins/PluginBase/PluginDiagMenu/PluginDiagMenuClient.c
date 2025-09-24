#ifndef SERVER
#ifdef DIAG_DEVELOPER
modded class PluginDiagMenuClient
{
	static Basic_Zone_int m_BlueZone_int;
	static Basic_Zone_ext m_BlueZone_ext;

	override protected void BindCallbacks()
	{
		super.BindCallbacks();

		DiagMenu.BindCallback(m_BRDiagDummyID, CBBRDiagDummy);
		DiagMenu.BindCallback(m_BRDiagRangeID, CBBRDiagRange);
	}

	static void CBBRDiagDummy(int value)
	{
		BattleRoyaleUtils.Trace("CBBRDiagDummy: " + value);
	}

	static void CBBRDiagRange(int value)
	{
		Print("CBBRDiagRange: " + value);
		BattleRoyaleUtils.Trace("CBBRDiagRange: " + value);

		DayZPlayer player = GetGame().GetPlayer();
		vector pos = player.GetPosition();

		if ( !m_BlueZone_int )
		{
			BattleRoyaleUtils.Trace("Creating blue zone int");
			m_BlueZone_int = Basic_Zone_int.Cast( GetGame().CreateObjectEx( "Basic_Zone_int", pos, ECE_LOCAL|ECE_PLACE_ON_SURFACE ) );
		}

		// Update blue zone scale and position
		if ( m_BlueZone_int )
		{
			// The default scale is 10 meters radius
			BattleRoyaleUtils.Trace("Blue zone scale: " + value / 10);
			m_BlueZone_int.SetScale( value / 10 );
			BattleRoyaleUtils.Trace("Blue zone position: " + pos);
			m_BlueZone_int.SetPosition( pos );
			m_BlueZone_int.Update();
		}

		if ( !m_BlueZone_ext )
		{
			BattleRoyaleUtils.Trace("Creating blue zone ext");
			m_BlueZone_ext = Basic_Zone_ext.Cast( GetGame().CreateObjectEx( "Basic_Zone_ext", pos, ECE_LOCAL|ECE_PLACE_ON_SURFACE ) );
		}

		// Update blue zone scale and position
		if ( m_BlueZone_ext )
		{
			// The default scale is 10 meters radius
			BattleRoyaleUtils.Trace("Blue zone scale: " + value / 10);
			m_BlueZone_ext.SetScale( value / 10 );
			BattleRoyaleUtils.Trace("Blue zone position: " + pos);
			m_BlueZone_ext.SetPosition( pos );
			m_BlueZone_ext.Update();
		}
	}
}