modded class DayZPlayerInventory {
	bool HandleInputData (bool juncture, bool remote, ParamsReadContext ctx)
	{
		int type = -1;
		if (!ctx.Read(type))
			return false;

		InventoryLocation src = new InventoryLocation;
		InventoryLocation dst = new InventoryLocation;

		switch (type)
		{
			case InventoryCommandType.SYNC_MOVE:
			{
				src.ReadFromContext(ctx);
				dst.ReadFromContext(ctx);

				if (remote && (!src.GetItem() || !dst.GetItem()))
				{
					//Error("[syncinv] remote input (cmd=SYNC_MOVE) dropped, item not in bubble! src=" + src.DumpToString() + " dst=" + dst.DumpToString());
					break; // not in bubble
				}
				
				//PATCH
				if (false == GameInventory.CheckMoveToDstRequest(GetManOwner(), src.GetItem(), dst))
				{
					//Error("[cheat] man=" + GetManOwner() + " is cheating with cmd=" + typename.EnumToString(InventoryCommandType, type) + " src=" + src.DumpToString() + " dst=" + dst.DumpToString());
					return false; // cheater
				}
				
				syncDebugPrint("[syncinv] t=" + GetGame().GetTime() + "ms received cmd=" + typename.EnumToString(InventoryCommandType, type) + " src=" + src.DumpToString() + " dst=" + dst.DumpToString());
				if (!juncture && GetDayZPlayerOwner().GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_SERVER)
				{
					if (GetDayZPlayerOwner().NeedInventoryJunctureFromServer(src.GetItem(), src.GetParent(), dst.GetParent()))
					{
						syncDebugPrint("[syncinv] redir to juncture cmd=" + typename.EnumToString(InventoryCommandType, type) + " dst=" + dst.DumpToString());
						if (GetGame().AddInventoryJuncture(GetDayZPlayerOwner(), src.GetItem(), dst, true, GameInventory.c_InventoryReservationTimeoutMS))
							GetDayZPlayerOwner().SendSyncJuncture(DayZPlayerSyncJunctures.SJ_INVENTORY, ctx);
						else
						{
							syncDebugPrint("[syncinv] redir to juncture DENIED");
							return true; // abort, do not send anything to remotes
						}
						break;
					}
				}

				LocationSyncMoveEntity(src, dst);
				break;
			}

			case InventoryCommandType.HAND_EVENT:
			{
				HandEventBase e = HandEventBase.CreateHandEventFromContext(ctx);

				if (remote && !e.m_Entity)
				{
					//PATCH
					//Error("[syncinv] remote input (cmd=HAND_EVENT) dropped, item not in bubble");
					break; // not in bubble
				}
				
				//PATCH
				if (false == e.CheckRequest())
				{
					//Error("[cheat] man=" + GetManOwner() + " is cheating with cmd=" + typename.EnumToString(InventoryCommandType, type) + " event=" + e);
					return false; // cheater
				}

				syncDebugPrint("[syncinv] t=" + GetGame().GetTime() + "ms received cmd=" + typename.EnumToString(InventoryCommandType, type) + " event=" + e);
				if (e.m_Entity.GetInventory().GetCurrentInventoryLocation(src))
				{
					dst = e.GetDst();
					if (!juncture && GetDayZPlayerOwner().GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_SERVER)
					{
						if (GetDayZPlayerOwner().NeedInventoryJunctureFromServer(src.GetItem(), src.GetParent(), dst.GetParent()))
						{
							syncDebugPrint("[syncinv] redir to juncture cmd=" + typename.EnumToString(InventoryCommandType, type) + " event=" + e);
							bool test_dst_occupancy = !e.IsSwapEvent();
							if (GetGame().AddInventoryJuncture(GetDayZPlayerOwner(), e.m_Entity, dst, test_dst_occupancy, GameInventory.c_InventoryReservationTimeoutMS))
								GetDayZPlayerOwner().SendSyncJuncture(DayZPlayerSyncJunctures.SJ_INVENTORY, ctx);
							else
							{
								syncDebugPrint("[syncinv] redir to juncture DENIED");
								return true; // abort, do not send anything to remotes
							}
							break;
						}
					}

					e.m_Player.GetHumanInventory().PostHandEvent(e);
				}
				//else
					//Error("OnInputData HandEvent - no inv loc");
				break;
			}

			case InventoryCommandType.SWAP:
			{
				EntityAI item1, item2;
				ctx.Read(item1);
				ctx.Read(item2);
				
				if (remote && (!item1 || !item2))
				{
					//Error("[syncinv] remote input (cmd=SWAP) dropped, item not in bubble");
					break; // not in bubble
				}

				//PATCH
				if (false == GameInventory.CheckSwapItemsRequest(GetManOwner(), item1, item2))
				{
					//Error("[cheat] man=" + GetManOwner() + " is cheating with cmd=" + typename.EnumToString(InventoryCommandType, type) + " src=" + src.DumpToString() + " dst=" + dst.DumpToString());
					return false; // cheater
				}

				InventoryLocation src1, src2, dst1, dst2;
				if (GameInventory.MakeSrcAndDstForSwap(item1, item2, src1, src2, dst1, dst2))
				{
					dst1.Copy(src1);
					dst1.CopyLocationFrom(src2);
					dst2.Copy(src2);
					dst2.CopyLocationFrom(src1);
					syncDebugPrint("[syncinv] t=" + GetGame().GetTime() + "ms received Swap src1=" + src1.DumpToString() + "dst1=" + dst1.DumpToString() +  "src2=" + src2.DumpToString() + "dst2=" + dst2.DumpToString());

					if (!juncture && GetDayZPlayerOwner().GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_SERVER)
					{
						bool need_j1 = GetDayZPlayerOwner().NeedInventoryJunctureFromServer(src1.GetItem(), src1.GetParent(), dst1.GetParent());
						bool need_j2 = GetDayZPlayerOwner().NeedInventoryJunctureFromServer(src2.GetItem(), src2.GetParent(), dst2.GetParent());
						if (need_j1 || need_j2)
						{
							syncDebugPrint("[syncinv] redir to juncture cmd=" + typename.EnumToString(InventoryCommandType, type) + " dst1=" + dst1.DumpToString());
							bool acq_j1 = true;
							if (need_j1)
								GetGame().AddInventoryJuncture(GetDayZPlayerOwner(), item1, dst1, false, GameInventory.c_InventoryReservationTimeoutMS);
							
							bool acq_j2 = true;
							if (need_j2)
								GetGame().AddInventoryJuncture(GetDayZPlayerOwner(), item2, dst2, false, GameInventory.c_InventoryReservationTimeoutMS);
							if (acq_j1 && acq_j2)
								GetDayZPlayerOwner().SendSyncJuncture(DayZPlayerSyncJunctures.SJ_INVENTORY, ctx);
							else
							{
								syncDebugPrint("[syncinv] redir to juncture DENIED");
								return true; // abort, do not send anything to remotes
							}
							break;
						}
					}

					LocationSwap(src1, src2, dst1, dst2);
				}
				//else
					//Error("MakeSrcAndDstForSwap - no inv loc");
				break;
			}

			case InventoryCommandType.FORCESWAP:
			{
				//Error("[syncinv] ForceSwap TODO");
				break;
			}

			case InventoryCommandType.DESTROY:
			{
				src.ReadFromContext(ctx);
				syncDebugPrint("[syncinv] t=" + GetGame().GetTime() + "ms received cmd=" + typename.EnumToString(InventoryCommandType, type) + " src=" + src.DumpToString());

				if (remote && !src.GetItem())
				{
					//Error("[syncinv] remote input (cmd=DESTROY) dropped, item not in bubble");
					break; // not in bubble
				}

				//PATCH
				if (false == GameInventory.CheckDropRequest(GetManOwner(), src.GetItem()))
				{
					//Error("[cheat] man=" + GetManOwner() + " is cheating with cmd=" + typename.EnumToString(InventoryCommandType, type) + " src=" + src.DumpToString());
					return false; // cheater
				}

				GetGame().ObjectDelete(src.GetItem());
				break;
			}
		}

		if (!juncture && !remote && GetDayZPlayerOwner().GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_SERVER)
		{
			syncDebugPrint("[syncinv] forward to remotes cmd=" + typename.EnumToString(InventoryCommandType, type));
			GetDayZPlayerOwner().StoreInputForRemotes(ctx); // @NOTE: needs to be called _after_ the operation
		}

		return true;
	}
	
	
	
}