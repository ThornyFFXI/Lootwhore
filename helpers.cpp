#include "Lootwhore.h"

const char* gContainers[17] =
    {
        "Inventory",
        "Safe",
        "Storage",
        "Temporary",
        "Locker",
        "Satchel",
        "Sack",
        "Case",
        "Wardrobe",
        "Safe2",
        "Wardrobe2",
        "Wardrobe3",
        "Wardrobe4",
        "Wardrobe5",
        "Wardrobe6",
        "Wardrobe7",
        "Wardrobe8"};

std::list<int> gEquipBags =
    {
        8, 10, 11, 12, 13, 14, 15, 16};

std::list<int> gFurnitureTypes =
    {
        10, 11, 12, 14};

bool Lootwhore::CheckRarePass(uint16_t ItemId)
{
    //Check if we can pull a resource and the item is rare flagged.
    IItem* pResource = m_AshitaCore->GetResourceManager()->GetItemById(ItemId);
    if ((!pResource) || ((pResource->Flags & 0x8000) == NULL))
        return false;

    //Check if we have the item
    for (int x = 0; x < 17; x++)
    {
        if (x == 3)
            continue;
        int containerMax = m_AshitaCore->GetMemoryManager()->GetInventory()->GetContainerCountMax(x);
        for (int y = 0; y <= containerMax; y++)
        {
            if (y == 81)
                break;
            if (m_AshitaCore->GetMemoryManager()->GetInventory()->GetContainerItem(x, y)->Id == ItemId)
                return true;
        }
    }
    return false;
}
bool Lootwhore::IsPositiveInteger(std::string text)
{
    for (int x = 0; x < text.size(); x++)
    {
        if (!isdigit(text[x]))
            return false;
    }
    return true;
}
void Lootwhore::LotItem(int Slot)
{
    char packet[8]    = {0};
    Write8(packet, 4) = Slot;
    pPacket->addOutgoingPacket_s(0x41, 8, packet);
    mState.PoolSlots[Slot].Lockout = std::chrono::steady_clock::now() + std::chrono::milliseconds(5000);
    mState.PoolSlots[Slot].PacketAttempts++;
}
void Lootwhore::PassItem(int Slot)
{
    char packet[8]    = {0};
    Write8(packet, 4) = Slot;
    pPacket->addOutgoingPacket_s(0x42, 8, packet);
    mState.PoolSlots[Slot].Lockout = std::chrono::steady_clock::now() + std::chrono::milliseconds(5000);
    mState.PoolSlots[Slot].PacketAttempts++;
}

void Lootwhore::DropItem(Ashita::FFXI::item_t* item)
{
    IItem* resource = m_AshitaCore->GetResourceManager()->GetItemById(item->Id);
    pk_DropItem packet;
    packet.Index    = item->Index;
    packet.Quantity = item->Count;
    pPacket->addOutgoingPacket_s(0x28, 12, &packet);
    mState.InventoryLocks[item->Index] = std::chrono::steady_clock::now();
}
void Lootwhore::StoreItem(int* FreeSpace, Ashita::FFXI::item_t* item)
{
    IItem* resource = m_AshitaCore->GetResourceManager()->GetItemById(item->Id);

    for (std::list<int>::iterator iter = mSettings.StoreBags.begin(); iter != mSettings.StoreBags.end(); iter++)
    {
        //Skip storage if item is furniture.
        if ((std::find(gFurnitureTypes.begin(), gFurnitureTypes.end(), resource->Type) != gFurnitureTypes.end()) && (*iter == 2))
            continue;

        //Skip wardrobes if item being stored isn't equippable.
        if ((std::find(gEquipBags.begin(), gEquipBags.end(), *iter) != gEquipBags.end()) && (!(item->Flags & 0x800)))
            continue;

        if (FreeSpace[*iter] > 0)
        {
            FreeSpace[*iter]--;
            pk_MoveItem packet;
            packet.FromIndex = item->Index;
            packet.ToStorage = *iter;
            packet.Quantity  = item->Count;
            pPacket->addOutgoingPacket_s(0x29, sizeof(pk_MoveItem), &packet);
            if (packet.Quantity == 1)
                pOutput->message_f("Storing a $H%s$R in $H%s$R.", resource->LogNameSingular[0], gContainers[packet.ToStorage]);
            else
                pOutput->message_f("Storing %d $H%s$R in $H%s$R.", packet.Quantity, resource->LogNamePlural[0], gContainers[packet.ToStorage]);
            mState.InventoryLocks[item->Index] = std::chrono::steady_clock::now();
            return;
        }
    }
}
void Lootwhore::MergeItems(Ashita::FFXI::item_t* source, Ashita::FFXI::item_t* destination)
{
    IItem* resource = m_AshitaCore->GetResourceManager()->GetItemById(destination->Id);

    pk_MoveItem packet;
    packet.FromIndex = source->Index;
    packet.ToIndex   = destination->Index;
    packet.Quantity  = min(resource->StackSize - destination->Count, source->Count);
    pPacket->addOutgoingPacket_s(0x29, sizeof(pk_MoveItem), &packet);
    if (packet.Quantity == 1)
        pOutput->message_f("Moving a $H%s$R into an existing partial stack.", resource->LogNameSingular[0]);
    else
        pOutput->message_f("Moving %d $H%s$R into an existing partial stack.", packet.Quantity, resource->LogNamePlural[0]);
    mState.InventoryLocks[source->Index]      = std::chrono::steady_clock::now();
    mState.InventoryLocks[destination->Index] = std::chrono::steady_clock::now();
}

void Lootwhore::checkBags()
{
    bool atNomad = false;
    bool inMog   = false;

    DWORD zonepointer = Read32(pZoneFlags, 0);
    if (zonepointer != 0)
    {
        if (Read32(zonepointer, pZoneOffset) & 0x100)
        {
            inMog = true;
        }
    }

    for (int x = 0; x < 1024; x++)
    {
        if ((m_AshitaCore->GetMemoryManager()->GetEntity()->GetRawEntity(x)) && (m_AshitaCore->GetMemoryManager()->GetEntity()->GetRenderFlags0(x) & 0x200))
        {
            if ((m_AshitaCore->GetMemoryManager()->GetEntity()->GetDistance(x) < 36) && (strcmp(m_AshitaCore->GetMemoryManager()->GetEntity()->GetName(x), "Nomad Moogle") == 0))
            {
                atNomad = true;
            }
        }
    }

    DWORD Memloc  = Read32(pWardrobe, 0);
    Memloc        = Read32(Memloc, 0);
    uint8_t flags = Read8(Memloc, 0xB4);

    mState.hasContainer[0]  = true;                                                 //Always have inventory.
    mState.hasContainer[1]  = (atNomad || inMog);                                   //Safe
    mState.hasContainer[2]  = (inMog || (atNomad && mSettings.EnableNomadStorage)); //Storage
    mState.hasContainer[3]  = false;                                                //Never have temp items.
    mState.hasContainer[4]  = (atNomad || inMog);                                   //Locker
    mState.hasContainer[5]  = ((Read8(Memloc, 0xB4) & 0x01) != 0);                  //Satchel
    mState.hasContainer[6]  = true;                                                 //Sack
    mState.hasContainer[7]  = true;                                                 //Case
    mState.hasContainer[8]  = true;                                                 //Wardrobe
    mState.hasContainer[9]  = (atNomad || inMog);                                   //Safe2
    mState.hasContainer[10] = true;                                                 //Wardrobe2
    mState.hasContainer[11]              = ((flags & 0x04) != 0);                                //Wardrobe3
    mState.hasContainer[12]              = ((flags & 0x08) != 0);                                //Wardrobe4
    mState.hasContainer[13]              = ((flags & 0x10) != 0);                                //Wardrobe5
    mState.hasContainer[14]              = ((flags & 0x20) != 0);                                //Wardrobe6
    mState.hasContainer[15]              = ((flags & 0x40) != 0);                                //Wardrobe7
    mState.hasContainer[16]              = ((flags & 0x80) != 0);                                //Wardrobe8

    for (std::list<int>::iterator iter = mSettings.ForceEnableBags.begin(); iter != mSettings.ForceEnableBags.end(); iter++)
    {
        mState.hasContainer[*iter] = true;
    }
}