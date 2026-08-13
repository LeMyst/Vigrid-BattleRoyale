#ifdef SERVER
/**
 *  The single answer to "who is responsible for this death".
 *
 *  Four places used to derive this independently and disagreed with each other: the spectator killer
 *  chain, the Vigrid API webhook, the kill credit, and PlayerBase.EEHitBy's last_unconscious_source.
 *  Everything funnels through here now, so a device the resolver learns about is learnt by all four
 *  at once.
 *
 *  Two facts drive the whole design, and both are counter-intuitive enough to have caused bugs:
 *
 *    - EEKilled's `source` is the WEAPON for every gun and melee kill, never the shooter. The
 *      hierarchy-parent step is what turns it back into a player.
 *    - For an explosive or a trap the source is the DEVICE, which has no hierarchy parent at all
 *      once it is armed in the world. The responsible player is only knowable because the device
 *      recorded them at arm time - see ExplosivesBase.c and LandMineTrap.c. That recording is a
 *      plain string, which is what lets a kill outlive its owner's death and disconnect.
 */
/**
 *  Everything the death recap needs about one kill, resolved in one pass.
 *
 *  Plain data only - no object references - so it can be copied straight onto a death record, which
 *  is bound by BattleRoyaleSpectators' "holds no object reference at all" invariant.
 */
class BattleRoyaleKillDetails
{
    int cause;              //!< BattleRoyaleKillCause
    string killer_uid;      //!< SteamID64, "" for every non-player cause
    string killer_name;
    string weapon_type;     //!< classname. Localised CLIENT-side; "" when there is no weapon.
    int distance_m;         //!< -1 when there is no shooter to measure from
    int killer_health_pct;  //!< 0-100, -1 unknown

    void BattleRoyaleKillDetails()
    {
        cause = BattleRoyaleKillCause.UNKNOWN;
        killer_uid = "";
        killer_name = "";
        weapon_type = "";
        distance_m = -1;
        killer_health_pct = -1;
    }
}

class BattleRoyaleKillAttribution
{
    /**
     *  The player behind a damage source, or NULL when nothing player-shaped is.
     *
     *  Deliberately does NOT consult a device's recorded activator: it answers "is there a live
     *  PlayerBase to point at", which is a different question from "whose kill is this". Use
     *  ResolveKillerUid for the latter.
     */
    static PlayerBase ResolvePlayerSource(Object source)
    {
        EntityAI source_entity = NULL;
        PlayerBase player_source = NULL;

        if (!source)
            return NULL;

        //--- gun or melee: the weapon's hierarchy parent is the shooter. The cast is checked because
        //--- a source is not necessarily an EntityAI - a building or a vehicle part is not - and the
        //--- two call sites this replaced both dereferenced it unguarded.
        source_entity = EntityAI.Cast(source);
        if (source_entity)
            player_source = PlayerBase.Cast(source_entity.GetHierarchyParent());

        //--- fists, or a direct player source
        if (!player_source)
            player_source = PlayerBase.Cast(source);

        return player_source;
    }

    //! An armed device that kills on someone else's behalf: any explosive, any trap.
    static bool IsProxyDevice(Object source)
    {
        if (!source)
            return false;

        if (source.IsInherited(ExplosivesBase))
            return true;

        return source.IsInherited(TrapBase);
    }

    //! SteamID64 recorded on a proxy device when it was armed, or "" for anything else.
    static string ResolveDeviceActivatorUid(Object source)
    {
        ExplosivesBase explosive = NULL;
        TrapBase trap = NULL;

        if (Class.CastTo(explosive, source))
            return explosive.GetActivatorId();

        if (Class.CastTo(trap, source))
            return trap.GetActivatorId();

        return "";
    }

    //! Display name recorded alongside the uid. Kept because resolving a uid back to a name needs a
    //! player object, which a dead or departed activator no longer has.
    static string ResolveDeviceActivatorName(Object source)
    {
        ExplosivesBase explosive = NULL;
        TrapBase trap = NULL;

        if (Class.CastTo(explosive, source))
            return explosive.GetActivatorName();

        if (Class.CastTo(trap, source))
            return trap.GetActivatorName();

        return "";
    }

    /**
     *  Resolve the PLAYER responsible for a death to a uid, or "" when nothing player-shaped is.
     *
     *  A player is never their own killer: zone damage, starvation, a fall and drowning all arrive
     *  with the victim as their own source, and blowing yourself up with your own grenade reads the
     *  same way once the activator resolves back to the victim.
     */
    static string ResolveKillerUid(PlayerBase victim, Object source)
    {
        string victim_uid = "";
        string result = "";
        PlayerBase killer_player = NULL;

        if (!victim)
            return "";
        if (!victim.GetIdentity())
            return "";

        victim_uid = victim.GetIdentity().GetPlainId();

        if (!source)
            return "";
        if (source == victim)
            return "";

        if (IsProxyDevice(source))
        {
            result = ResolveDeviceActivatorUid(source);
            if (result == victim_uid)
                result = "";

            return result;
        }

        killer_player = ResolvePlayerSource(source);

        //--- anything else (infected, animal, vehicle, world object) is environmental to us
        if (!killer_player)
            return "";
        if (killer_player == victim)
            return "";
        if (!killer_player.GetIdentity())
            return "";

        result = killer_player.GetIdentity().GetPlainId();
        if (result == victim_uid)
            result = "";

        return result;
    }

    /**
     *  The whole recap for one kill: cause, killer, weapon, range, killer's remaining health.
     *
     *  Lives HERE rather than inline in BattleRoyaleSpectators.RecordDeath for three reasons. This
     *  class exists precisely because four consumers derived attribution independently and disagreed;
     *  weapon-and-range is the same shape of fact and there are already TWO derivations of it in the
     *  tree (Extra/KillFeed/KillFeedDeath.c and the webhook JSON block in
     *  0_BattleRoyaleState.OnPlayerKilled), so writing a third inline repeats the original mistake.
     *  It is also a stage problem: this is 4_World and BattleRoyaleSpectators is 5_Mission, so a
     *  resolver placed there could not be reached from PlayerBase or the match stats. And RecordDeath
     *  is governed by a subtle first-write-wins rule that stays readable only while it is short.
     *
     *  Note the webhook's own copy in 0_BattleRoyaleState is deliberately NOT refactored onto this:
     *  it is an external Vigrid API contract and a behaviour-preserving change there is not locally
     *  verifiable.
     */
    //--- `details` is filled in place rather than returned. No `out` on it: a class is already a
    //--- reference in EnfusionScript, and `out` there buys nothing.
    static void ResolveKillDetails(PlayerBase victim, Object source, BattleRoyaleKillDetails details)
    {
        EntityAI source_entity = NULL;
        PlayerBase killer_player = NULL;

        if (!details)
            return;
        if (!victim)
            return;

        //--- The uid comes from the single existing resolver, which already returns "" for the victim
        //--- as their own source, their own explosive, infected, animals, buildings and a killer with
        //--- no identity. Nothing downstream needs to re-filter self or zone damage.
        details.killer_uid = ResolveKillerUid(victim, source);

        source_entity = EntityAI.Cast(source);

        if (!source || source == victim)
        {
            //--- Nothing hit them: the play area, a fall, drowning, exposure. The zone is the one we
            //--- can actually name, and only because the damage site left a hint.
            details.cause = ConsumeZoneHint(victim);
        }
        else if (IsProxyDevice(source))
        {
            //--- Name first, uid second: a device outlives its owner, which is the entire reason the
            //--- name is recorded at arm time rather than resolved from the uid later.
            details.cause = BattleRoyaleKillCause.EXPLOSIVE;
            details.weapon_type = source.GetType();
            details.killer_name = ResolveDeviceActivatorName(source);
        }
        else if (source.IsWeapon() || source.IsMeleeWeapon())
        {
            killer_player = ResolvePlayerSource(source);

            if (killer_player)
            {
                details.weapon_type = source.GetType();

                if (source.IsMeleeWeapon())
                {
                    details.cause = BattleRoyaleKillCause.MELEE;
                }
                else
                {
                    details.cause = BattleRoyaleKillCause.FIREARM;
                    details.distance_m = Math.Round(vector.Distance(victim.GetPosition(), killer_player.GetPosition()));
                }
            }
            else
            {
                //--- A weapon with no hierarchy parent: dropped, or held by something that is not a
                //--- player. Nobody to credit.
                details.cause = BattleRoyaleKillCause.ENVIRONMENT;
            }
        }
        else if (source.IsInherited(PlayerBase))
        {
            details.cause = BattleRoyaleKillCause.BAREHANDS;
        }
        else if (source.IsInherited(ZombieBase))
        {
            details.cause = BattleRoyaleKillCause.INFECTED;
        }
        else if (source.IsInherited(AnimalBase))
        {
            details.cause = BattleRoyaleKillCause.ANIMAL;
        }
        else
        {
            details.cause = BattleRoyaleKillCause.ENVIRONMENT;
        }

        //--- Name and health both come from the uid, so they are filled once here rather than in each
        //--- branch above. FindPlayerByUid matches a CORPSE, which is what names a killer who died
        //--- moments after their own kill landed.
        if (details.killer_uid != "")
        {
            if (!killer_player)
                killer_player = FindPlayerByUid(details.killer_uid);

            if (killer_player)
            {
                if (details.killer_name == "")
                    details.killer_name = NameOfPlayer(killer_player);

                details.killer_health_pct = Math.Round(100 * killer_player.GetHealth01("", "Health"));
                details.killer_health_pct = Math.Clamp(details.killer_health_pct, 0, 100);
            }
        }
    }

    /**
     *  Was this player being burnt by the play area just now? Consumes the hint.
     *
     *  Scripted zone damage reaches EEKilled with the victim as their own source, which is
     *  indistinguishable from starvation or drowning - so the two damage sites drop a timestamp and
     *  this reads it. CONSUMED rather than merely read: a hint left behind would mislabel the next
     *  environmental death this player suffers, which is exactly why KillFeedDeath.ConsumeHint does
     *  the same.
     *
     *  BR-owned rather than borrowed from KillFeedAPI, because that addon is optional by contract.
     */
    static int ConsumeZoneHint(PlayerBase victim)
    {
        int hint_ms = 0;

        if (!victim)
            return BattleRoyaleKillCause.ENVIRONMENT;

        hint_ms = victim.br_zone_damage_ms;
        victim.br_zone_damage_ms = 0;

        if (hint_ms <= 0)
            return BattleRoyaleKillCause.ENVIRONMENT;

        if (GetGame().GetTime() - hint_ms > BR_KILL_HINT_TTL_MS)
            return BattleRoyaleKillCause.ENVIRONMENT;

        return BattleRoyaleKillCause.ZONE;
    }

    /**
     *  Best display name for a player, resolved name first.
     *
     *  Same order as BattleRoyaleLeaderboard.RecordExit: player_name already carries the resolved
     *  name when there is one, and the identity branch covers a cache that was never populated.
     */
    static string NameOfPlayer(PlayerBase player)
    {
        string name = "";

        if (!player)
            return "";

        name = player.player_name;
        if (name != "")
            return name;

        if (player.GetIdentity())
            name = BattleRoyaleNameService.ResolveIdentity(player.GetIdentity());
        if (name != "")
            return name;

        return player.GetCachedName();
    }

    //! Resolve a SteamID64 back to a PlayerBase if one still exists. A CORPSE counts - the mod never
    //! deletes a body - which is what lets a dead killer's own HUD counter still tick.
    static PlayerBase FindPlayerByUid(string uid)
    {
        array<Man> players = NULL;
        PlayerBase candidate = NULL;
        int count = 0;
        int i = 0;

        if (uid == "")
            return NULL;

        players = new array<Man>();
        GetGame().GetPlayers(players);

        count = players.Count();
        for (i = 0; i < count; i++)
        {
            candidate = PlayerBase.Cast(players.Get(i));
            if (!candidate)
                continue;

            //--- player_steamid first: it is cached on the mod's PlayerBase and survives the identity
            //--- going NULL, which is exactly the corpse case this exists for.
            if (candidate.player_steamid == uid)
                return candidate;

            if (!candidate.GetIdentity())
                continue;

            if (candidate.GetIdentity().GetPlainId() == uid)
                return candidate;
        }

        return NULL;
    }
}
#endif
