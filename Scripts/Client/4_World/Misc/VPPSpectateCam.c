#ifndef SERVER
/*
 * The initial code for this class was taken from the VPP-Admin-Tools mod.
 * With the permission of the author DaOne, the code was modified to fit the needs of this mod.
 * https://github.com/VanillaPlusPlus/VPP-Admin-Tools/blob/8f19c5e559e5b2b46ecb118c48c5b12432e93b5d/4_World/VPPAdminTools/Misc/VPPCameras/VPPSpectateCam.c
 */

class BRVPPSpectateCam extends Camera
{
	private vector m_y_p_r_previous;
	private vector u_d_p_r_previous;
	private float m_bblUpdateInterval = 0.0;
	private vector m_vPreviousPosition = "0 0 0";
	PlayerBase m_FollowObject;

	void BRVPPSpectateCam()
	{
		SetEventMask( EntityEvent.FRAME );
	}

	/**
	 * Called on each frame update.
	 * @param other The entity that triggered the event.
	 * @param timeSlice The time elapsed since the last frame.
	 */
	override void EOnFrame( IEntity other, float timeSlice )
	{
		if (m_FollowObject == NULL) return;
			FollowTarget3PP(timeSlice);
	}

	/**
	 * Sets the target object for the camera to follow.
	 * @param target The PlayerBase object to follow.
	 */
	void SetTargetObj(PlayerBase target)
	{
		m_FollowObject = target;
		PPEffects.ResetDOFOverride();
	}

	/**
	 * Follows the target object in a third-person perspective.
	 * @param timeSlice The time elapsed since the last frame.
	 */
	protected void FollowTarget3PP(float timeSlice)
	{
		if ( m_FollowObject == null ) return;

		float ud = m_FollowObject.GetAimingAngleUD();
		vector aimOrientation = m_FollowObject.GetOrientation();
		aimOrientation[0] = aimOrientation[0];
		aimOrientation[1] = aimOrientation[1] + ud;

		SetOrientation(Vector(m_FollowObject.GetOrientation()[0], aimOrientation.VectorToAngles()[1] ,m_FollowObject.GetOrientation()[2]));

		float f = GetOrientation()[0] * 0.017453292;
		float motionX = (double)(Math.Sin(f) * 1.0);
		float motionZ = (double)(Math.Cos(f) * 1.0);

		vector camPOS = m_FollowObject.GetPosition();
		camPOS[0] = camPOS[0] - motionX;
		camPOS[1] = m_FollowObject.GetPosition()[1] + 1.6;
		camPOS[2] = camPOS[2] - motionZ;

		SetPosition(camPOS);

		if( m_bblUpdateInterval > 0.5 )
		{
			GetGame().UpdateSpectatorPosition(GetPosition());
			BattleRoyaleUtils.Trace("SpectateCam Pos: " + GetPosition());
			m_bblUpdateInterval = 0;
		}
		m_bblUpdateInterval = m_bblUpdateInterval + timeSlice;

		float distance = vector.Distance( Vector( m_vPreviousPosition[0], 0, m_vPreviousPosition[2] ), Vector( camPOS[0], 0, camPOS[2] ) );

		if (distance > 50)
		{
			vector spawn_pos = "0 0 0";
			spawn_pos[0] = Math.RandomFloatInclusive((camPOS[0] - 5), (camPOS[0] + 5));
			spawn_pos[2] = Math.RandomFloatInclusive((camPOS[2] - 5), (camPOS[2] + 5));
			spawn_pos[1] = GetGame().SurfaceY(spawn_pos[0], spawn_pos[2]);

			// Send RPC to update player position
			BattleRoyaleUtils.Trace("SpectateCam Teleport Player to: " + spawn_pos);
			BattleRoyaleUtils.Trace("SpectateCam Distance: " + distance);
			BattleRoyaleUtils.Trace("SpectateCam Player: " + GetGame().GetPlayer());
			BattleRoyaleUtils.Trace("SpectateCam OriginalPlayer: " + GetGame().GetPlayer());
			GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "UpdateSpectatorPosition", new Param2<PlayerBase, vector>(GetGame().GetPlayer(), spawn_pos), true );

			m_vPreviousPosition = spawn_pos;
		}
	}
};