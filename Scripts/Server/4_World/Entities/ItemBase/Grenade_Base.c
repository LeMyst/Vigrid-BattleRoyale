#ifdef SERVER
/**
 *  Grenade_Base : ExplosivesBase, so the activator fields, the getters and the placement/destruction
 *  hooks are all inherited - see ExplosivesBase.c. Only the pin is specific to a grenade.
 *
 *  Do NOT re-declare m_ActivatorId here: it would shadow the parent's, and the shared resolver reads
 *  the parent's through the getter.
 */
modded class Grenade_Base
{
    //! Pulling the pin by hand is what makes a grenade somebody's grenade.
    override void OnUnpin()
    {
        super.OnUnpin();

        BR_SetActivator(GetHierarchyRootPlayer());
    }

    /**
     *  A trap or an IED set this grenade off - inherit whoever armed THAT.
     *
     *  This is the grenade-on-a-tripwire case. TripwireTrap.SetInactive calls
     *  attachment.OnActivatedByItem(this) and then DROPS the attachment, so the GRENADE is what
     *  kills while the TRAP is what knew the owner. Vanilla answers by calling Unpin(), which does
     *  reach OnUnpin above - but by then the grenade's hierarchy root is the trap rather than a
     *  player, so it resolves nobody and the kill reads as environmental.
     *
     *  ⚠️ Hooked HERE and not on ExplosivesBase: vanilla's Grenade_Base.OnActivatedByItem does not
     *  call super, so a parent-level override never runs. The same is true of ClaymoreMine,
     *  ImprovisedExplosive and Plastic_Explosive - add a leaf hook per device, never a shared one.
     *
     *  Before super, because super is what starts the fuse.
     */
    override void OnActivatedByItem(notnull ItemBase item)
    {
        BR_InheritActivatorFrom(item);

        super.OnActivatedByItem(item);
    }
}
#endif
