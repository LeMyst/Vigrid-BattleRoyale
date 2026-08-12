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

class VigridPartyStoreFile
{
    int version = 1;
    int saved_at;
    ref array<ref VigridPartyStoreEntry> parties;

    void VigridPartyStoreFile()
    {
        parties = new array<ref VigridPartyStoreEntry>();
    }
}
#endif
