#ifdef SERVER
#ifdef JM_COT
/**
 *  Let the resolved name reach Community Online Tools.
 *
 *  COT captures the name once, in JMPlayerInstance's constructor
 *  (JM/COT/Scripts/4_World/.../JMPlayerInstance.c:66, `m_Name = identity.GetName()`), and every COT
 *  surface reads that one field afterwards: the player list row, the player form, ESP, the map
 *  marks, its log lines and the Steam webhook link. The capture happens at connect, seconds before
 *  a name resolves, so all of them show "Survivor".
 *
 *  Correcting the field is therefore the whole fix, and it is one write rather than an override per
 *  surface - COT also serializes m_Name to admin clients (`ctx.Write( m_Name )`), which an override
 *  of GetName() would not have fixed.
 *
 *  m_Name is `private`, which only restricts access from *outside* the class; a modded class is the
 *  same class, so it may write it - the same reason the mod can write vanilla's protected
 *  m_CachedPlayerName.
 */
modded class JMPlayerInstance
{
	void BR_SetName(string name)
	{
		m_Name = name;
	}
};
#endif
#endif
