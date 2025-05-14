#ifndef SERVER
#ifdef DIAG_DEVELOPER
modded class PluginDiagMenu
{
	protected int m_BRDiagRootMenuID;
	protected int m_BRDiagDummyID;
	protected int m_BRDiagRangeID;

	override protected void RegisterModdedDiagsIDs()
	{
		super.RegisterModdedDiagsIDs();

		m_BRDiagRootMenuID = GetModdedDiagID();
		m_BRDiagDummyID = GetModdedDiagID();
		m_BRDiagRangeID = GetModdedDiagID();
	}

	override protected void RegisterModdedDiags()
	{
		super.RegisterModdedDiags();

		DiagMenu.RegisterMenu(m_BRDiagRootMenuID, "BattleRoyale - Myst", GetModdedRootMenu());
		DiagMenu.RegisterBool(m_BRDiagDummyID, "", "Dummy Option", m_BRDiagRootMenuID);
		DiagMenu.RegisterRange(m_BRDiagRangeID, "", "Zone Range", m_BRDiagRootMenuID, "35,4500,35,5");
	}
}
