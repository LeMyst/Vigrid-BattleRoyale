#ifndef SERVER
#ifdef DIAG_DEVELOPER
modded class PluginDiagMenuClient
{
	override protected void BindCallbacks()
	{
		super.BindCallbacks();

		DiagMenu.BindCallback(m_BRDiagDummyID, CBBRDiagDummy);
		DiagMenu.BindCallback(m_BRDiagSpectateID, CBBRDiagSpectate);
	}

	static void CBBRDiagDummy(int value)
	{
		BattleRoyaleUtils.Trace("CBBRDiagDummy: " + value);
	}

	static void CBBRDiagSpectate(int value)
	{
		BattleRoyaleUtils.Trace("CBBRDiagSpectate: " + value);

		GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "SpectateRandom", NULL, true, NULL, GetGame().GetPlayer() );
	}
}