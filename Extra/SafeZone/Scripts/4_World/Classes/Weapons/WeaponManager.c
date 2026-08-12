/**
 *  SafeZone - the firing half of the truce.
 *
 *  WeaponManager.CanFire is the single script-side gate on the fire path: DayZPlayerImplement's
 *  HandleWeapons consults it once and every GetWeaponManager().Fire(weapon) call sits inside that
 *  branch. Refusing here means the trigger does nothing at all - no shot, no muzzle flash, no round
 *  consumed, no noise - while leaving weapon raise, ADS, reloading, bullet ejection and firearm
 *  melee bashing completely untouched.
 *
 *  That last part is the whole point. Expansion's safezone has no firing block; it stops shooting
 *  as a side effect of hic.OverrideRaise(true, false), which also costs you the ability to aim
 *  anything, melee included. Blocking the trigger directly is both narrower and more precise.
 *
 *  No guard: this compiles on both sides. The client refusal is the one players actually feel; the
 *  server refuses too, and a client that patches this out still lands zero damage because
 *  PlayerBase.EEOnDamageCalculated discards the hit.
 */
modded class WeaponManager
{
    override bool CanFire(Weapon_Base wpn)
    {
        if (m_player && m_player.VigridSafeZone_IsActive())
            return false;

        return super.CanFire(wpn);
    }
}
