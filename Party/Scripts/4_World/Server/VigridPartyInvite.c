#ifdef SERVER
/**
 *  A pending invitation. Never persisted - an invite that outlived a server restart would be
 *  meaningless, since neither party is still connected.
 *
 *  expires_at_ms is on the monotonic session clock (GetGame().GetTime()), not wall time: invites
 *  only ever live inside one session, so the simpler clock is also the correct one.
 */
class VigridPartyInvite
{
    string invite_id;
    string inviter_uid;
    string invitee_uid;
    string party_id;    //!< "" when the inviter had no party yet; accepting creates it
    int expires_at_ms;

    void VigridPartyInvite(string _invite_id, string _inviter_uid, string _invitee_uid, string _party_id, int _expires_at_ms)
    {
        invite_id = _invite_id;
        inviter_uid = _inviter_uid;
        invitee_uid = _invitee_uid;
        party_id = _party_id;
        expires_at_ms = _expires_at_ms;
    }

    bool IsExpired(int now_ms)
    {
        return now_ms >= expires_at_ms;
    }

    int SecondsLeft(int now_ms)
    {
        int left = (expires_at_ms - now_ms) / 1000;
        if (left < 0)
            return 0;
        return left;
    }
}
#endif
