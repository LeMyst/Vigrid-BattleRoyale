#ifdef SERVER
/**
 *  Every player body that died in this process, so an in-place reset can delete them all.
 *
 *  Fed from PlayerBase.EEKilled, which fires for every player death whatever happens afterwards:
 *  the victim spectating, leaving through the death screen, or the body being carried by
 *  BattleRoyaleSpectators. Deleting a body deletes its inventory too, so a victim's gear cannot
 *  survive into the next match. Today's process restart did that for free.
 *
 *  NOT `array<ref PlayerBase>`: a ref array would own the players and try to destroy them. The
 *  entries are plain references that go NULL once the engine deletes a body, which the sweep skips.
 *  Static, so it outlives the BattleRoyaleServer that an in-place reset replaces.
 *
 *  Within a match nothing changes: corpses stay where they fell, holding their loot, and lobby
 *  bodies are still removed by MissionServer.HandleBody.
 */
class BattleRoyaleCorpses
{
	protected static ref array<PlayerBase> s_Bodies;

	static void Track(PlayerBase body)
	{
		if ( !body )
			return;

		if ( !s_Bodies )
			s_Bodies = new array<PlayerBase>();

		if ( s_Bodies.Find( body ) != -1 )
			return;

		s_Bodies.Insert( body );
	}

	//! Delete every tracked body still in the world. Returns how many were deleted.
	static int DeleteAll()
	{
		if ( !s_Bodies )
			return 0;

		int deleted = 0;

		//--- Walked backwards so removing an entry never skips the next one.
		for ( int i = s_Bodies.Count() - 1; i >= 0; i-- )
		{
			PlayerBase body = s_Bodies.Get( i );
			if ( !body )
			{
				s_Bodies.Remove( i );
				continue;
			}

			//--- A body the engine still ties to a connection is kept for a later sweep: the reset
			//--- replaces those through SelectPlayer and deletes the old body itself, and a player
			//--- being kicked still owns theirs until the disconnect completes.
			if ( body.GetIdentity() )
				continue;

			//--- Already on its way out, e.g. the old body the reset itself just replaced.
			if ( body.IsSetForDeletion() )
			{
				s_Bodies.Remove( i );
				continue;
			}

			GetGame().ObjectDelete( body );
			s_Bodies.Remove( i );
			deleted++;
		}

		return deleted;
	}
}
#endif
