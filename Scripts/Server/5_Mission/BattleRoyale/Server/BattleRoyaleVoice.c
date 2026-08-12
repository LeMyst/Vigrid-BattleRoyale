#ifdef SERVER
/**
 *  Server-authoritative voice policy, and the source of the "who is speaking" panel.
 *
 *  While players are frozen - countdown, spawn selection, prepare - they are all standing in the
 *  same lobby blob, cannot walk away and, on PC, cannot mute anyone: DayZ's own mute UI is
 *  console-only. A player hears every other player and has no recourse. This class replaces that
 *  with party-only voice for the duration of the freeze.
 *
 *  The engine lever is CGame.MutePlayer(muteUID, playerUID, mute) - "mutes voice of source player
 *  to target player" - a directed per-pair matrix keyed on PlayerIdentity.GetPlainId(), the same
 *  SteamID64 VigridPartyAPI uses. It is *subtractive*: it can only remove hearing from within the
 *  engine's proximity set, never add it. That is exactly what is needed here, because during these
 *  states everyone - teammates included - is already well inside that set.
 *
 *  This class is the single owner of voice policy. PlayerBase.DisableInput() used to gag players
 *  itself, but on the client those calls are no-ops (the VON router is server-side) and on the
 *  server they gagged everyone globally, which would defeat party voice during prepare.
 *
 *  Lifecycle: applied from 2_BattleRoyaleCountReached.Activate() - the server is already locked
 *  there, so the roster is fixed for the whole states 2->4 window and the matrix never needs
 *  rebuilding - and cleared from 5_BattleRoyaleStartMatch.Activate(), before HandleUnlock() runs.
 */
class BattleRoyaleVoice
{
    /**
     *  Cached settings handle.
     *
     *  BattleRoyaleConfig.GetVoiceData() emits a Trace on every single call, and the code below runs
     *  at the server tick rate - once per tick in UpdateSpeakers, and once per listener/speaker pair
     *  in CanHear. Fetching it each time put roughly ten "Accessing Voice Data Config..." lines per
     *  second into the log, and on a DIAG server BattleRoyaleUtils mirrors every level into in-game
     *  chat, so it spammed that too.
     *
     *  The config is loaded once at boot and its data objects keep their identity, which is why the
     *  states already cache this in their constructors. Same idea, fetched lazily because this class
     *  has no constructor to hang it off.
     */
    private static BattleRoyaleVoiceData s_VoiceData;

    private static BattleRoyaleVoiceData VoiceData()
    {
        if (!s_VoiceData)
            s_VoiceData = BattleRoyaleConfig.GetConfig().GetVoiceData();

        return s_VoiceData;
    }

    //--- Exactly the uids the matrix was built from, so ClearAll() can undo precisely what it did.
    private static ref array<string> s_MutedRoster;
    //--- uid -> party group index. Kept alongside the matrix because the speaking panel needs the
    //--- same answer the matrix encodes: while party-only voice is on, you hear your party and
    //--- nobody else, so audibility is exact rather than inferred from distance.
    private static ref map<string, int> s_GroupIndex;
    private static bool s_PartyOnlyActive;

    static bool IsActive()
    {
        return s_PartyOnlyActive;
    }

    /**
     *  Mute every pair of players that are not in the same party.
     *
     *  Ordered pairs are walked, so both directions of every relationship are set explicitly rather
     *  than assumed symmetric.
     */
    static void ApplyPartyOnly(array<PlayerBase> players)
    {
        if (s_PartyOnlyActive)
        {
            BattleRoyaleUtils.Warn("BattleRoyaleVoice: party-only voice is already active, ignoring re-apply");
            return;
        }

        if (!players)
        {
            BattleRoyaleUtils.Warn("BattleRoyaleVoice: no player list supplied, leaving voice open");
            return;
        }

        if (!VoiceData().party_only_voice)
        {
            BattleRoyaleUtils.Info("BattleRoyaleVoice: party_only_voice is disabled, leaving voice open");
            return;
        }

        array<string> uids = new array<string>();
        map<string, int> group_index = new map<string, int>();

#ifdef VIGRID_PARTY
        //--- GetGroups() is a full partition: a player with no party comes back as a group of one.
        //--- Solo players therefore need no special case - every pair that touches them is muted,
        //--- which is the intended "a solo hears nobody while frozen" behaviour.
        array<ref array<PlayerBase>> groups = VigridPartyAPI.GetGroups(players);
        for (int g = 0; g < groups.Count(); g++)
        {
            array<PlayerBase> group = groups.Get(g);
            for (int m = 0; m < group.Count(); m++)
            {
                string member_uid = UidOf(group.Get(m));
                if (member_uid == "")
                    continue;

                group_index.Set(member_uid, g);
                uids.Insert(member_uid);
            }
        }
#else
        //--- Without the party addon everyone is their own group, so nobody hears anybody while
        //--- frozen. That is the safe direction to fail in: silence, not a free-for-all.
        for (int i = 0; i < players.Count(); i++)
        {
            string solo_uid = UidOf(players.Get(i));
            if (solo_uid == "")
                continue;

            group_index.Set(solo_uid, i);
            uids.Insert(solo_uid);
        }
#endif

        if (uids.Count() < 2)
        {
            BattleRoyaleUtils.Info("BattleRoyaleVoice: fewer than two identified players, nothing to mute");
            return;
        }

        int muted_pairs = 0;

        for (int a = 0; a < uids.Count(); a++)
        {
            for (int b = 0; b < uids.Count(); b++)
            {
                if (a == b)
                    continue;

                bool same_party = group_index.Get(uids.Get(a)) == group_index.Get(uids.Get(b));

                //--- uids[a] is the speaker being silenced, uids[b] the listener who stops hearing.
                GetGame().MutePlayer(uids.Get(a), uids.Get(b), !same_party);

                if (!same_party)
                    muted_pairs++;
            }
        }

        s_MutedRoster = uids;
        s_GroupIndex = group_index;
        s_PartyOnlyActive = true;
        ResetSpeakerPushes();

        BattleRoyaleUtils.Info(string.Format("BattleRoyaleVoice: party-only voice applied over %1 players, %2 muted pairs", uids.Count(), muted_pairs));
    }

    /**
     *  Restore open proximity voice.
     *
     *  Every pair is unmuted explicitly rather than relying on MuteAllPlayers(uid, false) to reset
     *  the matrix - the per-pair mute and the deafen-all flag are separate engine state, and this
     *  code should not depend on which of them wins.
     */
    static void ClearAll()
    {
        if (!s_PartyOnlyActive)
            return;

        if (!s_MutedRoster)
        {
            s_PartyOnlyActive = false;
            return;
        }

        for (int a = 0; a < s_MutedRoster.Count(); a++)
        {
            for (int b = 0; b < s_MutedRoster.Count(); b++)
            {
                if (a == b)
                    continue;

                GetGame().MutePlayer(s_MutedRoster.Get(a), s_MutedRoster.Get(b), false);
            }
        }

        BattleRoyaleUtils.Info(string.Format("BattleRoyaleVoice: party-only voice cleared for %1 players", s_MutedRoster.Count()));

        s_MutedRoster = NULL;
        s_GroupIndex = NULL;
        s_PartyOnlyActive = false;
        ResetSpeakerPushes();
    }

    //--------------------------------------------------------------------------------------------
    //--- "Who is speaking" panel
    //--------------------------------------------------------------------------------------------
    //
    //  This has to run on the server. DayZPlayer.IsPlayerSpeaking() is per-entity here - the
    //  speaker reads non-zero and everyone else reads exactly 0 - but on a client it returns the
    //  LOCAL microphone level whichever entity it is called on, so a client cannot tell who is
    //  talking. Measured with two clients on 2026-08-04; see BattleRoyaleConstants.c.
    //
    //  Each listener is pushed only the speakers it is actually allowed to hear, and only when
    //  that set changes. Nothing here logs per sample: on a DIAG server every log level mirrors
    //  into in-game chat (BattleRoyaleUtils.c:45-48), so a hot-path log would flood it.

    private static ref map<string, int> s_LastSpokeMs;    //!< speaker uid -> last time above threshold
    private static ref map<string, string> s_LastPushed;  //!< listener uid -> last payload signature
    private static int s_NextSpeakerPollMs;

    static void UpdateSpeakers()
    {
        //--- Throttle FIRST. This is called at the server tick rate, so anything above this line
        //--- runs twice as often as the poll it guards.
        int now_ms = GetGame().GetTime();
        if (now_ms < s_NextSpeakerPollMs)
            return;
        s_NextSpeakerPollMs = now_ms + BR_SPEAKING_POLL_MS;

        if (!VoiceData().show_speaking_players)
            return;

        if (!s_LastSpokeMs)
            s_LastSpokeMs = new map<string, int>();
        if (!s_LastPushed)
            s_LastPushed = new map<string, string>();

        array<Man> everyone = new array<Man>();
        GetGame().GetPlayers(everyone);
        if (everyone.Count() == 0)
            return;

        //--- Pass 1: who is making noise right now. Usually nobody, so the per-listener pass below
        //--- stays cheap - it is O(listeners x speakers), not O(players^2).
        for (int i = 0; i < everyone.Count(); i++)
        {
            PlayerBase talker = PlayerBase.Cast(everyone.Get(i));
            if (!talker)
                continue;
            if (!talker.GetIdentity())
                continue;

            if (talker.IsPlayerSpeaking() > BR_SPEAKING_AMPLITUDE_THRESHOLD)
                s_LastSpokeMs.Set(talker.GetIdentity().GetPlainId(), now_ms);
        }

        //--- Pass 2: collect those still inside the linger window.
        array<PlayerBase> speakers = new array<PlayerBase>();
        for (int j = 0; j < everyone.Count(); j++)
        {
            PlayerBase candidate = PlayerBase.Cast(everyone.Get(j));
            if (!candidate)
                continue;
            if (!candidate.GetIdentity())
                continue;

            string candidate_uid = candidate.GetIdentity().GetPlainId();
            if (!s_LastSpokeMs.Contains(candidate_uid))
                continue;

            if (now_ms - s_LastSpokeMs.Get(candidate_uid) <= BR_SPEAKING_LINGER_MS)
                speakers.Insert(candidate);
            else
                s_LastSpokeMs.Remove(candidate_uid);
        }

        //--- Pass 3: per listener, the subset it may hear.
        for (int k = 0; k < everyone.Count(); k++)
        {
            PlayerBase listener = PlayerBase.Cast(everyone.Get(k));
            if (!listener)
                continue;
            if (!listener.GetIdentity())
                continue;

            string listener_uid = listener.GetIdentity().GetPlainId();

            array<string> names = new array<string>();
            int self_index = -1;
            string signature = "";

            for (int s = 0; s < speakers.Count(); s++)
            {
                PlayerBase speaker = speakers.Get(s);
                if (!CanHear(listener, speaker))
                    continue;

                if (speaker == listener)
                    self_index = names.Count();

                names.Insert(speaker.GetIdentity().GetName());
                signature = signature + speaker.GetIdentity().GetPlainId() + ",";
            }

            signature = signature + "|" + self_index.ToString();

            //--- Only on change. A listener hearing nobody gets exactly one empty push, then
            //--- silence on the wire until somebody speaks again.
            if (s_LastPushed.Contains(listener_uid))
            {
                if (s_LastPushed.Get(listener_uid) == signature)
                    continue;
            }

            s_LastPushed.Set(listener_uid, signature);

            GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetSpeakingPlayers", new Param2<array<string>, int>( names, self_index ), true, listener.GetIdentity() );

            //--- Edge-triggered only, so this is a couple of lines per utterance rather than a
            //--- stream. Debug level, so it is silent on a production server (log level Error).
            BattleRoyaleUtils.Debug(string.Format("BattleRoyaleVoice: pushed %1 speaker(s) to %2 (self=%3)", names.Count(), listener.GetIdentity().GetName(), self_index));
        }
    }

    /**
     *  Can `listener` hear `speaker` right now?
     *
     *  While party-only voice is on this is exact: it reads the same grouping the mute matrix was
     *  built from. Otherwise it approximates the engine's proximity rule, because the audible
     *  radius of whisper/talk/shout exists nowhere in script - hence the configurable radii.
     */
    private static bool CanHear(PlayerBase listener, PlayerBase speaker)
    {
        if (listener == speaker)
            return true;

        string listener_uid = listener.GetIdentity().GetPlainId();
        string speaker_uid = speaker.GetIdentity().GetPlainId();

        if (s_PartyOnlyActive)
        {
            if (!s_GroupIndex)
                return false;
            if (!s_GroupIndex.Contains(listener_uid))
                return false;
            if (!s_GroupIndex.Contains(speaker_uid))
                return false;

            return s_GroupIndex.Get(listener_uid) == s_GroupIndex.Get(speaker_uid);
        }

        float radius = VoiceData().voice_radius_talk;
        int level = GetGame().GetVoiceLevel(speaker);
        if (level == VoiceLevelWhisper)
            radius = VoiceData().voice_radius_whisper;
        else if (level == VoiceLevelShout)
            radius = VoiceData().voice_radius_shout;

        vector listener_pos = listener.GetPosition();
        vector speaker_pos = speaker.GetPosition();
        listener_pos[1] = 0;
        speaker_pos[1] = 0;

        return vector.Distance(listener_pos, speaker_pos) <= radius;
    }

    /**
     *  Forget every listener's last payload, so the next tick re-pushes from scratch.
     *
     *  Called when the policy flips, because the same speaker set means something different before
     *  and after: a listener whose list is unchanged by value would otherwise never be told that
     *  the reason they can hear somebody has changed.
     */
    static void ResetSpeakerPushes()
    {
        if (s_LastPushed)
            s_LastPushed.Clear();
    }

    /**
     *  SteamID64 of a player, or "" when it cannot be read.
     *
     *  PlayerIdentity can be null while the PlayerBase is still alive, so this never dereferences
     *  blind. Warn rather than Error: Error raises a VM exception and would take the server down
     *  over one unidentifiable player.
     */
    private static string UidOf(PlayerBase player)
    {
        if (!player)
            return "";

        if (!player.GetIdentity())
        {
            BattleRoyaleUtils.Warn("BattleRoyaleVoice: player without identity skipped, they keep open voice");
            return "";
        }

        return player.GetIdentity().GetPlainId();
    }
}
#endif
