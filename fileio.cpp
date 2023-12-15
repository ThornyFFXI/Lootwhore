#include "Lootwhore.h"
#include "..\common\thirdparty\rapidxml.hpp"
using namespace rapidxml;

void Lootwhore::LoadDefaultSettings(bool forceReload)
{
    //Reset settings.
    mSettings = Settings_t();

    std::string Path = pSettings->GetCharacterSettingsPath(mState.MyName.c_str());
    if ((Path == pSettings->GetLoadedXmlPath()) && (!forceReload))
        return;

    if (Path == "FILE_NOT_FOUND")
    {
        Path = pSettings->GetDefaultSettingsPath();
        SaveSettings("default.xml");
    }
    else
    {
        LoadSettings(Path.c_str());
    }

    uint32_t seed = static_cast<uint32_t>(std::chrono::system_clock::now().time_since_epoch().count() + GetCurrentProcessId());
    if (mSettings.RandomDelayMax != 0)
    {
        int minDelay = max(0, mSettings.RandomDelayMin);
        int maxDelay = max(minDelay + 1, mSettings.RandomDelayMax);
        mRandomEngine       = std::default_random_engine(seed);
        mRandomDistribution = std::uniform_int_distribution<int32_t>(minDelay, maxDelay);
    }
}
void Lootwhore::LoadSettings(const char* Name)
{
    std::string SettingsFile = pSettings->GetInputSettingsPath(Name);
    if (SettingsFile == "FILE_NOT_FOUND")
    {
        pOutput->error_f("Could not find settings file.  Loading defaults.  [$H%s$R]", Name);
        LoadDefaultSettings(true);
        return;
    }

    //Reset settings.
    mSettings                 = Settings_t();
    xml_document<>* XMLReader = pSettings->LoadSettingsXml(SettingsFile);
    if (XMLReader == NULL)
    {
        pOutput->error_f("Could not load settings file.  Resetting to defaults.  [$H%s$R]", SettingsFile.c_str());
        return;
    }

    //Make sure XML has a lootwhore node.
    xml_node<>* Node = XMLReader->first_node("lwconfig");
    if (!Node)
    {
        pSettings->UnloadSettings();
        pOutput->error_f("Settings file did not have a lwconfig node at root level.  Resetting to defaults.  [$H%s$R]", SettingsFile.c_str());
        return;
    }

    //Parse settings.
    for (Node = Node->first_node(); Node; Node = Node->next_sibling())
    {
        if (_stricmp(Node->name(), "settings") == 0)
        {
            for (xml_node<>* SubNode = Node->first_node(); SubNode; SubNode = SubNode->next_sibling())
            {
                if (_stricmp(SubNode->name(), "maxretry") == 0)
                {
                    mSettings.MaxRetry = max(1, min(10, atoi(SubNode->value())));
                }
                else if (_stricmp(SubNode->name(), "storebags") == 0)
                {
                    mSettings.StoreBags.clear();
                    stringstream stream(SubNode->value());
                    while (stream.good())
                    {
                        string bag;
                        getline(stream, bag, ',');
                        if (bag.length() < 1)
                            break;
                        int bagIndex = atoi(bag.c_str());
                        if ((bagIndex < 1) || (bagIndex > 12))
                            continue;
                        mSettings.StoreBags.push_back(bagIndex);
                    }                
                }
                else if (_stricmp(SubNode->name(), "forceenablebags") == 0)
                {
                    mSettings.ForceEnableBags.clear();
                    stringstream stream(SubNode->value());
                    while (stream.good())
                    {
                        string bag;
                        getline(stream, bag, ',');
                        if (bag.length() < 1)
                            break;
                        int bagIndex = atoi(bag.c_str());
                        if ((bagIndex < 0) || (bagIndex > 12))
                            continue;
                        mSettings.ForceEnableBags.push_back(bagIndex);
                    }
                }
                else if (_stricmp(SubNode->name(), "nomadstorage") == 0)
                {
                    if (_stricmp(SubNode->value(), "enabled") == 0)
                        mSettings.EnableNomadStorage = true;
                }
                else if (_stricmp(SubNode->name(), "autostack") == 0)
                {
                    if (_stricmp(SubNode->value(), "disabled") == 0)
                        mSettings.AutoStack = false;
                }
                else if (_stricmp(SubNode->name(), "retrydelay") == 0)
                {
                    mSettings.RetryDelay = atoi(SubNode->value());
                }
                else if (_stricmp(SubNode->name(), "delaymin") == 0)
                {
                    mSettings.RandomDelayMin = atoi(SubNode->value());
                }
                else if (_stricmp(SubNode->name(), "delaymax") == 0)
                {
                    mSettings.RandomDelayMax = atoi(SubNode->value());
                }
                else if (_stricmp(SubNode->name(), "silentstack") == 0)
                {
                    if (_stricmp(SubNode->value(), "enabled") == 0)
                        mSettings.SilentStack = true;
                }
            }        
        }
        else if (_stricmp(Node->name(), "whitelist") == 0)
        {
            for (xml_node<>* SubNode = Node->first_node(); SubNode; SubNode = SubNode->next_sibling())
            {
                if (_stricmp(SubNode->name(), "entry") == 0)
                {
                    mSettings.WhiteList.push_back(FormatName(SubNode->value()));
                }
            }
        }
    }
}
void Lootwhore::SaveSettings(const char* Name)
{
    std::string Path = pSettings->GetInputWritePath(Name);
    pSettings->CreateDirectories(Path.c_str());

    ofstream outstream(Path.c_str());
    if (!outstream.is_open())
    {
        pOutput->error_f("Failed to write settings file.  [%s]", Path.c_str());
        return;
    }

    outstream << "<lwconfig>\n";
    outstream << "\n\t<settings>\n";
    outstream << "\t\t<storebags>";
    for (std::list<int>::iterator iter = mSettings.StoreBags.begin(); iter != mSettings.StoreBags.end(); iter++)
    {
        if (iter != mSettings.StoreBags.begin())
            outstream << ",";
        outstream << *iter;
    }
    outstream << "</storebags>\n";
    outstream << "\t\t<autostack>"  << (mSettings.AutoStack ? "enabled" : "disabled") << "</autostack>\n";
    outstream << "\t\t<forceenablebags></forceenablebags>\n";
    outstream << "\t\t<nomadstorage>" << (mSettings.EnableNomadStorage ? "enabled" : "disabled") << "</nomadstorage>\n";
    outstream << "\t\t<maxretry>" << mSettings.MaxRetry << "</maxretry> <!--Maximum amount of times to try lotting or passing an item if server doesn't respond to indicate packet was received. -->\n";
    outstream << "\t\t<retrydelay>" << mSettings.RetryDelay << "</retrydelay> <!--Time, in milliseconds, to wait before retrying a pass or lot. -->\n";
    outstream << "\t\t<delaymin>" << mSettings.RandomDelayMin << "</delaymin> <!--Minimum time, in milliseconds, to wait before lotting a freshly dropped item. -->\n";
    outstream << "\t\t<delaymax>" << mSettings.RandomDelayMax << "</delaymax> <!--Maximum time, in milliseconds, to wait before lotting a freshly dropped item.  Set to 0 for instant lots. -->\n";
    outstream << "\t\t<silentstack>" << (mSettings.SilentStack ? "enabled" : "disabled") << "</silentstack>\n";
    outstream << "\t</settings>\n\n";
    outstream << "\t<whitelist> <!--Anyone listed here will trigger smartpass when in 'listonly' mode. -->\n";
    for (std::list<string>::iterator iter = mSettings.WhiteList.begin(); iter != mSettings.WhiteList.end(); iter++)
    {
        outstream << "\t\t<entry>" << *iter << "</entry>\n";
    }
    outstream << "\t</whitelist>\n\n";    
    outstream << "</lwconfig>";
    outstream.close();
    pOutput->message_f("Wrote settings XML. [$H%s$R]", Path.c_str());
}

void Lootwhore::LoadDefaultProfile(bool forceReload)
{
    bool playerFile = true;
    char buffer[1024];
    sprintf_s(buffer, 1024, "%sconfig\\%s\\profiles\\%s.xml", m_AshitaCore->GetInstallPath(), this->GetName(), mState.MyName.c_str());
    if (!std::filesystem::exists(buffer))
    {
        sprintf_s(buffer, 1024, "%sconfig\\%s\\profiles\\default.xml", m_AshitaCore->GetInstallPath(), this->GetName());
        playerFile = false;
    }

    if ((std::string(buffer) == mState.CurrentProfile) && (!forceReload))
        return;

    if (!std::filesystem::exists(buffer))
    {
        mProfile = Profile_t();
        //Save a default file so we have it next time.
        SaveProfile("default.xml", true);
    }
    else
    {
        if (playerFile)
        {
            LoadProfile((mState.MyName + ".xml").c_str());        
        }
        else
        {
            LoadProfile("default.xml");
        }
    }
}
void Lootwhore::ImportProfile(const char* Name)
{
    char buffer[1024];
    sprintf_s(buffer, 1024, "profiles\\%s", Name);
    std::string ProfilePath = pSettings->GetInputSettingsPath(buffer);

    if (ProfilePath == "FILE_NOT_FOUND")
    {
        pOutput->error_f("Could not find profile.  Loading defaults.  [$H%s$R]", Name);
        LoadDefaultProfile(true);
        return;
    }

    //Reset settings.
    mProfile = Profile_t();

    //Load profile.
    char* bigbuffer           = NULL;
    xml_document<>* XMLReader = pSettings->LoadXml(ProfilePath, bigbuffer);
    if (XMLReader == NULL)
    {
        pOutput->error_f("Could not import profile.  Resetting to defaults.  [$H%s$R]", ProfilePath.c_str());
        return;
    }

    bool foundBase = false;
    for (xml_node<>* baseNode = XMLReader->first_node(); baseNode; baseNode = baseNode->next_sibling())
    {
        if (_stricmp(baseNode->name(), "lootwhore") == 0)
        {
            foundBase = true;
            for (xml_node<>* Node = baseNode->first_node(); Node; Node = Node->next_sibling())
            {
                if (_stricmp(Node->name(), "settings") == 0)
                {
                    for (xml_node<>* SubNode = Node->first_node(); SubNode; SubNode = SubNode->next_sibling())
                    {
                        if (_stricmp(SubNode->name(), "smartpass") == 0)
                        {
                            if (_stricmp(SubNode->value(), "true") == 0)
                                mProfile.SmartPass = SmartPassSetting::Everyone;
                            else
                                mProfile.SmartPass = SmartPassSetting::Disabled;
                        }
                        if (_stricmp(SubNode->name(), "rarepass") == 0)
                        {
                            mProfile.RarePass = (_stricmp(SubNode->value(), "true") == 0);
                        }
                        if (_stricmp(SubNode->name(), "zonereset") == 0)
                        {
                            mProfile.ResetOnZone = (_stricmp(SubNode->value(), "true") == 0);
                        }
                        if (_stricmp(SubNode->name(), "defaultaction") == 0)
                        {
                            if (_stricmp(SubNode->value(), "lot") == 0)
                                mProfile.DefaultReaction = LotReaction::Lot;
                            else if (_stricmp(SubNode->value(), "pass") == 0)
                                mProfile.DefaultReaction = LotReaction::Pass;
                        }
                    }
                }
                else if (_stricmp(Node->name(), "itemlist") == 0)
                {
                    for (xml_node<>* SubNode = Node->first_node(); SubNode; SubNode = SubNode->next_sibling())
                    {
                        if (_stricmp(SubNode->name(), "item") == 0)
                        {
                            IItem* pResource        = nullptr;
                            xml_attribute<>* idAttr = SubNode->first_attribute("id");
                            if (idAttr)
                            {
                                pResource = m_AshitaCore->GetResourceManager()->GetItemById(atoi(idAttr->value()));
                            }

                            if (!pResource)
                            {
                                xml_attribute<>* pName = SubNode->first_attribute("name");
                                if (pName)
                                {
                                    pResource = m_AshitaCore->GetResourceManager()->GetItemByName(pName->value(), 0);
                                }
                            }

                            if (pResource == nullptr)
                                continue;

                            auto reaction = LotReaction::Ignore;

                            if (_stricmp(SubNode->value(), "lot") == 0)
                                reaction = LotReaction::Lot;
                            else if (_stricmp(SubNode->value(), "pass") == 0)
                                reaction = LotReaction::Pass;
                            else if (_stricmp(SubNode->value(), "store") == 0)
                            {
                                reaction = LotReaction::Lot;
                                mProfile.AutoStore.push_back(pResource->Id);
                            }
                            else if (_stricmp(SubNode->value(), "drop") == 0)
                            {
                                reaction = LotReaction::Pass;
                                mProfile.AutoDrop.push_back(pResource->Id);
                            }
                            
                            mProfile.ItemMap[pResource->Id] = reaction;
                        }
                    }
                }
            }
        }
    }

    if (!foundBase)
    {
        mState.CurrentProfile = "NO_FILE";
        delete XMLReader;
        delete[] bigbuffer;
        pOutput->error_f("Profile did not have a lootwhore node at root level.  Resetting to defaults.  [$H%s$R]", ProfilePath.c_str());
        return;
    }

    mState.CurrentProfile = ProfilePath;
    pOutput->message_f("Imported profile. [$H%s$R]", ProfilePath.c_str());
    SaveProfile(ProfilePath.c_str(), false);
    delete XMLReader;
    delete bigbuffer;
}
void Lootwhore::LoadProfile(const char* Profile)
{
    char buffer[1024];
    sprintf_s(buffer, 1024, "profiles\\%s", Profile);
    std::string ProfilePath = pSettings->GetInputSettingsPath(buffer);

    if (ProfilePath == "FILE_NOT_FOUND")
    {
        pOutput->error_f("Could not find profile.  Loading defaults.  [$H%s$R]", Profile);
        LoadDefaultProfile(true);
        return;
    }

    //Reset settings.
    mProfile                  = Profile_t();

    //Load profile.
    char* bigbuffer           = NULL;
    xml_document<>* XMLReader = pSettings->LoadXml(ProfilePath, bigbuffer);
    if (XMLReader == NULL)
    {
        pOutput->error_f("Could not load profile.  Resetting to defaults.  [$H%s$R]", ProfilePath.c_str());
        return;
    }

    xml_node<>* Node = XMLReader->first_node("lootwhore");
    if (!Node)
    {
        mState.CurrentProfile = "NO_FILE";
        delete XMLReader;
        delete[] bigbuffer;
        pOutput->error_f("Profile did not have a lootwhore node at root level.  Resetting to defaults.  [$H%s$R]", ProfilePath.c_str());
        return;
    }

    for (Node = Node->first_node(); Node; Node = Node->next_sibling())
    {
        if (_stricmp(Node->name(), "settings") == 0)
        {
            for (xml_node<>* SubNode = Node->first_node(); SubNode; SubNode = SubNode->next_sibling())
            {
                if (_stricmp(SubNode->name(), "smartpass") == 0)
                {
                    if ((_stricmp(SubNode->value(), "true") == 0) || (_stricmp(SubNode->value(), "everyone") == 0))
                        mProfile.SmartPass = SmartPassSetting::Everyone;
                    else if (_stricmp(SubNode->value(), "listonly") == 0)
                        mProfile.SmartPass = SmartPassSetting::ListOnly;
                }
                if (_stricmp(SubNode->name(), "rarepass") == 0)
                {
                    if ((_stricmp(SubNode->value(), "true") == 0) || (_stricmp(SubNode->value(), "enabled") == 0))
                        mProfile.RarePass = true;
                }
                if (_stricmp(SubNode->name(), "zonereset") == 0)
                {
                    if ((_stricmp(SubNode->value(), "true") == 0) || (_stricmp(SubNode->value(), "enabled") == 0))
                        mProfile.ResetOnZone = true;
                }
                if (_stricmp(SubNode->name(), "defaultaction") == 0)
                {
                    if (_stricmp(SubNode->value(), "lot") == 0)
                        mProfile.DefaultReaction = LotReaction::Lot;
                    else if (_stricmp(SubNode->value(), "pass") == 0)
                        mProfile.DefaultReaction = LotReaction::Pass;
                }
            }
        }
        else if (_stricmp(Node->name(), "itemlist") == 0)
        {
            for (xml_node<>* SubNode = Node->first_node(); SubNode; SubNode = SubNode->next_sibling())
            {
                if (_stricmp(SubNode->name(), "item") == 0)
                {
                    xml_attribute<>* idAttr = SubNode->first_attribute("id");
                    if (idAttr == NULL)
                        continue;
                    auto id = atoi(idAttr->value());
                    if ((id < 0) || (id > 65534)) continue;

                    if (_stricmp(SubNode->value(), "lot") == 0)
                        mProfile.ItemMap[(uint16_t)id] = LotReaction::Lot;
                    else if (_stricmp(SubNode->value(), "pass") == 0)
                        mProfile.ItemMap[(uint16_t)id] = LotReaction::Pass;
                    else
                        mProfile.ItemMap[(uint16_t)id] = LotReaction::Ignore;
                }
            }
        }

        else if (_stricmp(Node->name(), "droplist") == 0)
        {
            for (xml_node<>* SubNode = Node->first_node(); SubNode; SubNode = SubNode->next_sibling())
            {
                if (_stricmp(SubNode->name(), "item") == 0)
                {
                    xml_attribute<>* idAttr = SubNode->first_attribute("id");
                    if (idAttr == NULL)
                        continue;
                    auto id = atoi(idAttr->value());
                    if ((id < 0) || (id > 65534))
                        continue;
                    mProfile.AutoDrop.push_back((uint16_t)id);
                }
            }
        }

        else if (_stricmp(Node->name(), "storelist") == 0)
        {
            for (xml_node<>* SubNode = Node->first_node(); SubNode; SubNode = SubNode->next_sibling())
            {
                if (_stricmp(SubNode->name(), "item") == 0)
                {
                    xml_attribute<>* idAttr = SubNode->first_attribute("id");
                    if (idAttr == NULL)
                        continue;
                    auto id = atoi(idAttr->value());
                    if ((id < 0) || (id > 65534))
                        continue;

                    mProfile.AutoStore.push_back((uint16_t)id);
                }
            }
        }
    }

    mState.CurrentProfile = ProfilePath;
    pOutput->message_f("Loaded profile. [$H%s$R]", ProfilePath.c_str());
    delete XMLReader;
    delete bigbuffer;
}
void Lootwhore::SaveProfile(const char* Profile, bool AppendPath)
{
    std::string ProfilePath(Profile);
    if (AppendPath)
    {
        char buffer[1024];
        sprintf_s(buffer, 1024, "profiles\\%s", Profile);
        ProfilePath = pSettings->GetInputWritePath(buffer);
    }

    ofstream outstream(ProfilePath.c_str());
    if (!outstream.is_open())
    {
        pOutput->error_f("Failed to write profile file.  [%s]", ProfilePath.c_str());
        return;
    }

    outstream << "<lootwhore>\n";
    outstream << "\n\t<settings>\n";
    outstream << "\t\t<defaultaction>";
    if (mProfile.DefaultReaction == LotReaction::Lot)
        outstream << "lot";
    else if (mProfile.DefaultReaction == LotReaction::Pass)
        outstream << "pass";
    else
        outstream << "ignore";
    outstream << "</defaultaction>\n";
    outstream << "\t\t<smartpass>";
    if (mProfile.SmartPass == SmartPassSetting::Disabled)
        outstream << "disabled";
    else if (mProfile.SmartPass == SmartPassSetting::ListOnly)
        outstream << "listonly";
    else
        outstream << "everyone";
    outstream << "</smartpass>\n";
    outstream << "\t\t<rarepass>" << (mProfile.RarePass ? "enabled" : "disabled") << "</rarepass>\n";
    outstream << "\t\t<zonereset>" << (mProfile.ResetOnZone ? "enabled" : "disabled") << "</zonereset>\n";
    outstream << "\t</settings>\n\n";

    outstream << "\t<itemlist>\n";
    for (std::map<uint16_t, LotReaction>::iterator iter = mProfile.ItemMap.begin(); iter != mProfile.ItemMap.end(); iter++)
    {
        outstream << "\t\t<item id=\"" << iter->first << "\">";
        if (iter->second == LotReaction::Lot)
            outstream << "lot";
        else if (iter->second == LotReaction::Pass)
            outstream << "pass";
        else
            outstream << "ignore";
        outstream << "</item> <!--" << m_AshitaCore->GetResourceManager()->GetItemById(iter->first)->Name[0] << "-->\n";
    }
    outstream << "\t</itemlist>\n\n";

    outstream << "\t<droplist>\n";
    for (std::list<uint16_t>::iterator iter = mProfile.AutoDrop.begin(); iter != mProfile.AutoDrop.end(); iter++)
    {
        outstream << "\t\t<item id=\"" << *iter << "\" />";
        outstream << " <!--" << m_AshitaCore->GetResourceManager()->GetItemById(*iter)->Name[0] << "-->\n";
    }
    outstream << "\t</droplist>\n\n";

    outstream << "\t<storelist>\n";
    for (std::list<uint16_t>::iterator iter = mProfile.AutoStore.begin(); iter != mProfile.AutoStore.end(); iter++)
    {
        outstream << "\t\t<item id=\"" << *iter << "\" />";
        outstream << " <!--" << m_AshitaCore->GetResourceManager()->GetItemById(*iter)->Name[0] << "-->\n";
    }
    outstream << "\t</storelist>\n\n";

    outstream << "</lootwhore>";
    outstream.close();
    pOutput->message_f("Wrote profile XML. [$H%s$R]", ProfilePath.c_str());
}