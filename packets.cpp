#include "Lootwhore.h"

bool Lootwhore::HandleIncomingPacket(uint16_t id, uint32_t size, const uint8_t* data, uint8_t* modified, uint32_t sizeChunk, const uint8_t* dataChunk, bool injected, bool blocked)
{
    UNREFERENCED_PARAMETER(size);
    UNREFERENCED_PARAMETER(data);
    UNREFERENCED_PARAMETER(sizeChunk);
    UNREFERENCED_PARAMETER(dataChunk);
    UNREFERENCED_PARAMETER(injected);
    UNREFERENCED_PARAMETER(blocked);
    
    
    if (id == 0x0B)
        mState.InventoryLoading = true;
    if (id == 0x01D)
        mState.InventoryLoading = false;

    if (id == 0x0A)
    {
        HandleIncomingPacket0x0A(modified);
    }

    if (id == 0xD2)
    {
        HandleIncomingPacket0xD2(modified);
    }

    if (id == 0xD3)
    {
        HandleIncomingPacket0xD3(modified);
    }

    return false;
}

bool Lootwhore::HandleOutgoingPacket(uint16_t id, uint32_t size, const uint8_t* data, uint8_t* modified, uint32_t sizeChunk, const uint8_t* dataChunk, bool injected, bool blocked)
{
    UNREFERENCED_PARAMETER(modified);
    UNREFERENCED_PARAMETER(sizeChunk);
    UNREFERENCED_PARAMETER(dataChunk);
    UNREFERENCED_PARAMETER(injected);
    UNREFERENCED_PARAMETER(blocked);

    if (pPacket->checkOutgoingSelfInjected(id, size, data))
        return false;

    if (id == 0x15)
    {
        HandleOutgoingPacket0x15();
    }

    if (id == 0x41)
    {
        uint8_t poolIndex                  = Read8(modified, 0x04);
        mState.PoolSlots[poolIndex].Status = LotState::Lotted;
    }

    if (id == 0x42)
    {
        uint8_t poolIndex                  = Read8(modified, 0x04);
        mState.PoolSlots[poolIndex].Status = LotState::Passed;
    }

    return false;
}

void Lootwhore::HandleIncomingPacket0x0A(uint8_t* modified)
{
    //Get our ID/Name.  If name changed, load default settings(initial login after autoload, or char change).
    mState.MyId   = Read32(modified, 0x04);
    std::string Name = std::string((const char*)modified + 0x84);
    if (Name != mState.MyName)
    {
        mState.MyName = Name;
        LoadDefaultSettings(false);
        LoadDefaultProfile(false);
    }
    
    if (mProfile.ResetOnZone)
    {
        mProfile = Profile_t();
    }

    //Zoning, reset lots.
    for (int x = 0; x < 10; x++)
    {
        mState.PoolSlots[x]        = TreasurePoolSlot_t();
    }
}

void Lootwhore::HandleIncomingPacket0xD2(uint8_t* modified)
{
    //Handle item entering treasure pool by updating our state.
    uint8_t poolIndex = Read8(modified, 0x14);
    uint16_t itemId   = Read16(modified, 0x10);

    int delay = -1;
    if (mSettings.RandomDelayMax != 0)
    {
        delay = mRandomDistribution(mRandomEngine);
    }

    mState.PoolSlots[poolIndex]                         = TreasurePoolSlot_t(itemId, delay);
}

void Lootwhore::HandleIncomingPacket0xD3(uint8_t* modified)
{
    //Handle item lot update.
    uint8_t poolIndex = Read8(modified, 0x14);

    //If this isn't 0, the item is sorting or being floored.
    if (Read8(modified, 0x15) != 0)
    {
        mState.PoolSlots[poolIndex] = TreasurePoolSlot_t();
        return;
    }

    //If this packet was in regards to us, flag the item.
    if (Read32(modified, 0x08) == mState.MyId)
    {
        if (Read16(modified, 0x12) == 0xFFFF)
            mState.PoolSlots[poolIndex].Status = LotState::Passed;
        else
            mState.PoolSlots[poolIndex].Status = LotState::Lotted;
    }
}

void Lootwhore::HandleOutgoingPacket0x15()
{
    //We handle our packet injection on outgoing 0x15 because it gives us the most recent data possible before injection.

    //Address any potential lots.
    for (int x = 0; x < 10; x++)
    {
        HandleTreasureSlot(x);
    }

    //Address item drop/stores.
    if (!mState.InventoryLoading)
    {
        HandleInventory();
    }
}