#ifndef __ASHITA_Lootwhore_Structs_H_INCLUDED__
#define __ASHITA_Lootwhore_Structs_H_INCLUDED__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <chrono>
#include <list>
#include <map>
#include <stdint.h>
#include <string>

#define RBUFP(p,pos) (((uint8_t*)(p)) + (pos))
#define Read8(p,pos) (*(uint8_t*)RBUFP((p),(pos)))
#define Read16(p,pos) (*(uint16_t*)RBUFP((p),(pos)))
#define Read32(p,pos) (*(uint32_t*)RBUFP((p),(pos)))
#define Read64(p,pos) (*(uint64_t*)RBUFP((p),(pos)))
#define ReadFloat(p,pos) (*(float_t*)RBUFP((p),(pos)))

#define WBUFP(p,pos) (((uint8_t*)(p)) + (pos))
#define Write8(p,pos) (*(uint8_t*)WBUFP((p),(pos)))
#define Write16(p,pos) (*(uint16_t*)WBUFP((p),(pos)))
#define Write32(p,pos) (*(uint32_t*)WBUFP((p),(pos)))
#define Write64(p,pos) (*(uint64_t*)WBUFP((p),(pos)))
#define WriteFloat(p,pos) (*(float_t*)WBUFP((p),(pos)))

#define CheckArg(a,b) (argcount > a) && (_stricmp(args[a].c_str(), b) == 0)

enum class LotState
{
    Untouched,
    Lotted,
    Passed
};

enum class SmartPassSetting
{
    Disabled,
    ListOnly,
    Everyone
};

enum class LotReaction
{
    Ignore,
    Lot,
    Pass,
    Unknown
};

struct pk_DropItem
{
    uint32_t Header;
    uint32_t Quantity;
    uint8_t Location;
    uint8_t Index;
    uint16_t Unknown;

    pk_DropItem()
        : Header(0)
        , Quantity(0)
        , Location(0)
        , Index(0)
        , Unknown(0)
    {}
};

struct pk_MoveItem
{
    uint32_t Header;
    uint32_t Quantity;
    uint8_t FromStorage;
    uint8_t ToStorage;
    uint8_t FromIndex;
    uint8_t ToIndex;

    pk_MoveItem()
        : Header(0)
        , Quantity(0)
        , FromStorage(0)
        , ToStorage(0)
        , FromIndex(0)
        , ToIndex(81)
    {}
};

struct LotInfo_t
{
    uint16_t Lot;
    uint32_t Id;
    std::string Name;

    LotInfo_t(uint16_t Lot, uint32_t Id, std::string Name)
        : Lot(Lot)
        , Id(Id)
        , Name(Name)
    {}
};
struct TreasurePoolSlot_t
{
    uint16_t Id;
    LotState Status;
    std::list<LotInfo_t> LotList;
    uint16_t PacketAttempts;
    std::chrono::steady_clock::time_point LastAction;

    TreasurePoolSlot_t()
        : Id(0)
        , Status(LotState::Untouched)
        , LotList(std::list<LotInfo_t>())
        , PacketAttempts(0)
        , LastAction(std::chrono::steady_clock::now() - std::chrono::minutes(5))
    {}

    TreasurePoolSlot_t(Ashita::FFXI::treasureitem_t* pItem)
    {
        Id = pItem->ItemId;
        LotList = std::list<LotInfo_t>();
        if (pItem->Lot == 0)
            Status = LotState::Untouched;
        else if (pItem->Lot == 65535)
            Status = LotState::Passed;
        else
            Status = LotState::Lotted;
        PacketAttempts = 0;
        LastAction     = (std::chrono::steady_clock::now() - std::chrono::minutes(5));
    }

    TreasurePoolSlot_t(uint16_t Id)
        : Id(Id)
        , Status(LotState::Untouched)
        , LotList(std::list<LotInfo_t>())
        , PacketAttempts(0)
        , LastAction(std::chrono::steady_clock::now() - std::chrono::minutes(5))
    {}
};

struct State_t
{
    std::chrono::time_point<std::chrono::steady_clock> InventoryLocks[81];
    TreasurePoolSlot_t PoolSlots[10];
    std::string MyName;
    uint32_t MyId;
    std::string CurrentProfile;
    bool hasContainer[13];
};

struct Profile_t
{
    SmartPassSetting SmartPass;
    bool RarePass;
    bool ResetOnZone;
    LotReaction DefaultReaction;
    std::map<uint16_t, LotReaction> ItemMap;
    std::list<uint16_t> AutoDrop;
    std::list<uint16_t> AutoStore;

    Profile_t()
        : SmartPass(SmartPassSetting::Disabled)
        , RarePass(false)
        , ResetOnZone(false)
        , DefaultReaction(LotReaction::Ignore)
        , ItemMap(std::map<uint16_t, LotReaction>())
        , AutoDrop(std::list<uint16_t>())
        , AutoStore(std::list<uint16_t>())
    {}
};

struct Settings_t
{
    int MaxRetry;
    bool EnableNomadStorage;
    bool AutoStack;
    std::list<int> ForceEnableBags;
    std::list<int> StoreBags;
    std::list<string> WhiteList;

    Settings_t()
        : MaxRetry(3)
        , EnableNomadStorage(false)
        , AutoStack(true)
        , ForceEnableBags(std::list<int>())
        , StoreBags(std::list<int>{5, 6, 7, 8, 10, 11, 12})
        , WhiteList(std::list<string>())
    {}
};


struct cistringcmp
{
    bool operator()(const string& lhs, const string& rhs) const
    {
        return (_stricmp(lhs.c_str(), rhs.c_str()) < 0);
    }
};
struct CommandHelp
{
    string command;
    string description;
};
#endif