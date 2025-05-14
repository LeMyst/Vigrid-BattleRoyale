#ifndef SERVER
modded class PluginDiagMenu
{
	protected int m_BRDiagRootMenuID;
	protected int m_BRDiagDummyID;

	override protected void RegisterModdedDiagsIDs()
	{
		super.RegisterModdedDiagsIDs();

		m_BRDiagRootMenuID = GetModdedDiagID();
		m_BRDiagDummyID = GetModdedDiagID();
	}

	override protected void RegisterModdedDiags()
	{
		super.RegisterModdedDiags();

		DiagMenu.RegisterMenu(m_BRDiagRootMenuID, "BattleRoyale - Myst", GetModdedRootMenu());
		DiagMenu.RegisterBool(m_BRDiagDummyID, "", "Dummy Option", m_BRDiagRootMenuID);
	}
}