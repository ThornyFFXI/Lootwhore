#include "Lootwhore.h"
#pragma comment(lib, "psapi.lib")
#include <psapi.h>

__declspec(dllexport) IPlugin* __stdcall expCreatePlugin(const char* args)
{
    UNREFERENCED_PARAMETER(args);

    return (IPlugin*)(new Lootwhore());
}

__declspec(dllexport) double __stdcall expGetInterfaceVersion(void)
{
    return ASHITA_INTERFACE_VERSION;
}

bool Lootwhore::Initialize(IAshitaCore* core, ILogManager* logger, const uint32_t id)
{
    m_AshitaCore = core;
    m_LogManager = logger;
    m_PluginId = id;

    MODULEINFO mod = {0};
    ::GetModuleInformation(::GetCurrentProcess(), ::GetModuleHandle("FFXiMain.dll"), &mod, sizeof(MODULEINFO));
    pWardrobe  = Ashita::Memory::FindPattern((uintptr_t)mod.lpBaseOfDll, (uintptr_t)mod.SizeOfImage, "A1????????8B88B4000000C1E907F6C101E9", 1, 0);
    pZoneFlags = Ashita::Memory::FindPattern((uintptr_t)mod.lpBaseOfDll, (uintptr_t)mod.SizeOfImage, "8B8C24040100008B90????????0BD18990????????8B15????????8B82", 0, 0);

    if (pWardrobe == 0)
    {
        m_AshitaCore->GetChatManager()->Writef(ERROR_COLOR, false, "%s: Wardrobe status signature scan failed.", Ashita::Chat::Header(GetName()).c_str());
        return false;
    }

    if (pZoneFlags == 0)
    {
        m_AshitaCore->GetChatManager()->Writef(ERROR_COLOR, false, "%s: Zone flag signature scan failed.", Ashita::Chat::Header(GetName()).c_str());
        return false;
    }

    pZoneOffset = Read32(pZoneFlags, 0x09);
    if (pZoneOffset == 0)
    {
        m_AshitaCore->GetChatManager()->Writef(ERROR_COLOR, false, "%s: Zone flag offset not found.", Ashita::Chat::Header(GetName()).c_str());
        return false;
    }

    pZoneFlags = Read32(pZoneFlags, 0x17);
    if (pZoneFlags == 0)
    {
        m_AshitaCore->GetChatManager()->Writef(ERROR_COLOR, false, "%s: Zone flag sub pointer not found.", Ashita::Chat::Header(GetName()).c_str());
        return false;
    }

    pOutput      = new OutputHelpers(core, logger, this->GetName());
    pSettings    = new SettingsHelper(core, pOutput, this->GetName());
    pPacket      = new safePacketInjector(core->GetPacketManager());

    mSettings    = Settings_t();
    InitializeCommands();
    InitializeState();
    LoadDefaultSettings(true);
    LoadDefaultProfile(true);

    return true;
}

void Lootwhore::Release(void)
{
    delete pPacket;
    delete pSettings;
    delete pOutput;
}

void Lootwhore::InitializeCommands()
{
    CommandInformation build;
    build.handler          = &Lootwhore::HandleCommandProfile;
    build.help.command = "/lw profile [Required: Profile Name]";
    build.help.description = "Load a profile, containing settings to determine how items should be treated.";
    mCommandMap.insert(std::make_pair("profile", build));

    build.handler          = &Lootwhore::HandleCommandExport;
    build.help.command     = "/lw export [Optional: Profile Name]";
    build.help.description = "Saves the current settings to a profile.  If you already have a profile loaded, you can omit name to overwrite it.";
    mCommandMap.insert(std::make_pair("export", build));

    build.handler          = &Lootwhore::HandleCommandReset;
    build.help.command     = "/lw reset";
    build.help.description = "Resets all profile settings.  This does not reset configuration.";
    mCommandMap.insert(std::make_pair("reset", build));

    build.handler          = &Lootwhore::HandleCommandDefault;
    build.help.command     = "/lw default [Required: lot/pass/ignore]";
    build.help.description = "Sets the default action to be performed on items in treasure pool.";
    mCommandMap.insert(std::make_pair("default", build));

    build.handler          = &Lootwhore::HandleCommandAutoStack;
    build.help.command     = "/lw autostack [Required: on/off]";
    build.help.description = "Changes autostack setting.  If autostack is enabled, lootwhore will automatically combine stackable items in your inventory.";
    mCommandMap.insert(std::make_pair("autostack", build));

    build.handler          = &Lootwhore::HandleCommandSmartPass;
    build.help.command     = "/lw smartpass [Required: everyone/listonly/off]";
    build.help.description = "Changes smartpass setting.  If smartpass is enabled, lootwhore will pass whenever someone else lots an item.  If it is in list only mode, this only applies to people on your whitelist in your configuration.";
    mCommandMap.insert(std::make_pair("smartpass", build));

    build.handler          = &Lootwhore::HandleCommandRarePass;
    build.help.command     = "/lw rarepass [Required: on/off]";
    build.help.description = "Changes rarepass setting.  If rarepass is enabled, lootwhore will automatically pass anything rare marked that you already have.";
    mCommandMap.insert(std::make_pair("rarepass", build));

    build.handler          = &Lootwhore::HandleCommandZoneReset;
    build.help.command     = "/lw zonereset [Required: on/off]";
    build.help.description = "Changes zonereset setting.  If zonereset is enabled, your profile will be reset the next time you zone.  Good for safety when using default pass.";
    mCommandMap.insert(std::make_pair("zonereset", build));

    build.handler          = &Lootwhore::HandleCommandAdd;
    build.help.command     = "/lw add [Required: item id or name] [Required: ignore/lot/pass]";
    build.help.description = "Adds an item-specific reaction to be performed on items in treasure pool.";
    mCommandMap.insert(std::make_pair("add", build));

    build.handler          = &Lootwhore::HandleCommandRemove;
    build.help.command     = "/lw remove [Required: item id or name]";
    build.help.description = "Clears an item-specific reaction from an item.";
    mCommandMap.insert(std::make_pair("remove", build));

    build.handler          = &Lootwhore::HandleCommandAddDrop;
    build.help.command     = "/lw adddrop [Required: item id or name]";
    build.help.description = "Adds an item to drop list to be automatically dropped whenever it enters inventory.";
    mCommandMap.insert(std::make_pair("adddrop", build));

    build.handler          = &Lootwhore::HandleCommandRemoveDrop;
    build.help.command     = "/lw removedrop [Required: item id or name]";
    build.help.description = "Removes an item from drop list.";
    mCommandMap.insert(std::make_pair("removedrop", build));

    build.handler          = &Lootwhore::HandleCommandAddStore;
    build.help.command     = "/lw addstore [Required: item id or name]";
    build.help.description = "Adds an item to store list to be automatically stored whenever a full stack is in inventory.";
    mCommandMap.insert(std::make_pair("addstore", build));

    build.handler          = &Lootwhore::HandleCommandRemoveStore;
    build.help.command     = "/lw removestore [Required: item id or name]";
    build.help.description = "Removes an item from store list.";
    mCommandMap.insert(std::make_pair("removestore", build));

    build.handler          = &Lootwhore::HandleCommandList;
    build.help.command     = "/lw list [Optional: react/drop/store]";
    build.help.description = "Lists your current item settings.  If you specify a type, will only list that type.";
    mCommandMap.insert(std::make_pair("list", build));

    build.handler          = &Lootwhore::HandleCommandLot;
    build.help.command     = "/lw lot";
    build.help.description = "Lots all items you have not yet passed in current treasure pool.";
    mCommandMap.insert(std::make_pair("lot", build));

    build.handler          = &Lootwhore::HandleCommandPass;
    build.help.command     = "/lw pass";
    build.help.description = "Passes all items you have not yet lotted in current treasure pool.";
    mCommandMap.insert(std::make_pair("pass", build));

    build.handler          = &Lootwhore::HandleCommandHelp;
    build.help.command     = "/lw help [Optional: command]";
    build.help.description = "Print information on a command.  If no command is specified, a list of commands will be printed.";
    mCommandMap.insert(std::make_pair("help", build));
}

void Lootwhore::InitializeState()
{
    //Initialize to default values.
    mState.InventoryLoading = true;
    mState.MyId = 0;
    mState.MyName = "NO_NAME";
    for (int x = 0; x < 10; x++)
    {
        mState.PoolSlots[x] = TreasurePoolSlot_t();
    }
    for (int x = 0; x < 81; x++)
    {
        mState.InventoryLocks[x] = std::chrono::steady_clock::now() - std::chrono::seconds(10);
    }
    
    //Check if we're already ingame
    uint16_t myIndex = m_AshitaCore->GetMemoryManager()->GetParty()->GetMemberTargetIndex(0);
    if (myIndex < 1)
        return;
    if (m_AshitaCore->GetMemoryManager()->GetEntity()->GetRawEntity(myIndex) == 0)
        return;
    if ((m_AshitaCore->GetMemoryManager()->GetEntity()->GetRenderFlags0(myIndex) & 0x200) == 0)
        return;
    if ((m_AshitaCore->GetMemoryManager()->GetEntity()->GetRenderFlags0(myIndex) & 0x4000) != 0)
        return;

    //We're ingame, so fill in ID/Name.
    mState.MyId = m_AshitaCore->GetMemoryManager()->GetEntity()->GetServerId(myIndex);
    mState.MyName = m_AshitaCore->GetMemoryManager()->GetEntity()->GetName(myIndex);
    mState.InventoryLoading = false;

    IParty* pParty = m_AshitaCore->GetMemoryManager()->GetParty();
    for (int x = 0; x < 10; x++)
    {
        Ashita::FFXI::treasureitem_t* pItem = m_AshitaCore->GetMemoryManager()->GetInventory()->GetTreasurePoolItem(x);
        mState.PoolSlots[x]                 = TreasurePoolSlot_t(pItem);
    }
}