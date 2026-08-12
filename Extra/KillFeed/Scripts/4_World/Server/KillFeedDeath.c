#ifdef SERVER
/**
 *  KillFeed - death classification and broadcast. The whole server side of the addon.
 *
 *  Reached from the modded PlayerBase.EEKilled hook, so it works on a bare DayZ server; the host
 *  game only ever contributes optional hints through KillFeedAPI.
 *
 *  Attribution follows vanilla PluginAdminLog.PlayerKilled: for a shot or a swing the `source` IS
 *  the weapon, and the killer is its hierarchy parent. For fists the source is the player.
 */
class KillFeedDeath
{
    //--- Runtime gate on top of the `enabled` setting, driven by KillFeedAPI.SetActive. Defaults
    //--- to on so a server that never calls the API still gets a feed.
    private static bool m_Active = true;

    static void SetActive(bool active)
    {
        if (m_Active == active)
            return;

        m_Active = active;
        KillFeedLog.Info("Feed active = " + active);
    }

    static bool IsActive()
    {
        return m_Active;
    }

    static void OnPlayerKilled(PlayerBase victim, Object source)
    {
        if (!victim)
            return;
        if (!m_Active)
            return;

        KillFeedData settings = KillFeedConfig.GetConfig().GetSettings();
        if (!settings.enabled)
            return;

        string victim_name = NameOf(victim);
        if (victim_name == "")
        {
            KillFeedLog.Debug("Skipping a death with no resolvable victim name");
            return;
        }

        //--- EnfusionScript allows one declaration per name per method scope, so everything the
        //--- cascade below writes to is declared up front.
        int cause = KillFeedCause.ENVIRONMENT;
        string killer_name = "";
        string weapon_type = "";
        string attachments = "";
        int distance = -1;

        EntityAI source_entity = EntityAI.Cast(source);
        PlayerBase killer_player = NULL;

        if (!source || source == victim)
        {
            //--- Nothing hit them: zone damage, a fall, starvation, drowning. The host game may
            //--- have told us which, in which case that wins.
            cause = ConsumeHint(victim);
        }
        else if (source_entity && IsExplosive(source_entity))
        {
            cause = KillFeedCause.EXPLOSIVE;
            killer_name = ResolveActivatorName(source_entity);
        }
        else if (source.IsWeapon() || source.IsMeleeWeapon())
        {
            if (source_entity)
                killer_player = PlayerBase.Cast(source_entity.GetHierarchyParent());

            if (killer_player)
            {
                killer_name = NameOf(killer_player);
                weapon_type = source.GetType();
                attachments = CollectAttachments(source_entity);

                if (source.IsMeleeWeapon())
                {
                    cause = KillFeedCause.MELEE;
                }
                else
                {
                    cause = KillFeedCause.WEAPON;
                    if (settings.show_distance)
                        distance = Math.Round(vector.Distance(victim.GetPosition(), killer_player.GetPosition()));
                }
            }
        }
        else if (source.IsInherited(PlayerBase))
        {
            killer_player = PlayerBase.Cast(source);
            killer_name = NameOf(killer_player);
            cause = KillFeedCause.BAREHANDS;
        }
        else if (source.IsInherited(ZombieBase))
        {
            cause = KillFeedCause.INFECTED;
        }
        else if (source.IsInherited(AnimalBase))
        {
            cause = KillFeedCause.ANIMAL;
        }

        //--- A row with no name to credit is an environmental death however it was reached, which
        //--- is exactly the set the setting gates.
        if (killer_name == "" && !settings.show_environment_deaths)
        {
            KillFeedLog.Debug("Suppressing environmental death of " + victim_name);
            return;
        }

        //--- A player cannot be their own killer on a feed row; it reads as a bug.
        if (killer_name == victim_name)
            killer_name = "";

        Broadcast(killer_name, victim_name, weapon_type, attachments, distance, cause);
    }

    private static void Broadcast(string killer_name, string victim_name, string weapon_type, string attachments, int distance, int cause)
    {
        KillFeedLog.Info(string.Format("%1 killed %2 (cause=%3 weapon=%4 dist=%5)", killer_name, victim_name, cause, weapon_type, distance));

        //--- No identity argument: that is what makes CF fan the message out to every client.
        GetRPCManager().SendRPC(RPC_KILLFEED_NAMESPACE, KF_RPC_ENTRY,
            new Param6<string, string, string, string, int, int>(
                killer_name,
                victim_name,
                weapon_type,
                attachments,
                distance,
                cause),
            true);
    }

    /**
     *  Everything attached to the weapon, as classnames. Iterated with AttachmentCount() rather
     *  than GetAttachmentSlotsCount() - the former counts what is actually attached, the latter
     *  counts declared slots and would mostly yield nulls.
     *
     *  Magazines are deliberately included: they are visible on the model, and rendering the gun
     *  exactly as the killer carried it is the whole point of the feature.
     */
    private static string CollectAttachments(EntityAI weapon)
    {
        string result = "";
        if (!weapon)
            return result;

        GameInventory inventory = weapon.GetInventory();
        if (!inventory)
            return result;

        int count = inventory.AttachmentCount();
        for (int i = 0; i < count; i++)
        {
            EntityAI attachment = inventory.GetAttachmentFromIndex(i);
            if (!attachment)
                continue;

            if (result != "")
                result = result + KILLFEED_ATTACHMENT_SEPARATOR;

            result = result + attachment.GetType();
        }

        return result;
    }

    private static bool IsExplosive(EntityAI source)
    {
        if (source.IsInherited(Grenade_Base))
            return true;

        return source.IsInherited(LandMineTrap);
    }

    /**
     *  Who armed the grenade or mine. Read defensively: the host mod records m_ActivatorId on its
     *  own modded Grenade_Base, a bare DayZ server does not, and the row still renders without it.
     */
    private static string ResolveActivatorName(EntityAI source)
    {
        string uid = "";
        EnScript.GetClassVar(source, "m_ActivatorId", -1, uid);
        if (uid == "")
            return "";

        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);

        int count = players.Count();
        for (int i = 0; i < count; i++)
        {
            Man candidate = players.Get(i);
            if (!candidate)
                continue;

            PlayerIdentity identity = candidate.GetIdentity();
            if (!identity)
                continue;

            if (identity.GetPlainId() == uid)
                return identity.GetName();
        }

        return "";
    }

    /**
     *  Take the cause the host game pushed in, if it is still fresh. Consumed rather than merely
     *  read: a hint describes one death, and leaving it set would mislabel the next one.
     */
    private static int ConsumeHint(PlayerBase victim)
    {
        int cause = KillFeedCause.ENVIRONMENT;

        if (victim.m_KillFeedHintCause != KillFeedCause.INVALID)
        {
            int age = GetGame().GetTime() - victim.m_KillFeedHintTime;
            if (age >= 0 && age <= KILLFEED_HINT_TTL_MS)
                cause = victim.m_KillFeedHintCause;
        }

        victim.m_KillFeedHintCause = KillFeedCause.INVALID;
        victim.m_KillFeedHintTime = 0;

        return cause;
    }

    //! Identity first, cached name as the fallback - a corpse can already have lost its identity.
    private static string NameOf(Man player)
    {
        if (!player)
            return "";

        PlayerIdentity identity = player.GetIdentity();
        if (identity)
            return identity.GetName();

        PlayerBase base_player = PlayerBase.Cast(player);
        if (base_player)
            return base_player.GetCachedName();

        return "";
    }
}
#endif
