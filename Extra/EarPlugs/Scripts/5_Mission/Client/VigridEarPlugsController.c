#ifndef SERVER
/**
 *  EarPlugs - the whole feature.
 *
 *  Two independent halves, both client-local:
 *
 *    VOLUME   scale the effects (and radio) bus to a FRACTION OF THE PLAYER'S OWN SETTING;
 *    MUFFLE   put a low-pass EQ on the master bus via Man.SetMasterAttenuation.
 *
 *  Either can be removed without touching the other - which matters, because the muffle half is the
 *  one with an open question against it (see the VOIP note in VigridEarPlugsConstants.c).
 *
 *  ══ THE BASELINE IS MEASURED, NOT READ ═════════════════════════════════════════════════════════
 *
 *  The obvious source for "the player's own volume" is g_Game.m_volume_sound. It is wrong. Those
 *  five fields are written in exactly ONE place - DayZGame.DeferredInit, dayzgame.c:1130-1134 - and
 *  never again, so they are stale the moment the player touches the Options audio sliders. (Vanilla
 *  inherits its own bug here: MissionGameplay.OnPlayerRespawned restores from them.)
 *
 *  So while the level is Off, WHATEVER THE ENGINE REPORTS IS THE BASELINE, refreshed on the tick.
 *  An Options change made with the plugs out is picked up within 250 ms with no hook and no event.
 *
 *  ══ THE TOGGLE WRITES, THE RECONCILER ONLY EVER LOWERS ═════════════════════════════════════════
 *
 *  That asymmetry is the entire answer to a problem the reference implementation does not attempt.
 *  Vanilla moves these buses out from under any mod that touches them:
 *
 *    zeroes all five on death            dayzplayerimplement.c:861
 *    zeroes sound on uncon start         playerbase.c:3534
 *    restores from m_volume_* on:        playerbase.c:3581 (uncon stop)
 *                                        missiongameplay.c:1626 (unpause / respawn)
 *
 *  and in this mod set the host's spectate entry restores all five as well. Left alone, the level
 *  and the badge would both go on claiming "Heavy" while the engine sat at full volume, and the
 *  player would have to press the key three more times to resync.
 *
 *  Applying the rule "the reconciler never raises" makes every one of those cases fall out for free:
 *
 *    vanilla restored to baseline  ->  current > desired  ->  re-lower                     ✔
 *    vanilla zeroed for death/uncon->  current < desired  ->  leave it alone, it is theirs ✔
 *    level is Off                  ->  current == desired ->  no write at all              ✔
 *
 *  and the ONE path allowed to raise is the player pressing the key, which is exactly what should
 *  restore their audio when they take the plugs out.
 *
 *  ══ THE ATTENUATION SLOT IS SHARED, SO WE YIELD ════════════════════════════════════════════════
 *
 *  SetMasterAttenuation is a single global slot with no stacking, shared with vanilla's unconscious,
 *  burlap sack, flashbang and complete-deafness effects. Two rules keep everyone honest: only write
 *  when the slot is empty or already ours, and only clear when it is ours. Clearing blindly would
 *  un-muffle a player who is unconscious or wearing a sack over their head.
 */
class VigridEarPlugsController
{
    private int m_Level;

    //--- The player's own volumes, measured rather than read. See the header.
    private float m_BaselineSound;
    private float m_BaselineRadio;

    private bool m_Initialised;
    private int m_LastTickMs;

    //--- Reconciler writes are suppressed until this instant so they do not fight the toggle's own
    //--- fade. Without it, a fade in progress reads as "above target" on every tick of its duration.
    private int m_SettleUntilMs;

    //--- Last reason logged per decision point. A no-op decision is only logged when its reason
    //--- CHANGES, which is what keeps "log every rejection" from meaning twelve lines a second.
    private string m_ReasonSound;
    private string m_ReasonRadio;
    private string m_ReasonAtten;

    void VigridEarPlugsController()
    {
        m_Level = VigridEarPlugsPrefs.GetLevel();
        VigridEarPlugsLog.Debug("Controller created, stored level " + VigridEarPlugsLevels.DebugName(m_Level));
    }

    int GetLevel()
    {
        return m_Level;
    }

    /**
     *  Advance to the next level, persist it, and write it out.
     *
     *  This is the only path permitted to RAISE a bus volume - which is precisely what taking the
     *  plugs out has to do.
     */
    void Cycle()
    {
        AbstractSoundScene scene = GetGame().GetSoundScene();
        if (!scene)
        {
            VigridEarPlugsLog.Warn("Toggle ignored: no sound scene");
            return;
        }

        //--- A toggle before the first Update would apply a level against a baseline nobody has
        //--- measured yet. Measure it now instead of guessing.
        if (!m_Initialised)
            Initialise(scene);

        int previous = m_Level;
        m_Level = VigridEarPlugsLevels.Next(m_Level);
        VigridEarPlugsPrefs.SetLevel(m_Level);

        ApplyLevel(scene, VIGRID_EARPLUGS_FADE_S);
        m_SettleUntilMs = GetGame().GetTime() + VIGRID_EARPLUGS_FADE_MS;

        string line = "Level " + VigridEarPlugsLevels.DebugName(previous);
        line = line + " -> " + VigridEarPlugsLevels.DebugName(m_Level);
        line = line + " (x" + VigridEarPlugsLevels.Factor(m_Level).ToString() + ")";
        VigridEarPlugsLog.Info(line);
    }

    void Update(float timeslice)
    {
        AbstractSoundScene scene = GetGame().GetSoundScene();
        if (!scene)
            return;

        if (!m_Initialised)
        {
            Initialise(scene);
            return;
        }

        if ((GetGame().GetTime() - m_LastTickMs) < VIGRID_EARPLUGS_TICK_MS)
            return;

        m_LastTickMs = GetGame().GetTime();
        Tick(scene);
    }

    /**
     *  Give back what we took, on the way out of the mission.
     *
     *  A master attenuation left set would follow the player into the next session - the main menu,
     *  then whatever server they join next - with no UI anywhere to remove it.
     */
    void Shutdown()
    {
        ClearAttenuationIfOurs();

        AbstractSoundScene scene = GetGame().GetSoundScene();
        if (!scene)
            return;
        if (!m_Initialised)
            return;

        scene.SetSoundVolume(m_BaselineSound, 0);
        if (VIGRID_EARPLUGS_SCALE_RADIO)
            scene.SetRadioVolume(m_BaselineRadio, 0);

        VigridEarPlugsLog.Debug("Shutdown: buses restored to baseline, attenuation released");
    }

    /**
     *  Measure the player's own volumes and put the stored level back on.
     *
     *  Deferred to the first tick rather than done in the constructor: that runs inside
     *  MissionGameplay.OnInit, and the only timing proven to work in this mod is after
     *  super.OnInit() has returned. By the time a local player exists, DeferredInit is long done and
     *  whatever the scene reports is the player's Options setting.
     */
    private void Initialise(AbstractSoundScene scene)
    {
        if (!GetGame().GetPlayer())
            return;

        m_BaselineSound = scene.GetSoundVolume();
        m_BaselineRadio = scene.GetRadioVolume();
        m_Initialised = true;

        //--- Zero here is legal - a player really can run the game silent - but it makes the whole
        //--- feature a no-op, and that is worth a line so it is not mistaken for a broken addon.
        if (m_BaselineSound <= 0)
            VigridEarPlugsLog.Warn("Effects volume is already 0 at startup - ear plugs will have nothing to do");

        string line = "Baseline measured: sound=" + m_BaselineSound.ToString();
        line = line + " radio=" + m_BaselineRadio.ToString();
        line = line + ", restoring level " + VigridEarPlugsLevels.DebugName(m_Level);
        VigridEarPlugsLog.Info(line);

        //--- Applied with no fade: this is the session starting, not a change the player made.
        ApplyLevel(scene, 0);
    }

    private void Tick(AbstractSoundScene scene)
    {
        if (GetGame().GetTime() < m_SettleUntilMs)
        {
            Note("sound", "settling");
            return;
        }

        RefreshBaseline(scene);

        float desired_sound = m_BaselineSound * VigridEarPlugsLevels.Factor(m_Level);
        if (NeedsLowering(scene.GetSoundVolume(), desired_sound, "sound"))
            scene.SetSoundVolume(desired_sound, 0);

        if (VIGRID_EARPLUGS_SCALE_RADIO)
        {
            float desired_radio = m_BaselineRadio * VigridEarPlugsLevels.Factor(m_Level);
            if (NeedsLowering(scene.GetRadioVolume(), desired_radio, "radio"))
                scene.SetRadioVolume(desired_radio, 0);
        }

        ApplyAttenuation();
    }

    /**
     *  While the plugs are out, the engine's value IS the player's setting.
     *
     *  Guarded on being alive and above zero, because both of the ways vanilla zeroes these buses -
     *  death and going unconscious - would otherwise latch a baseline of 0 and leave the player
     *  permanently silent with nothing on screen to explain it.
     */
    private void RefreshBaseline(AbstractSoundScene scene)
    {
        if (m_Level != VIGRID_EARPLUGS_LEVEL_OFF)
            return;

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return;
        if (!player.IsAlive())
            return;

        float sound = scene.GetSoundVolume();
        if (sound > 0 && Math.AbsFloat(sound - m_BaselineSound) > VIGRID_EARPLUGS_EPSILON)
        {
            VigridEarPlugsLog.Debug("Baseline sound follows the player's Options change: " + m_BaselineSound.ToString() + " -> " + sound.ToString());
            m_BaselineSound = sound;
        }

        float radio = scene.GetRadioVolume();
        if (radio > 0 && Math.AbsFloat(radio - m_BaselineRadio) > VIGRID_EARPLUGS_EPSILON)
            m_BaselineRadio = radio;
    }

    /**
     *  The never-raise rule, and the reason each answer was given.
     *
     *  Correction writes pass time = 0 rather than a fade, deliberately: a fade would keep
     *  `current > desired` true for its whole duration and re-fire the write on every tick of it.
     */
    private bool NeedsLowering(float current, float desired, string bus)
    {
        if (current > desired + VIGRID_EARPLUGS_EPSILON)
        {
            string line = "corrected " + current.ToString();
            line = line + " -> " + desired.ToString();
            Note(bus, line);
            return true;
        }

        if (current < desired - VIGRID_EARPLUGS_EPSILON)
        {
            //--- Something else - vanilla's death or unconsciousness handling - has taken it below
            //--- where we want it. That is theirs to undo, not ours to fight.
            Note(bus, "below-target-leaving-alone");
            return false;
        }

        Note(bus, "already-at-target");
        return false;
    }

    private void ApplyLevel(AbstractSoundScene scene, float fade)
    {
        float factor = VigridEarPlugsLevels.Factor(m_Level);

        scene.SetSoundVolume(m_BaselineSound * factor, fade);
        if (VIGRID_EARPLUGS_SCALE_RADIO)
            scene.SetRadioVolume(m_BaselineRadio * factor, fade);

        ApplyAttenuation();
    }

    /**
     *  Take, hold or release the shared attenuation slot.
     *
     *  Called from both the toggle and the tick, so a slot that a vanilla effect borrowed and has
     *  since given back is picked up again within 250 ms without anyone having to notice.
     */
    private void ApplyAttenuation()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return;

        string want = VigridEarPlugsLevels.Attenuation(m_Level);
        string have = player.GetMasterAttenuation();

        if (want == "")
        {
            if (have == "")
            {
                Report("already-clear");
                return;
            }

            if (!IsOurs(have))
            {
                //--- Not ours to clear. Clearing here is how you accidentally un-muffle somebody
                //--- who is unconscious or has a burlap sack on their head.
                Report("foreign-left-alone:" + have);
                return;
            }

            player.SetMasterAttenuation("");
            Report("cleared");
            return;
        }

        if (have == want)
        {
            Report("already-applied:" + want);
            return;
        }

        if (have != "" && !IsOurs(have))
        {
            Report("yielded-to:" + have);
            return;
        }

        player.SetMasterAttenuation(want);
        Report("applied:" + want);
    }

    private void ClearAttenuationIfOurs()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return;

        if (!IsOurs(player.GetMasterAttenuation()))
            return;

        player.SetMasterAttenuation("");
    }

    //! Derived from the level table rather than a second list, so a new level cannot be forgotten here.
    private bool IsOurs(string attenuation)
    {
        if (attenuation == "")
            return false;

        for (int i = 0; i < VIGRID_EARPLUGS_LEVEL_COUNT; i++)
        {
            string mine = VigridEarPlugsLevels.Attenuation(i);
            if (mine != "" && mine == attenuation)
                return true;
        }

        return false;
    }

    //--- Reason bookkeeping. Split out so the three decision points read as decisions rather than as
    //--- logging. A no-op is logged only when its reason changes: a guard that never logs cannot
    //--- distinguish "this never ran" from "this ran and threw everything away", but one that logs
    //--- unconditionally at 4 Hz buries the change that mattered.

    private void Report(string reason)
    {
        Note("attenuation", reason);
    }

    private void Note(string subject, string reason)
    {
        if (subject == "radio")
        {
            if (m_ReasonRadio == reason)
                return;

            m_ReasonRadio = reason;
        }
        else if (subject == "attenuation")
        {
            if (m_ReasonAtten == reason)
                return;

            m_ReasonAtten = reason;
        }
        else
        {
            if (m_ReasonSound == reason)
                return;

            m_ReasonSound = reason;
        }

        VigridEarPlugsLog.Trace(subject + ": " + reason);
    }
}
#endif
