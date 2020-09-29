#include "Lootwhore.h"

void Lootwhore::HandleTreasureSlot(int Slot)
{
    //Grab a pointer to slot info, skip if empty.
    TreasurePoolSlot_t* treasureItem = &mState.PoolSlots[Slot];
    if (treasureItem->Id == 0)
        return;

    //Skip if we've already acted on it.
    if (treasureItem->Status != LotState::Untouched)
        return;

    //Skip if we've tried the maximum amount of times.
    if (treasureItem->PacketAttempts >= mSettings.MaxRetry)
        return;

    //Skip if we've sent a packet for the slot in the last 5 seconds.
    std::chrono::time_point<std::chrono::steady_clock> comparand = std::chrono::steady_clock::now() - std::chrono::seconds(5);
    if (comparand < treasureItem->LastAction)
        return;

    //Check rarepass first.
    if (CheckRarePass(treasureItem->Id))
    {
        PassItem(Slot);
        return;
    }

    //Check individual item lot setting next.
    std::map<uint16_t, LotReaction>::iterator iter = mProfile.ItemMap.find(treasureItem->Id);
    if (iter != mProfile.ItemMap.end())
    {
        if (iter->second == LotReaction::Lot)
        {
            LotItem(Slot);
            return;
        }
        else if (iter->second == LotReaction::Pass)
        {
            PassItem(Slot);
            return;
        }
        else
            return;
    }

    //Check SmartPass next.
    if (mProfile.SmartPass == SmartPassSetting::ListOnly)
    {
        for (std::list<LotInfo_t>::iterator iter = treasureItem->LotList.begin(); iter != treasureItem->LotList.end(); iter++)
        {
            if (std::find(mSettings.WhiteList.begin(), mSettings.WhiteList.end(), iter->Name) != mSettings.WhiteList.end())
            {
                PassItem(Slot);
                return;
            }
        }
    }
    else if (mProfile.SmartPass == SmartPassSetting::Everyone)
    {
        if (treasureItem->LotList.size() > 0)
        {
            PassItem(Slot);
            return;
        }
    }

    //Finally, check default action.
    if (mProfile.DefaultReaction == LotReaction::Lot)
    {
        LotItem(Slot);
    }
    else if (mProfile.DefaultReaction == LotReaction::Pass)
    {
        PassItem(Slot);
    }
}

void Lootwhore::HandleInventory()
{
    //Update mState.hasContainer to show currently available bags.
    checkBags();

    //Calculate free slots for each bag.
    int FreeSpace[13] = {0};
    for (std::list<int>::iterator iter = mSettings.StoreBags.begin(); iter != mSettings.StoreBags.end(); iter++)
    {
        if (!mState.hasContainer[*iter])
            continue;

        FreeSpace[*iter] = 0;
        
        int containerMax = m_AshitaCore->GetMemoryManager()->GetInventory()->GetContainerCountMax(*iter) + 1;
        for (int y = 1; y < containerMax; y++)
        {
            Ashita::FFXI::item_t* pItem = m_AshitaCore->GetMemoryManager()->GetInventory()->GetContainerItem(*iter, y);
            if ((pItem->Id == 0) && (pItem->Count == 0))
                FreeSpace[*iter]++;
        }
    }

    int containerMax = m_AshitaCore->GetMemoryManager()->GetInventory()->GetContainerCountMax(0) + 1;
    for (int x = 1; x < containerMax; x++)
    {
        //Skip if we've sent a packet for the slot in the last 5 seconds.
        std::chrono::time_point<std::chrono::steady_clock> comparand = std::chrono::steady_clock::now() - std::chrono::seconds(5);
        if (comparand < mState.InventoryLocks[x])
            continue;

        Ashita::FFXI::item_t* pItem = m_AshitaCore->GetMemoryManager()->GetInventory()->GetContainerItem(0, x);
        if (pItem->Id == 0)
            continue;

        IItem* pResource = m_AshitaCore->GetResourceManager()->GetItemById(pItem->Id);
        if (!pResource)
            continue;

        if (std::find(mProfile.AutoStore.begin(), mProfile.AutoStore.end(), pItem->Id) != mProfile.AutoStore.end())
        {
            if (pItem->Count == pResource->StackSize)
            {
                StoreItem(FreeSpace, pItem);
                continue;
            }
        }

        else if (std::find(mProfile.AutoDrop.begin(), mProfile.AutoDrop.end(), pItem->Id) != mProfile.AutoDrop.end())
        {
            DropItem(pItem);
            continue;
        }

        if ((mSettings.AutoStack) && (pItem->Count < pResource->StackSize))
        {
            for (int y = 1; y < x; y++)
            {
                if (comparand < mState.InventoryLocks[y])
                    continue;

                Ashita::FFXI::item_t* pItem2 = m_AshitaCore->GetMemoryManager()->GetInventory()->GetContainerItem(0, y);
                if (pItem2->Id != pItem->Id)
                    continue;

                if (pItem2->Count < pResource->StackSize)
                {
                    MergeItems(pItem, pItem2);
                    break;
                }
            }
        }
    }
}