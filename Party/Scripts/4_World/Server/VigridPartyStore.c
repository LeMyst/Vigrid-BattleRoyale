#ifdef SERVER
/**
 *  Vigrid Party - persistence for parties.json.
 *
 *  Parties outlive the process on purpose: a Battle Royale server restarts between every match, so
 *  without this players would rebuild their team every single game. That is the whole reason this
 *  file exists.
 *
 *  Everything read off disk is treated as untrusted. A hand-edited or half-written file must never
 *  stop the server booting, so Load() repairs what it can and drops what it cannot.
 */
class VigridPartyStore
{
    /**
     *  Read parties.json and return the surviving parties.
     *
     *  Pruning, in order:
     *    - parties not seen for longer than party_ttl_hours
     *    - parties with fewer than 2 members (a party of one is just a player)
     *    - members beyond max_party_size, dropped from the tail so the oldest joiners are kept
     *    - a leader_uid that is not actually in member_uids, repaired to the first member
     *    - duplicate uids, and a uid claimed by more than one party (first party wins)
     *
     *  `name_cache` (uid -> last known display name) is filled in place rather than returned. A map
     *  is a reference type, so the caller sees the writes and neither an `out` parameter - `out` is
     *  a direction keyword EnfusionScript will not accept as an identifier - nor a second pass over
     *  the file is needed. Deliberately not named `map`, which collides with the container type.
     */
    static array<ref VigridParty> Load(VigridPartyData settings, map<string, string> name_cache)
    {
        array<ref VigridParty> result = new array<ref VigridParty>();

        if (!FileExist(VIGRID_PARTY_STORE_FILE))
        {
            VigridPartyLog.Info("No parties.json yet - starting with an empty registry");
            return result;
        }

        VigridPartyStoreFile store = new VigridPartyStoreFile();
        string error_message;

        if (!JsonFileLoader<VigridPartyStoreFile>.LoadFile(VIGRID_PARTY_STORE_FILE, store, error_message))
        {
            //--- Truncated or malformed. Log it and boot with an empty registry; the next mutation
            //--- overwrites the file. Losing the parties beats refusing to start.
            VigridPartyLog.Error("parties.json could not be read, ignoring it: " + error_message);
            return result;
        }

        if (!store.parties)
        {
            VigridPartyLog.Warn("parties.json has no parties array, ignoring it");
            return result;
        }

        int now_hours = VigridPartyTime.NowHours();
        set<string> claimed = new set<string>();

        int party_count = store.parties.Count();
        for (int i = 0; i < party_count; i++)
        {
            VigridPartyStoreEntry entry = store.parties.Get(i);
            if (!entry)
                continue;
            if (!entry.member_uids)
                continue;
            if (entry.id == "")
                continue;

            if (settings.party_ttl_hours > 0 && (now_hours - entry.last_seen_hours) > settings.party_ttl_hours)
            {
                VigridPartyLog.Debug("Dropping stale party " + entry.id);
                continue;
            }

            VigridParty party = new VigridParty();
            party.id = entry.id;
            party.created_at = entry.created_at;
            party.last_seen_hours = entry.last_seen_hours;

            int member_count = entry.member_uids.Count();
            for (int j = 0; j < member_count; j++)
            {
                string uid = entry.member_uids.Get(j);
                if (uid == "")
                    continue;
                if (claimed.Find(uid) != -1)
                {
                    VigridPartyLog.Warn("uid " + uid + " appears in more than one party, keeping the first");
                    continue;
                }
                if (party.Count() >= settings.max_party_size)
                {
                    VigridPartyLog.Warn("Party " + entry.id + " exceeds max_party_size, dropping trailing member " + uid);
                    continue;
                }

                //--- Add() also seeds leader_uid from the first member, which is the repair path
                //--- for an entry whose stored leader turns out to be invalid.
                if (party.Add(uid))
                    claimed.Insert(uid);
            }

            if (party.Count() < 2)
            {
                //--- Release the uids so a one-member leftover does not block a rebuild.
                int kept = party.Count();
                for (int k = 0; k < kept; k++)
                {
                    int claimed_index = claimed.Find(party.member_uids.Get(k));
                    if (claimed_index != -1)
                        claimed.Remove(claimed_index);
                }
                continue;
            }

            //--- Honour the stored leader when it survived the filtering above.
            if (entry.leader_uid != "" && party.Contains(entry.leader_uid))
                party.SetLeader(entry.leader_uid);

            result.Insert(party);
        }

        //--- Names are read whatever happened to the parties above: a v1 file has no names key at
        //--- all and simply yields nothing here, which is the migration.
        if (store.names)
        {
            int name_count = store.names.Count();
            for (int n = 0; n < name_count; n++)
            {
                VigridPartyNameEntry name_entry = store.names.Get(n);
                if (!name_entry)
                    continue;
                if (name_entry.uid == "")
                    continue;
                if (name_entry.name == "")
                    continue;
                if (settings.party_ttl_hours > 0 && (now_hours - name_entry.seen_hours) > settings.party_ttl_hours)
                    continue;

                name_cache.Set(name_entry.uid, name_entry.name);
            }
        }

        VigridPartyLog.Info("Loaded " + result.Count() + " parties and " + name_cache.Count() + " names from disk");
        return result;
    }

    /**
     *  `name_cache` is written pruned: only uids that are actually in a party being persisted make
     *  it to disk, so the file stays bounded by real membership no matter how many players have
     *  connected this session.
     *
     *  Auto-placed members are subtracted first. A party is meant to outlive the process so a real
     *  team does not have to be rebuilt every match; a teammate the host's auto-grouper assigned at
     *  random is precisely what must not. So a fully auto-formed party writes nothing at all (it
     *  fails the two-member rule once filtered) and a real party that was topped up is written back
     *  as the party it was before the match.
     */
    static void Save(array<ref VigridParty> parties, map<string, string> name_cache)
    {
        VigridPartyStoreFile store = new VigridPartyStoreFile();
        store.saved_at = VigridPartyTime.NowSeconds();
        int now_hours = VigridPartyTime.NowHours();

        array<string> persistent = new array<string>();

        int count = parties.Count();
        for (int i = 0; i < count; i++)
        {
            VigridParty party = parties.Get(i);
            if (!party)
                continue;

            party.BuildPersistentMembers(persistent);
            if (persistent.Count() < 2)
                continue; //!< never persist a party that has decayed to a single member

            VigridPartyStoreEntry entry = new VigridPartyStoreEntry();
            entry.id = party.id;
            entry.created_at = party.created_at;
            entry.last_seen_hours = party.last_seen_hours;
            entry.member_uids.Copy(persistent);

            //--- The live leader can be an auto member: leader_transfer_on_disconnect promotes the
            //--- first ONLINE member, which does not care how they joined. Writing that uid would
            //--- leave a leader who is not in member_uids - Load() repairs it, but a file that needs
            //--- repairing every save is a file nobody can read. Fall back to the senior survivor,
            //--- which is what Load()'s repair would have picked anyway.
            entry.leader_uid = party.leader_uid;
            if (party.IsAuto(entry.leader_uid))
                entry.leader_uid = persistent.Get(0);

            store.parties.Insert(entry);
        }

        //--- Walk what actually got persisted rather than the cache: a name is only worth keeping
        //--- for somebody who will still be in a party when the file is read back.
        int stored_count = store.parties.Count();
        for (int s = 0; s < stored_count; s++)
        {
            VigridPartyStoreEntry stored = store.parties.Get(s);

            int stored_members = stored.member_uids.Count();
            for (int m = 0; m < stored_members; m++)
            {
                string member_uid = stored.member_uids.Get(m);
                if (!name_cache.Contains(member_uid))
                    continue;

                VigridPartyNameEntry name_entry = new VigridPartyNameEntry();
                name_entry.uid = member_uid;
                name_entry.name = name_cache.Get(member_uid);
                name_entry.seen_hours = now_hours;

                store.names.Insert(name_entry);
            }
        }

        string error_message;
        if (!JsonFileLoader<VigridPartyStoreFile>.SaveFile(VIGRID_PARTY_STORE_FILE, store, error_message))
        {
            VigridPartyLog.Error("Failed to write parties.json: " + error_message);
            return;
        }

        VigridPartyLog.Debug("Wrote " + store.parties.Count() + " parties and " + store.names.Count() + " names to disk");
    }
}
#endif
