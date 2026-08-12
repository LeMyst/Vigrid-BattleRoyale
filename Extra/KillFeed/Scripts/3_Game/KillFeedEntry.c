/**
 *  KillFeed - one row's worth of data. No guard: the server fills it in, the client renders it.
 *
 *  Pure data, deliberately: the client-side render state (the throwaway preview entity, the expiry
 *  stamp) lives on KillFeedRowModel in 5_Mission, so this class stays safe to build on the server.
 */
class KillFeedEntry
{
    string killer_name;   //!< "" when nobody is credited - a zone death, a fall, an unattributed mine
    string victim_name;
    string weapon_type;   //!< classname to render, "" for the weaponless causes
    string attachments;   //!< classnames joined by KILLFEED_ATTACHMENT_SEPARATOR, "" when bare
    int distance;         //!< metres, -1 to hide the field
    int cause;            //!< KillFeedCause

    void KillFeedEntry(string killer, string victim, string weapon, string attachment_list, int metres, int death_cause)
    {
        killer_name = killer;
        victim_name = victim;
        weapon_type = weapon;
        attachments = attachment_list;
        distance = metres;
        cause = death_cause;
    }

    //! True when the middle cell should render a model rather than an icon and a phrase.
    bool HasWeapon()
    {
        return weapon_type != "";
    }

    /**
     *  Attachment classnames, never null. Split here rather than at the call site so the join and
     *  the split can never disagree about the separator.
     */
    array<string> GetAttachmentTypes()
    {
        array<string> types = new array<string>();
        if (attachments == "")
            return types;

        attachments.Split(KILLFEED_ATTACHMENT_SEPARATOR, types);
        return types;
    }
}
