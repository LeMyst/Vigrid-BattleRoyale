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
     */
    static array<ref VigridParty> Load(VigridPartyData settings)
    {
        array<ref VigridParty> result = new array<ref VigridParty>();

        if (!FileExist(VIGRID_PARTY_STORE_FILE))
        {
            VigridPartyLog.Info("No parties.json yet - starting with an empty registry");
            return result;
        }

        ref VigridPartyStoreFile store = new VigridPartyStoreFile();
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
        ref set<string> claimed = new set<string>();

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

            ref VigridParty party = new VigridParty();
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

        VigridPartyLog.Info("Loaded " + result.Count() + " parties from disk");
        return result;
    }

    static void Save(array<ref VigridParty> parties)
    {
        ref VigridPartyStoreFile store = new VigridPartyStoreFile();
        store.version = 1;
        store.saved_at = VigridPartyTime.NowSeconds();

        int count = parties.Count();
        for (int i = 0; i < count; i++)
        {
            VigridParty party = parties.Get(i);
            if (!party)
                continue;
            if (party.Count() < 2)
                continue; //!< never persist a party that has decayed to a single member

            ref VigridPartyStoreEntry entry = new VigridPartyStoreEntry();
            entry.id = party.id;
            entry.leader_uid = party.leader_uid;
            entry.created_at = party.created_at;
            entry.last_seen_hours = party.last_seen_hours;
            entry.member_uids.Copy(party.member_uids);

            store.parties.Insert(entry);
        }

        string error_message;
        if (!JsonFileLoader<VigridPartyStoreFile>.SaveFile(VIGRID_PARTY_STORE_FILE, store, error_message))
        {
            VigridPartyLog.Error("Failed to write parties.json: " + error_message);
            return;
        }

        VigridPartyLog.Debug("Wrote " + store.parties.Count() + " parties to disk");
    }
}
#endif
