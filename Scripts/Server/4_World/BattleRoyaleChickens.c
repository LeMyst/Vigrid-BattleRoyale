#ifdef SERVER
/**
 *  Chickens, and the one place that knows how to put one in the world.
 *
 *  Two callers want the same thing for entirely different reasons - 8_BattleRoyaleWin rains birds on
 *  the winner, and the lobby drips one or two into the waiting area every few minutes - and the
 *  arithmetic they need is identical: pick a spot in a ring around a point, refuse water, drop the
 *  Y onto the terrain, create, face a random way. Written out twice, the two copies drift; that is
 *  the whole reason BattleRoyaleKillAttribution exists, and it applies just as well to a chicken.
 *
 *  Three things are worth knowing before touching this:
 *
 *    - CreateObject RESOLVES THROUGH CONFIG, so a classname that is scope = 0 comes back NULL rather
 *      than erroring. Animal_GallusGallusDomesticusF - the hen base, no colour suffix - is exactly
 *      that, and it sits one line away in the vanilla config from the three that are spawnable.
 *    - The legacy CreateObject(type, pos, create_local, init_ai) form is used rather than vanilla
 *      PlayerBase.SpawnAI's CreateObjectEx(ECE_PLACE_ON_SURFACE|ECE_INITAI|ECE_EQUIP_ATTACHMENTS).
 *      Both are vanilla-sanctioned - the Santa sleigh wreck spawns its deer exactly this way - but
 *      this is the form already proven on this mod's dedicated server, and CLAUDE.md records
 *      CreateObjectEx hard-crashing here for a character class. If a chicken ever turns out to stand
 *      still rather than wander, the missing piece is ECE_UPDATEPATHGRAPH (which lives inside
 *      ECE_PLACE_ON_SURFACE) - that, and nothing else, is the reason to change it.
 *    - Chickens ReplaceOnDeath(). Vanilla AnimalBase.DeathUpdate deletes the animal outright and
 *      swaps in a DeadChicken_* / DeadRooster food item a moment later, so a handle kept by a caller
 *      NULLS ITSELF once the bird is killed. A caller counting live birds gets that for free, and
 *      only has to cover the brief window between the death and the swap - which IsAlive() does.
 */
class BattleRoyaleChickens
{
    /**
     *  One of the four spawnable vanilla chickens, at random.
     *
     *  Returns the classname rather than the list, deliberately: a returned array has no owner and
     *  the lifetime question does not need to exist for something this small.
     */
    static string RandomType()
    {
        //--- All scope = 2 in vanilla. DO NOT add Animal_GallusGallusDomesticusF (no colour suffix)
        //--- to this list - it is the scope = 0 hen base and CreateObject would answer NULL.
        array<string> types = {
            "Animal_GallusGallusDomesticus",
            "Animal_GallusGallusDomesticusF_Brown",
            "Animal_GallusGallusDomesticusF_Spotted",
            "Animal_GallusGallusDomesticusF_White"
        };

        return types.GetRandomElement();
    }

    /**
     *  Put one chicken somewhere in the ring [min_radius, max_radius] around `center`, or answer
     *  NULL when it could not be placed.
     *
     *  Only water is rejected. A player-sized collision test would be the wrong bar for a bird, and
     *  the one this repo already has (BattleRoyaleState.IsSafeForTeleport) is protected on the state
     *  and unreachable from here anyway.
     */
    static EntityAI SpawnOne(vector center, float min_radius, float max_radius)
    {
        vector position = "0 0 0";
        vector facing = "0 0 0";
        EntityAI chicken = NULL;
        float distance = 0;
        float angle = 0;
        int attempt = 0;
        bool dry = false;

        for (attempt = 0; attempt < BR_LOBBY_CHICKEN_PLACE_TRIES; attempt++)
        {
            distance = Math.RandomFloatInclusive(min_radius, max_radius);
            angle = Math.RandomFloat(0, 360) * Math.DEG2RAD;

            position[0] = center[0] + (distance * Math.Cos(angle));
            position[2] = center[2] + (distance * Math.Sin(angle));

            if (GetGame().SurfaceIsSea(position[0], position[2]))
                continue;

            if (GetGame().SurfaceIsPond(position[0], position[2]))
                continue;

            dry = true;
            break;
        }

        if (!dry)
        {
            BattleRoyaleUtils.Debug("[Chickens] no dry spot in the ring, skipping this bird");
            return NULL;
        }

        position[1] = GetGame().SurfaceY(position[0], position[2]);

        chicken = EntityAI.Cast(GetGame().CreateObject(RandomType(), position, false, true));

        //--- CreateObject answers NULL for anything it cannot resolve, and says nothing. The call
        //--- site this was lifted from dereferenced the result unguarded for two years.
        if (!chicken)
        {
            BattleRoyaleUtils.Warn("[Chickens] CreateObject returned NULL - is the classname scope 2?");
            return NULL;
        }

        angle = Math.RandomFloat(0, 360);
        facing = vector.YawToVector(angle);
        chicken.SetDirection(Vector(facing[0], 0, facing[1]));

        return chicken;
    }

    /**
     *  Place up to `count` chickens around `center`, appending each one that really appeared to
     *  `spawned`. Returns how many were made, which is not always `count`.
     *
     *  `spawned` may be NULL for a caller that does not want to keep track of them.
     */
    static int SpawnBatch(vector center, float min_radius, float max_radius, int count, array<EntityAI> spawned)
    {
        EntityAI chicken = NULL;
        int made = 0;
        int i = 0;

        for (i = 0; i < count; i++)
        {
            chicken = SpawnOne(center, min_radius, max_radius);
            if (!chicken)
                continue;

            made++;

            if (spawned)
                spawned.Insert(chicken);
        }

        return made;
    }
};
#endif
