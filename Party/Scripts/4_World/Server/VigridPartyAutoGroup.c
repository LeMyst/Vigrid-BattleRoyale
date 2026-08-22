#ifdef SERVER
/**
 *  The result of planning one auto-group pass. Pure data - see VigridPartyAutoGroup below for what
 *  fills it in and why the planning is separated from the applying at all.
 *
 *  SLOTS. A slot is a party the plan may put players into. The leading `existing_count` slots are
 *  parties that already exist, in the order the caller listed them; every slot after that is a party
 *  the caller must create. `slot_of_pool[i]` is the slot pool member i ends up in, or -1 when the
 *  plan leaves them where they are.
 */
class VigridPartyAutoGroupPlan
{
    ref array<int> slot_of_pool; //!< per pool member: target slot, or -1 to stay as they are
    ref array<int> slot_size;    //!< final size of every slot, existing ones first
    int existing_count;          //!< how many leading entries of slot_size are pre-existing parties

    int groups_before;
    int groups_after;

    //! Set when the min_groups floor stopped the pass early. Not an error - see the class below.
    bool floor_hit;

    //! Members placed into a party that was already at max_size. Only ever non-zero in ABSORB mode.
    int overflow_count;

    //! Pool members the plan could not bring up to min_size, left as they were.
    int left_short;

    void VigridPartyAutoGroupPlan()
    {
        slot_of_pool = new array<int>();
        slot_size = new array<int>();
    }

    bool ChangedAnything()
    {
        int count = slot_of_pool.Count();
        for (int i = 0; i < count; i++)
        {
            if (slot_of_pool.Get(i) != -1)
                return true;
        }

        return false;
    }

    string Repr()
    {
        string text = "groups " + groups_before + "->" + groups_after + " sizes{";

        int count = slot_size.Count();
        for (int i = 0; i < count; i++)
        {
            if (i > 0)
                text = text + ",";
            if (i == existing_count)
                text = text + "|";
            text = text + slot_size.Get(i).ToString();
        }

        text = text + "}";
        if (left_short > 0)
            text = text + " short=" + left_short;
        if (overflow_count > 0)
            text = text + " overflow=" + overflow_count;
        if (floor_hit)
            text = text + " FLOOR";

        return text;
    }
}

/**
 *  Decides how to fill a population into parties of at least min_size. Arithmetic only - it never
 *  touches a party, a player or the network.
 *
 *  WHY IT IS SEPARATE. The interesting cases here are combinatorial (a top-up that also has a
 *  remainder, a four-way split, the max_size overflow) and the local test rig tops out at three
 *  clients, so most of them can never be reached by playing. Keeping the decisions in a pure
 *  function means a boot-time self-test can run hundreds of synthetic populations through the real
 *  code path in milliseconds - the same reason the zone generator's chain planner is testable.
 *  Anything that mutates state belongs in VigridPartyManager.AutoGroupPopulation, not here.
 *
 *  THE FLOOR IS NOT AN OPTIMISATION. `min_groups` is a hard floor on the number of distinct groups
 *  left standing, and it OUTRANKS min_size. A host that counts groups down to a winner needs at
 *  least two of them to count, so a pass that would merge the population into one party must stop
 *  short and leave somebody short-handed instead. Every merge below is gated on it individually,
 *  because a merge costs a different number of groups depending on what kind it is.
 */
class VigridPartyAutoGroup
{
    /**
     *  @param pool_size       players who are in no party and may be placed freely
     *  @param existing_sizes  size of each party already represented in the population, counting
     *                         only members who are actually present
     *  @param min_size        the party size to aim for; 1 or less is a no-op upstream
     *  @param max_size        the invite-flow cap, respected except where noted per remainder mode
     *  @param min_groups      floor on the resulting group count - wins over min_size
     *  @param remainder       one of VIGRID_PARTY_REMAINDER_*
     */
    static VigridPartyAutoGroupPlan Plan(int pool_size, array<int> existing_sizes, int min_size, int max_size, int min_groups, int remainder)
    {
        VigridPartyAutoGroupPlan plan = new VigridPartyAutoGroupPlan();

        int existing_count = 0;
        if (existing_sizes)
            existing_count = existing_sizes.Count();

        plan.existing_count = existing_count;
        for (int e = 0; e < existing_count; e++)
        {
            plan.slot_size.Insert(existing_sizes.Get(e));
        }

        for (int p = 0; p < pool_size; p++)
        {
            plan.slot_of_pool.Insert(-1);
        }

        //--- Every solo player is their own group, and every listed party is one more.
        int groups_now = pool_size + existing_count;
        plan.groups_before = groups_now;
        plan.groups_after = groups_now;

        if (pool_size <= 0)
            return plan;

        int next = 0; //!< next pool member awaiting a slot

        // ------------------------------------------------------------ 1. top up existing parties
        //
        // Undersized real parties are filled before any new party is started, so a duo that wanted
        // a third gets one rather than watching three strangers pair up beside them. A party that
        // already meets min_size is never touched and never split.
        for (int s = 0; s < existing_count; s++)
        {
            while (plan.slot_size.Get(s) < min_size && next < pool_size)
            {
                //--- Moving one solo into an existing party removes exactly one group.
                if ((groups_now - 1) < min_groups)
                {
                    plan.floor_hit = true;
                    return Finish(plan, groups_now, next, pool_size);
                }

                plan.slot_of_pool.Set(next, s);
                plan.slot_size.Set(s, plan.slot_size.Get(s) + 1);
                next = next + 1;
                groups_now = groups_now - 1;
            }
        }

        // ------------------------------------------------------------ 2. form new parties
        //
        // Exactly min_size each. The caller has already shuffled the pool, so who ends up with whom
        // - and therefore who leads, since the first member in becomes the leader - is random.
        int full_parties = (pool_size - next) / min_size;
        for (int f = 0; f < full_parties; f++)
        {
            //--- Fusing min_size solos into one party removes min_size - 1 groups.
            //---
            //--- Break rather than return: this merge costs min_size - 1 groups and the remainder
            //--- step below costs 1, so the floor can forbid a whole new party while still allowing
            //--- somebody to be tucked into an existing one. Whoever is left here IS the remainder
            //--- by definition - they cannot make a party of the required size - so they get the
            //--- remainder treatment the operator asked for, which re-checks the floor per merge.
            if ((groups_now - (min_size - 1)) < min_groups)
            {
                plan.floor_hit = true;
                break;
            }

            int new_slot = plan.slot_size.Count();
            plan.slot_size.Insert(0);

            for (int m = 0; m < min_size; m++)
            {
                plan.slot_of_pool.Set(next, new_slot);
                plan.slot_size.Set(new_slot, plan.slot_size.Get(new_slot) + 1);
                next = next + 1;
            }

            groups_now = groups_now - (min_size - 1);
        }

        // ------------------------------------------------------------ 3. the remainder
        //
        // Fewer than min_size players are left and they cannot make a full party between them, so
        // one rule or another has to bend. Which one is the operator's choice.
        if (next >= pool_size)
            return Finish(plan, groups_now, next, pool_size);

        if (remainder == VIGRID_PARTY_REMAINDER_SOLO)
            return Finish(plan, groups_now, next, pool_size);

        if (remainder == VIGRID_PARTY_REMAINDER_PARTY)
        {
            int short_size = pool_size - next;

            //--- Capped: step 2 can break out early on the floor and leave more players here than a
            //--- true remainder, and this mode's whole promise is that it never exceeds max_size.
            //--- Anyone past the cap stays solo.
            if (short_size > max_size)
                short_size = max_size;

            if (short_size < 2)
                return Finish(plan, groups_now, next, pool_size); //!< one player cannot form a party

            if ((groups_now - (short_size - 1)) < min_groups)
            {
                plan.floor_hit = true;
                return Finish(plan, groups_now, next, pool_size);
            }

            int short_slot = plan.slot_size.Count();
            plan.slot_size.Insert(0);

            for (int t = 0; t < short_size; t++)
            {
                plan.slot_of_pool.Set(next, short_slot);
                plan.slot_size.Set(short_slot, plan.slot_size.Get(short_slot) + 1);
                next = next + 1;
            }

            groups_now = groups_now - (short_size - 1);
            return Finish(plan, groups_now, next, pool_size);
        }

        //--- ABSORB: one at a time into the smallest party going, so the overflow spreads instead of
        //--- landing on one team. A slot below max_size always wins over a full one.
        while (next < pool_size)
        {
            int target = PickAbsorbTarget(plan.slot_size, max_size);
            if (target == -1)
                break; //!< no party exists to absorb into - a pool smaller than min_size and nothing else

            if ((groups_now - 1) < min_groups)
            {
                plan.floor_hit = true;
                break;
            }

            if (plan.slot_size.Get(target) >= max_size)
                plan.overflow_count = plan.overflow_count + 1;

            plan.slot_of_pool.Set(next, target);
            plan.slot_size.Set(target, plan.slot_size.Get(target) + 1);
            next = next + 1;
            groups_now = groups_now - 1;
        }

        return Finish(plan, groups_now, next, pool_size);
    }

    /**
     *  Smallest slot still under `max_size`; the smallest slot overall when every one of them is
     *  full, which is the case that overflows the cap. -1 when there are no slots at all.
     */
    private static int PickAbsorbTarget(array<int> slot_size, int max_size)
    {
        int best = -1;
        int best_full = -1;

        int count = slot_size.Count();
        for (int i = 0; i < count; i++)
        {
            int size = slot_size.Get(i);

            if (size < max_size)
            {
                if (best == -1 || size < slot_size.Get(best))
                    best = i;

                continue;
            }

            if (best_full == -1 || size < slot_size.Get(best_full))
                best_full = i;
        }

        if (best != -1)
            return best;

        return best_full;
    }

    private static VigridPartyAutoGroupPlan Finish(VigridPartyAutoGroupPlan plan, int groups_now, int placed, int pool_size)
    {
        plan.groups_after = groups_now;
        plan.left_short = pool_size - placed;
        return plan;
    }

    // ---------------------------------------------------------------- self test

    /**
     *  Run `cases` synthetic populations through Plan() and log what came out.
     *
     *  This exists because the cases that matter cannot be played. A local rig runs three clients,
     *  which is not enough to produce a four-way split, a top-up that also leaves a remainder, or the
     *  max_size overflow - and those are exactly where an off-by-one would either strand a player
     *  or collapse the lobby into one team and end the match on its first tick. Planning is pure
     *  arithmetic, so a few hundred of them cost microseconds and answer the question at boot.
     *
     *  Two invariants are checked rather than merely reported, and neither may ever be non-zero:
     *    - conservation: every player ends up in exactly one slot or is left where they were
     *    - the floor: a pass that started with enough groups never finishes below min_groups
     */
    static void SelfTest(int cases, int min_size, int max_size, int min_groups, int remainder)
    {
        if (cases <= 0)
            return;
        if (min_size <= 1)
            return;

        int ran = 0;
        int floor_hits = 0;
        int overflow_hits = 0;
        int short_hits = 0;
        int floor_broken = 0;
        int players_lost = 0;
        int biggest = 0;
        string first_failure = "";

        for (int variant = 0; variant < 5 && ran < cases; variant++)
        {
            for (int pool_size = 1; pool_size <= VIGRID_PARTY_AUTOGROUP_TEST_MAX_PLAYERS && ran < cases; pool_size++)
            {
                array<int> shape = BuildTestShape(variant, max_size);

                int seated = pool_size;
                int shape_count = shape.Count();
                for (int q = 0; q < shape_count; q++)
                {
                    seated = seated + shape.Get(q);
                }

                VigridPartyAutoGroupPlan plan = Plan(pool_size, shape, min_size, max_size, min_groups, remainder);
                ran = ran + 1;

                if (plan.floor_hit)
                    floor_hits = floor_hits + 1;
                if (plan.overflow_count > 0)
                    overflow_hits = overflow_hits + 1;
                if (plan.left_short > 0)
                    short_hits = short_hits + 1;

                int total = plan.left_short;
                int size_count = plan.slot_size.Count();
                for (int r = 0; r < size_count; r++)
                {
                    int size = plan.slot_size.Get(r);
                    total = total + size;

                    if (size > biggest)
                        biggest = size;
                }

                if (total != seated)
                {
                    players_lost = players_lost + 1;
                    if (first_failure == "")
                        first_failure = "conservation, shape " + variant + " pool " + pool_size;
                }

                //--- Only meaningful when there were enough groups to begin with: a one-player
                //--- population starts below the floor and is left alone, which is correct.
                if (plan.groups_before >= min_groups && plan.groups_after < min_groups)
                {
                    floor_broken = floor_broken + 1;
                    if (first_failure == "")
                        first_failure = "floor, shape " + variant + " pool " + pool_size;
                }
            }
        }

        string line = "AutoGroup selftest " + ran + " cases at min_size=" + min_size;
        line = line + " max_size=" + max_size + " floor=" + min_groups + " remainder=" + remainder;
        VigridPartyLog.Info(line);

        string counts = "AutoGroup selftest short=" + short_hits + " floor_stops=" + floor_hits;
        counts = counts + " overflows=" + overflow_hits + " biggest_team=" + biggest;
        VigridPartyLog.Info(counts);

        if (players_lost == 0 && floor_broken == 0)
        {
            VigridPartyLog.Info("AutoGroup selftest PASSED - conservation and the group floor both held");
            return;
        }

        string failure = "AutoGroup selftest FAILED - lost=" + players_lost + " broke_floor=" + floor_broken;
        failure = failure + " first at " + first_failure;
        VigridPartyLog.Error(failure);
    }

    //! A spread of pre-made party shapes to plan against: none, a pair, two pairs, a trio and a
    //! pair, and one party already at the cap.
    private static array<int> BuildTestShape(int variant, int max_size)
    {
        array<int> shape = new array<int>();

        if (variant == 1)
        {
            shape.Insert(2);
        }
        else if (variant == 2)
        {
            shape.Insert(2);
            shape.Insert(2);
        }
        else if (variant == 3)
        {
            shape.Insert(3);
            shape.Insert(2);
        }
        else if (variant == 4)
        {
            shape.Insert(max_size);
        }

        return shape;
    }
}
#endif
