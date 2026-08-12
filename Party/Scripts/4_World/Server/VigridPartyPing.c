#ifdef SERVER
/**
 *  One world marker placed by a party member.
 *
 *  Never persisted, for the same reason an invite is not: a marker that outlived a server restart
 *  would point at a position nobody remembers agreeing to.
 *
 *  expires_at_ms is on the monotonic session clock (VigridPartyTime.NowMs), matching
 *  VigridPartyInvite. 0 means "never" - a permanent ping is a legal configuration and is what Carim
 *  always did.
 */
class VigridPartyPing
{
    string owner_uid;
    vector position;
    int placed_at_ms;
    int expires_at_ms; //!< 0 = never

    void VigridPartyPing(string _owner_uid, vector _position, int _placed_at_ms, int _expires_at_ms)
    {
        owner_uid = _owner_uid;
        position = _position;
        placed_at_ms = _placed_at_ms;
        expires_at_ms = _expires_at_ms;
    }

    bool IsExpired(int now_ms)
    {
        if (expires_at_ms == 0)
            return false;

        return now_ms >= expires_at_ms;
    }

    /**
     *  Milliseconds left, for the wire. The client is sent a remaining time rather than an absolute
     *  one because the two processes' clocks are unrelated.
     *
     *  Never returns 0 for a ping that has an expiry: 0 is the permanent sentinel on the wire, and a
     *  ping caught between expiring and being swept must not be relayed as immortal.
     */
    int RemainingMs(int now_ms)
    {
        if (expires_at_ms == 0)
            return 0;

        int left = expires_at_ms - now_ms;
        if (left < 1)
            return 1;

        return left;
    }
}
#endif
