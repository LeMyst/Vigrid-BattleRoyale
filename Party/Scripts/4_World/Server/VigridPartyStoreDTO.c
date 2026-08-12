#ifdef SERVER
/**
 *  On-disk shape of $profile:Vigrid-Party\parties.json.
 *
 *  Kept separate from VigridParty so the runtime type is free to grow indices, caches or anything
 *  else without changing the file format - only what lives here is a compatibility promise.
 */
class VigridPartyStoreEntry
{
    string id;
    string leader_uid;
    ref array<string> member_uids;
    int created_at;
    int last_seen_hours;

    void VigridPartyStoreEntry()
    {
        member_uids = new array<string>();
    }
}

/**
 *  One remembered display name.
 *
 *  Kept in its own section rather than as a names[] array parallel to member_uids: that order is
 *  the member's stable slot and is never reshuffled (VigridParty.c:7-9), while Load() legitimately
 *  *filters* members as it rebuilds a party - duplicates, and anything past max_party_size - so a
 *  parallel array would drift out of step with no way to notice. Keyed by uid there is nothing to
 *  keep in lockstep, and a uid that is not currently in any party (an invitee, someone who left
 *  and will rejoin) can be remembered just as well.
 */
class VigridPartyNameEntry
{
    string uid;
    string name;
    /**
     *  Same clock as last_seen_hours. Restamped on every save for every name that survives the
     *  prune, so in normal operation a name lives exactly as long as the membership that keeps it -
     *  which is the intent. The TTL check on the way back in is therefore a backstop against a
     *  hand-edited or very old file, not the mechanism that expires names.
     */
    int seen_hours;
}

class VigridPartyStoreFile
{
    //--- 1 -> 2 added the names[] section. There is no migration to write: a v1 file simply has no
    //--- names key, JsonFileLoader leaves the ctor-allocated array empty, and every name is
    //--- relearned the first time its owner connects.
    int version = 2;
    int saved_at;
    ref array<ref VigridPartyStoreEntry> parties;
    ref array<ref VigridPartyNameEntry> names;

    void VigridPartyStoreFile()
    {
        parties = new array<ref VigridPartyStoreEntry>();
        names = new array<ref VigridPartyNameEntry>();
    }
}
#endif
