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
    //! Pulling the pin is what makes a grenade somebody's grenade.
    override void OnUnpin()
    {
        super.OnUnpin();

        BR_SetActivator(GetHierarchyRootPlayer());
    }
}
#endif
