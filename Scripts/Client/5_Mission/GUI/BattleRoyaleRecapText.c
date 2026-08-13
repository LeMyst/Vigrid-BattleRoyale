#ifndef SERVER
/**
 *  Battle Royale - the death recap, rendered as text.
 *
 *  ONE formatter, deliberately, because the same recap is now drawn in two places from two different
 *  transports: the death screen paints it from the live push at the moment of death, and the lobby's
 *  Last Match tab paints it from the file written at the end of that match. They must agree, and a
 *  second copy of this logic would drift. Both read the same BattleRoyaleRPC fields, which is the
 *  other half of that defence.
 *
 *  Every line here is built in STEPS. A single expression carrying killer, weapon, range, cause,
 *  health and damage-dealt is exactly the shape that trips EnfusionScript's "Formula too complex",
 *  which is a hard compile error that packing does not catch - it only surfaces when the game loads
 *  the module.
 */
class BattleRoyaleRecapText
{
    //! classname -> localised display name. Resolving one is three config probes, and a scrolling
    //! table can ask for the same weapon on many rows.
    private static ref map<string, string> s_NameCache;

    /**
     *  The headline: who finished you, with what, and from how far.
     *
     *  Returns "" when there is nothing to say, which the caller renders as an empty widget rather
     *  than a placeholder - the death screen's dialog is transparent, so an empty line is invisible.
     */
    static string BuildRecapLine(int cause, string killer_name, string weapon_type, int distance_m)
    {
        string weapon = "";
        string line = "";

        //--- Causes with nobody to name. Handled first so the killer-name tests below never have to
        //--- consider them.
        if (cause == BattleRoyaleKillCause.NONE)
            return Widget.TranslateString("#STR_BR_LASTMATCH_RECAP_WON");

        if (cause == BattleRoyaleKillCause.ZONE)
            return Widget.TranslateString("#STR_BR_DEAD_RECAP_ZONE");

        if (cause == BattleRoyaleKillCause.ENVIRONMENT)
            return Widget.TranslateString("#STR_BR_DEAD_RECAP_ENV");

        if (killer_name == "")
        {
            //--- Infected and animals reach here: a real cause, but no name to print.
            if (cause == BattleRoyaleKillCause.INFECTED)
                return CauseLabel(cause);
            if (cause == BattleRoyaleKillCause.ANIMAL)
                return CauseLabel(cause);

            return Widget.TranslateString("#STR_BR_DEAD_RECAP_UNKNOWN");
        }

        //--- Bare hands and the degraded disconnect path both have a killer and no weapon worth
        //--- naming. The disconnect path deliberately carries no distance either: the killer may
        //--- have moved a kilometre since the hit that downed them, and a plausible wrong number is
        //--- worse than none.
        weapon = WeaponDisplayName(weapon_type);
        if (weapon == "")
        {
            line = Widget.TranslateString("#STR_BR_DEAD_RECAP_BY");
            return string.Format(line, killer_name);
        }

        if (distance_m < 0)
        {
            line = Widget.TranslateString("#STR_BR_DEAD_RECAP_BY_WEAPON");
            return string.Format(line, killer_name, weapon);
        }

        line = Widget.TranslateString("#STR_BR_DEAD_RECAP_BY_WEAPON_RANGE");
        return string.Format(line, killer_name, weapon, distance_m);
    }

    /**
     *  The second line: how the killer was left, and what you had already taken off them.
     *
     *  Both halves are optional and each is only shown when it is actually known, so a death with
     *  neither renders nothing at all rather than "on -1 health".
     */
    static string BuildRecapDetail(int killer_health_pct, int damage_to_killer)
    {
        string detail = "";
        string part = "";

        if (killer_health_pct >= 0)
        {
            part = Widget.TranslateString("#STR_BR_DEAD_RECAP_HEALTH");
            detail = string.Format(part, killer_health_pct);
        }

        if (damage_to_killer > 0)
        {
            part = Widget.TranslateString("#STR_BR_DEAD_RECAP_DEALT");
            part = string.Format(part, damage_to_killer);

            if (detail != "")
                detail = detail + " ";

            detail = detail + part;
        }

        return detail;
    }

    //! Short label for a cause, for the causes that have no killer to name.
    static string CauseLabel(int cause)
    {
        if (cause == BattleRoyaleKillCause.FIREARM)
            return Widget.TranslateString("#STR_BR_CAUSE_FIREARM");
        if (cause == BattleRoyaleKillCause.MELEE)
            return Widget.TranslateString("#STR_BR_CAUSE_MELEE");
        if (cause == BattleRoyaleKillCause.BAREHANDS)
            return Widget.TranslateString("#STR_BR_CAUSE_BAREHANDS");
        if (cause == BattleRoyaleKillCause.EXPLOSIVE)
            return Widget.TranslateString("#STR_BR_CAUSE_EXPLOSIVE");
        if (cause == BattleRoyaleKillCause.ZONE)
            return Widget.TranslateString("#STR_BR_CAUSE_ZONE");
        if (cause == BattleRoyaleKillCause.INFECTED)
            return Widget.TranslateString("#STR_BR_CAUSE_INFECTED");
        if (cause == BattleRoyaleKillCause.ANIMAL)
            return Widget.TranslateString("#STR_BR_CAUSE_ANIMAL");
        if (cause == BattleRoyaleKillCause.ENVIRONMENT)
            return Widget.TranslateString("#STR_BR_CAUSE_ENVIRONMENT");

        return Widget.TranslateString("#STR_BR_CAUSE_UNKNOWN");
    }

    /**
     *  Classname -> localised display name.
     *
     *  Resolved on the CLIENT, which is why only the classname travels: the server has no idea what
     *  language each client is running, so a name resolved there would be wrong for every player not
     *  sharing its locale. This is the same contract as the mod's server-ships-the-bare-key rule for
     *  notifications.
     *
     *  Three roots because DayZ splits them: firearms and melee live in CfgWeapons, most items in
     *  CfgVehicles, ammunition in CfgMagazines. ConfigGetTextOut is the localising variant (the Raw
     *  forms hand back the untranslated key). Falls back to the raw classname rather than blank, so
     *  an unknown item still reads as something.
     */
    static string WeaponDisplayName(string classname)
    {
        string resolved = "";

        if (classname == "")
            return "";

        if (!s_NameCache)
            s_NameCache = new map<string, string>();

        if (s_NameCache.Contains(classname))
            return s_NameCache.Get(classname);

        resolved = LookupDisplayName(CFG_WEAPONSPATH, classname);
        if (resolved == "")
            resolved = LookupDisplayName(CFG_VEHICLESPATH, classname);
        if (resolved == "")
            resolved = LookupDisplayName(CFG_MAGAZINESPATH, classname);
        if (resolved == "")
            resolved = classname;

        s_NameCache.Set(classname, resolved);
        return resolved;
    }

    private static string LookupDisplayName(string config_root, string classname)
    {
        string path = config_root + " " + classname + " displayName";

        if (!GetGame().ConfigIsExisting(path))
            return "";

        return GetGame().ConfigGetTextOut(path);
    }

    //! Seconds as m:ss. Not a stringtable entry - a colon-separated duration reads the same in every
    //! language this mod ships.
    static string FormatDuration(int seconds)
    {
        int minutes = 0;
        int remainder = 0;

        if (seconds < 0)
            seconds = 0;

        minutes = seconds / 60;
        remainder = seconds % 60;

        if (remainder < 10)
            return minutes.ToString() + ":0" + remainder.ToString();

        return minutes.ToString() + ":" + remainder.ToString();
    }
}
#endif
