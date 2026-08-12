#ifdef SERVER
/**
 *  One party. Members are identified by PlayerIdentity.GetPlainId() (SteamID64) throughout the
 *  addon - never GetPlayerId(), which is a session index the engine reuses after a disconnect and
 *  would silently reassign a party to a different person.
 *
 *  member_uids is ordered by join time and that order is the member's stable slot: it drives the
 *  spawn-marker colour and the roster indices the client renders against, so never reorder it for
 *  cosmetic reasons.
 */
class VigridParty
{
    string id;
    string leader_uid;
    ref array<string> member_uids;
    int created_at;      //!< hours since epoch, UTC
    int last_seen_hours; //!< refreshed whenever a member connects; drives TTL pruning

    /**
     *  Stamped from a process-wide monotonic counter every time membership or leadership changes,
     *  so a value is never reused - not even across two different parties.
     *
     *  This is what makes roster changes atomic without a handshake: VP_TeamState carries the
     *  version its parallel arrays were built against, and a client whose roster version differs
     *  discards the message instead of indexing into a roster that no longer matches.
     */
    int roster_version;

    void VigridParty()
    {
        member_uids = new array<string>();
    }

    bool Contains(string uid)
    {
        return member_uids.Find(uid) != -1;
    }

    bool IsLeader(string uid)
    {
        return leader_uid == uid;
    }

    int IndexOf(string uid)
    {
        return member_uids.Find(uid);
    }

    int Count()
    {
        return member_uids.Count();
    }

    bool Add(string uid)
    {
        if (uid == "")
            return false;
        if (Contains(uid))
            return false;

        member_uids.Insert(uid);

        //--- First member in becomes the leader, so a party is never leaderless.
        if (leader_uid == "")
            leader_uid = uid;

        return true;
    }

    /**
     *  Remove a member. When the leader leaves, the longest-tenured remaining member is promoted
     *  immediately rather than leaving the party headless - member_uids is join-ordered, so
     *  element 0 is exactly that. Returns false when the uid was not a member.
     */
    bool Remove(string uid)
    {
        int index = member_uids.Find(uid);
        if (index == -1)
            return false;

        member_uids.Remove(index);

        if (leader_uid != uid)
            return true;

        leader_uid = "";
        if (member_uids.Count() > 0)
            leader_uid = member_uids.Get(0);

        return true;
    }

    bool SetLeader(string uid)
    {
        if (!Contains(uid))
            return false;

        leader_uid = uid;
        return true;
    }

    string Repr()
    {
        //--- Not named `out`: that is a reserved parameter-direction keyword in EnfusionScript
        //--- and using it as an identifier fails to parse.
        string text = id + "[leader=" + leader_uid + "]{";
        int count = member_uids.Count();
        for (int i = 0; i < count; i++)
        {
            if (i > 0)
                text = text + ",";
            text = text + member_uids.Get(i);
        }
        return text + "}";
    }
}
#endif
