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
