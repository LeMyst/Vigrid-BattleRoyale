#ifndef SERVER
modded class PluginDiagMenuClient
{
	override protected void BindCallbacks()
	{
		super.BindCallbacks();

		DiagMenu.BindCallback(m_BRDiagDummyID, CBBRDiagDummy);
	}

	static void CBBRDiagDummy(int value)
	{
		BattleRoyaleUtils.Trace("CBBRDiagDummy: " + value);
	}
}