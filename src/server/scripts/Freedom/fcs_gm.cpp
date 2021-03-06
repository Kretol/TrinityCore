#include "AccountMgr.h"
#include "Chat.h"
#include "DatabaseEnv.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "Realm.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "Language.h"
#include "World.h"
#include "WorldSession.h"
#include "FreedomMgr.h"
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

class fgm_commandscript : public CommandScript
{
public:
    fgm_commandscript() : CommandScript("fgm_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> gmCommandTable =
        {
            { "chat",           rbac::RBAC_PERM_COMMAND_GM_CHAT,        false,  &HandleGMChatCommand,                   "" },
            { "fly",            rbac::RBAC_PERM_COMMAND_GM_FLY,         false,  &HandleGMFlyCommand,                    "" },
            { "ingame",         rbac::RBAC_PERM_COMMAND_GM_INGAME,      true,   &HandleGMListIngameCommand,             "" },
            { "list",           rbac::RBAC_PERM_COMMAND_GM_LIST,        true,   &HandleGMListFullCommand,               "" },
            { "visible",        rbac::RBAC_PERM_COMMAND_GM_VISIBLE,     false,  &HandleGMVisibleCommand,                "" },
            { "",               rbac::RBAC_PERM_COMMAND_GM,             false,  &HandleGMCommand,                       "" },
        };
        static std::vector<ChatCommand> blacklistCommandTable =
        {
            { "item",           rbac::RBAC_FPERM_ADMINISTRATION,        false,  &HandleBlacklistItemCommand,            "" },
            { "gobject",        rbac::RBAC_FPERM_ADMINISTRATION,        false,  &HandleBlacklistGameobjectCommand,      "" },
            { "creature",       rbac::RBAC_FPERM_ADMINISTRATION,        false,  &HandleBlacklistCreatureCommand,      "" },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "gm",             rbac::RBAC_PERM_COMMAND_GM,             false,  NULL, "", gmCommandTable },
            { "blacklist",      rbac::RBAC_FPERM_ADMINISTRATION,        false,  NULL, "", blacklistCommandTable },
        };
        return commandTable;
    }

#pragma region BLACKLIST
    static bool HandleBlacklistItemCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_BLACKLIST_ITEM);
            return true;
        }

        char* id = handler->extractKeyFromLink((char*)args, "Hitem");
        char* flagStr = strtok(NULL, " ");

        uint32 itemId = atoul(id);
        uint32 flag = flagStr ? atoul(flagStr) : 1;

        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
        if (!itemTemplate)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_ID_NOT_FOUND, "Item", itemId);
            return true;
        }

        if (flag)
        {
            sFreedomMgr->SetItemTemplateExtraHiddenFlag(itemId, true);
            handler->PSendSysMessage(FREEDOM_CMDI_BLACKLIST_ITEM, "added to the blacklist");
        }
        else
        {
            sFreedomMgr->SetItemTemplateExtraHiddenFlag(itemId, false);
            handler->PSendSysMessage(FREEDOM_CMDI_BLACKLIST_ITEM, "removed from the blacklist");
        }
        
        return true;
    }

    static bool HandleBlacklistGameobjectCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_BLACKLIST_GAMEOBJECT);
            return true;
        }

        char* id = handler->extractKeyFromLink((char*)args, "Hgameobject_entry");
        char* flagStr = strtok(NULL, " ");

        uint32 entryId = atoul(id);
        uint32 flag = flagStr ? atoul(flagStr) : 1;

        GameObjectTemplate const* objectTemplate = sObjectMgr->GetGameObjectTemplate(entryId);
        if (!objectTemplate)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_ID_NOT_FOUND, "Gameobject", entryId);
            return true;
        }

        if (flag)
        {
            sFreedomMgr->SetGameobjectTemplateExtraDisabledFlag(entryId, true);
            handler->PSendSysMessage(FREEDOM_CMDI_BLACKLIST_GAMEOBJECT, "added to the blacklist");
        }
        else
        {
            sFreedomMgr->SetGameobjectTemplateExtraDisabledFlag(entryId, false);
            handler->PSendSysMessage(FREEDOM_CMDI_BLACKLIST_GAMEOBJECT, "removed from the blacklist");
        }

        return true;
    }

    static bool HandleBlacklistCreatureCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_BLACKLIST_CREATURE);
            return true;
        }

        char* id = handler->extractKeyFromLink((char*)args, "Hgameobject_entry");
        char* flagStr = strtok(NULL, " ");

        uint32 entryId = atoul(id);
        uint32 flag = flagStr ? atoul(flagStr) : 1;

        CreatureTemplate const* creatureTemplate = sObjectMgr->GetCreatureTemplate(entryId);
        if (!creatureTemplate)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_ID_NOT_FOUND, "Creature", entryId);
            return true;
        }

        if (flag)
        {
            sFreedomMgr->SetCreatureTemplateExtraDisabledFlag(entryId, true);
            handler->PSendSysMessage(FREEDOM_CMDI_BLACKLIST_CREATURE, "added to the blacklist");
        }
        else
        {
            sFreedomMgr->SetCreatureTemplateExtraDisabledFlag(entryId, false);
            handler->PSendSysMessage(FREEDOM_CMDI_BLACKLIST_CREATURE, "removed from the blacklist");
        }

        return true;
    }
#pragma endregion

    // Enables or disables hiding of the staff badge
    static bool HandleGMChatCommand(ChatHandler* handler, char const* args)
    {
        if (WorldSession* session = handler->GetSession())
        {
            if (!*args)
            {
                if (session->HasPermission(rbac::RBAC_PERM_CHAT_USE_STAFF_BADGE) && session->GetPlayer()->isGMChat())
                    session->SendNotification(LANG_GM_CHAT_ON);
                else
                    session->SendNotification(LANG_GM_CHAT_OFF);
                return true;
            }

            std::string param = (char*)args;

            if (param == "on")
            {
                session->GetPlayer()->SetGMChat(true);
                session->SendNotification(LANG_GM_CHAT_ON);
                return true;
            }

            if (param == "off")
            {
                session->GetPlayer()->SetGMChat(false);
                session->SendNotification(LANG_GM_CHAT_OFF);
                return true;
            }
        }

        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }

    static bool HandleGMFlyCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        Player* target = handler->getSelectedPlayer();
        if (!target)
            target = handler->GetSession()->GetPlayer();

        WorldPacket data;
        if (strncmp(args, "on", 3) == 0)
            target->SetCanFly(true);
        else if (strncmp(args, "off", 4) == 0)
            target->SetCanFly(false);
        else
        {
            handler->SendSysMessage(LANG_USE_BOL);
            return false;
        }
        handler->PSendSysMessage(LANG_COMMAND_FLYMODE_STATUS, handler->GetNameLink(target).c_str(), args);
        return true;
    }

    static bool HandleGMListIngameCommand(ChatHandler* handler, char const* /*args*/)
    {
        bool first = true;
        bool footer = false;

        boost::shared_lock<boost::shared_mutex> lock(*HashMapHolder<Player>::GetLock());
        HashMapHolder<Player>::MapType const& m = ObjectAccessor::GetPlayers();
        for (HashMapHolder<Player>::MapType::const_iterator itr = m.begin(); itr != m.end(); ++itr)
        {
            AccountTypes itrSec = itr->second->GetSession()->GetSecurity();
            if ((itr->second->IsGameMaster() ||
                (itr->second->GetSession()->HasPermission(rbac::RBAC_PERM_COMMANDS_APPEAR_IN_GM_LIST) &&
                    itrSec <= AccountTypes(sWorld->getIntConfig(CONFIG_GM_LEVEL_IN_GM_LIST)))) &&
                (!handler->GetSession() || itr->second->IsVisibleGloballyFor(handler->GetSession()->GetPlayer())))
            {
                if (first)
                {
                    first = false;
                    footer = true;
                    handler->SendSysMessage(LANG_GMS_ON_SRV);
                    handler->SendSysMessage("========================");
                }
                std::string const& name = itr->second->GetName();
                uint8 size = name.size();
                uint8 security = itrSec;
                uint8 max = ((16 - size) / 2);
                uint8 max2 = max;
                if ((max + max2 + size) == 16)
                    max2 = max - 1;
                if (handler->GetSession())
                    handler->PSendSysMessage("|    %s GMLevel %u", name.c_str(), security);
                else
                    handler->PSendSysMessage("|%*s%s%*s|   %u  |", max, " ", name.c_str(), max2, " ", security);
            }
        }
        if (footer)
            handler->SendSysMessage("========================");
        if (first)
            handler->SendSysMessage(LANG_GMS_NOT_LOGGED);
        return true;
    }

    /// Display the list of GMs
    static bool HandleGMListFullCommand(ChatHandler* handler, char const* /*args*/)
    {
        ///- Get the accounts with GM Level >0
        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_GM_ACCOUNTS);
        stmt->setUInt8(0, uint8(SEC_MODERATOR));
        stmt->setInt32(1, int32(realm.Id.Realm));
        PreparedQueryResult result = LoginDatabase.Query(stmt);

        if (result)
        {
            handler->SendSysMessage(LANG_GMLIST);
            handler->SendSysMessage("========================");
            ///- Cycle through them. Display username and GM level
            do
            {
                Field* fields = result->Fetch();
                char const* name = fields[0].GetCString();
                uint8 security = fields[1].GetUInt8();
                uint8 max = (16 - strlen(name)) / 2;
                uint8 max2 = max;
                if ((max + max2 + strlen(name)) == 16)
                    max2 = max - 1;
                if (handler->GetSession())
                    handler->PSendSysMessage("|    %s GMLevel %u", name, security);
                else
                    handler->PSendSysMessage("|%*s%s%*s|   %u  |", max, " ", name, max2, " ", security);
            } while (result->NextRow());
            handler->SendSysMessage("========================");
        }
        else
            handler->PSendSysMessage(LANG_GMLIST_EMPTY);
        return true;
    }

    //Enable\Disable Invisible mode
    static bool HandleGMVisibleCommand(ChatHandler* handler, char const* args)
    {
        Player* _player = handler->GetSession()->GetPlayer();

        if (!*args)
        {
            handler->PSendSysMessage(LANG_YOU_ARE, _player->isGMVisible() ? handler->GetTrinityString(LANG_VISIBLE) : handler->GetTrinityString(LANG_INVISIBLE));
            return true;
        }

        const uint32 VISUAL_AURA = 37800;
        std::string param = (char*)args;

        if (param == "on")
        {
            if (_player->HasAura(VISUAL_AURA))
                _player->RemoveAurasDueToSpell(VISUAL_AURA);

            _player->SetGMVisible(true);
            _player->UpdateObjectVisibility();
            handler->GetSession()->SendNotification(LANG_INVISIBLE_VISIBLE);
            return true;
        }

        if (param == "off")
        {
            _player->AddAura(VISUAL_AURA, _player);
            _player->SetGMVisible(false);
            _player->UpdateObjectVisibility();
            handler->GetSession()->SendNotification(LANG_INVISIBLE_INVISIBLE);
            return true;
        }

        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }

    //Enable\Disable GM Mode
    static bool HandleGMCommand(ChatHandler* handler, char const* args)
    {
        Player* _player = handler->GetSession()->GetPlayer();

        if (!*args)
        {
            handler->GetSession()->SendNotification(_player->IsGameMaster() ? LANG_GM_ON : LANG_GM_OFF);
            return true;
        }

        std::string param = (char*)args;

        if (param == "on")
        {
            _player->SetGameMaster(true);
            handler->GetSession()->SendNotification(LANG_GM_ON);
            _player->UpdateTriggerVisibility();
#ifdef _DEBUG_VMAPS
            VMAP::IVMapManager* vMapManager = VMAP::VMapFactory::createOrGetVMapManager();
            vMapManager->processCommand("stoplog");
#endif
            return true;
        }

        if (param == "off")
        {
            _player->SetGameMaster(false);
            handler->GetSession()->SendNotification(LANG_GM_OFF);
            _player->UpdateTriggerVisibility();
#ifdef _DEBUG_VMAPS
            VMAP::IVMapManager* vMapManager = VMAP::VMapFactory::createOrGetVMapManager();
            vMapManager->processCommand("startlog");
#endif
            return true;
        }

        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }
};

void AddSC_fgm_commandscript()
{
    new fgm_commandscript();
}

#ifdef FREEDOM_MOP_548_CODE

class fgm_commandscript : public CommandScript
{
public:
    fgm_commandscript() : CommandScript("fgm_commandscript") { }

    ChatCommand* GetCommands() const OVERRIDE
    {
        static ChatCommand demoteToCommandTable[] =
        {
            { "player",     rbac::RBAC_PERM_COMMAND_PROMOTION_DEMOTION,     false, &HandleDemoteToPlayer,                           "", NULL },
            { NULL, 0, false, NULL, "", NULL }
        };
        static ChatCommand promoteToCommandTable[] =
        {
            { "moderator",  rbac::RBAC_PERM_COMMAND_PROMOTION_DEMOTION,     false, &HandlePromoteToModerator,                       "", NULL },
            { NULL, 0, false, NULL, "", NULL }
        };
        static ChatCommand gmCommandTable[] =
        {
            { "chat",       rbac::RBAC_PERM_COMMAND_GM_CHAT,                false, &HandleGMChatCommand,                            "", NULL },
            { "fly",        rbac::RBAC_PERM_COMMAND_GM_FLY,                 false, &HandleGMFlyCommand,                             "", NULL },
            { "ingame",     rbac::RBAC_PERM_COMMAND_GM_INGAME,              true,  &HandleGMListIngameCommand,                      "", NULL },
            { "list",       rbac::RBAC_PERM_COMMAND_GM_LIST,                true,  &HandleGMListFullCommand,                        "", NULL },
            { "visible",    rbac::RBAC_PERM_COMMAND_GM_VISIBLE,             false, &HandleGMVisibleCommand,                         "", NULL },
            { "",           rbac::RBAC_PERM_COMMAND_GM,                     false, &HandleGMCommand,                                "", NULL },
            { NULL, 0, false, NULL, "", NULL }
        };
        static ChatCommand commandTable[] =
        {
            { "gm",         rbac::RBAC_PERM_COMMAND_GM,                     false, NULL,                                            "", gmCommandTable },
            { "promoteto",  rbac::RBAC_PERM_COMMAND_PROMOTION_DEMOTION,     false, NULL,                                            "", promoteToCommandTable },
            { "demoteto",   rbac::RBAC_PERM_COMMAND_PROMOTION_DEMOTION,     false, NULL,                                            "", demoteToCommandTable },
            { NULL, 0, false, NULL, "", NULL }
        };
        return commandTable;
    }

    static bool HandleDemoteToPlayer(ChatHandler* handler, char const* args)
    {
        Player* source = handler->GetSession()->GetPlayer();
        Player* target = NULL;
        uint32 target_account_id = 0;
        std::string target_account_username = "";

        // get target account through 3 possible means: 
        // # username (if param is a word); 
        // # account id (if param is a number); 
        // # target player (if no param)
        if (*args)
        {
            std::string param = args;

            target_account_id = sAccountMgr->GetId(param);

            if (!target_account_id)
            {
                sAccountMgr->GetName(atol(param.c_str()), target_account_username);
                target_account_id = sAccountMgr->GetId(target_account_username);
            } 
            else
            {
                sAccountMgr->GetName(target_account_id, target_account_username);
            }
        }
        else
        {
            target = handler->getSelectedPlayer();
            
            if (target)
            {
                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ACCOUNT_BY_GUID);
                stmt->setUInt32(0, GUID_LOPART(target->GetGUIDLow()));
                PreparedQueryResult result = CharacterDatabase.Query(stmt);

                if (result)
                {
                    target_account_id = result->Fetch()[0].GetUInt32();
                    sAccountMgr->GetName(target_account_id, target_account_username);
                }
            }
        }

        if (!target_account_id || target_account_username.empty())
        {
            handler->PSendSysMessage("Target account does not exist");
            return true;
        }

        uint32 target_sec_level = sAccountMgr->GetSecurity(target_account_id);
        
        if (target_sec_level > SEC_MODERATOR)
        {
            handler->PSendSysMessage("Target account is a GM or higher. Demotion not allowed.");
            return true;
        }

        sAccountMgr->UpdateAccountAccess(NULL, target_account_id, SEC_PLAYER, -1);

        // notify source and target (if target is on) about changes
        if (handler->needReportToTarget(target))
        {
            ChatHandler(target->GetSession()).PSendSysMessage(">> Your account has been demoted to %s|Hgmlevel:0|h[PLAYER]|h|r rank by %s.",
                MSG_COLOR_LIGHTBLUE, handler->GetNameLink().c_str());
        }

        handler->PSendSysMessage(">> Account %s|Haccount:%u|h[%s]|h|r has been successfully demoted to %s|Hgmlevel:0|h[PLAYER]|h|r rank.",
            MSG_COLOR_LIGHTBLUE, target_account_id, target_account_username.c_str(), MSG_COLOR_LIGHTBLUE);
        handler->PSendSysMessage("Hint: You should kick or make him/her fully relog with the account for the changes to take place.");

        return true;
    }

    static bool HandlePromoteToModerator(ChatHandler* handler, char const* args)
    {
        Player* source = handler->GetSession()->GetPlayer();
        Player* target = NULL;
        uint32 target_account_id = 0;
        std::string target_account_username = "";

        // get target account through 3 possible means: 
        // # username (if param is a word); 
        // # account id (if param is a number); 
        // # target player (if no param)
        if (*args)
        {
            std::string param = args;

            target_account_id = sAccountMgr->GetId(param);

            if (!target_account_id)
            {
                sAccountMgr->GetName(atol(param.c_str()), target_account_username);
                target_account_id = sAccountMgr->GetId(target_account_username);
            }
            else
            {
                sAccountMgr->GetName(target_account_id, target_account_username);
            }
        }
        else
        {
            target = handler->getSelectedPlayer();

            if (target)
            {
                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ACCOUNT_BY_GUID);
                stmt->setUInt32(0, GUID_LOPART(target->GetGUIDLow()));
                PreparedQueryResult result = CharacterDatabase.Query(stmt);

                if (result)
                {
                    target_account_id = result->Fetch()[0].GetUInt32();
                    sAccountMgr->GetName(target_account_id, target_account_username);
                }
            }
        }

        if (!target_account_id || target_account_username.empty())
        {
            handler->PSendSysMessage("Target account does not exist");
            return true;
        }

        uint32 target_sec_level = sAccountMgr->GetSecurity(target_account_id);

        if (target_sec_level > SEC_MODERATOR)
        {
            handler->PSendSysMessage("Target account is a GM or higher. Demotion not allowed.");
            return true;
        }

        sAccountMgr->UpdateAccountAccess(NULL, target_account_id, SEC_MODERATOR, -1);

        // notify source and target (if target is on) about changes
        if (handler->needReportToTarget(target))
        {
            ChatHandler(target->GetSession()).PSendSysMessage(">> Your account has been promoted to %s|Hgmlevel:1|h[MODERATOR]|h|r rank by %s.",
                MSG_COLOR_LIGHTBLUE, handler->GetNameLink().c_str());
        }

        handler->PSendSysMessage(">> Account %s|Haccount:%u|h[%s]|h|r has been successfully promoted to %s|Hgmlevel:1|h[MODERATOR]|h|r rank.",
            MSG_COLOR_LIGHTBLUE, target_account_id, target_account_username.c_str(), MSG_COLOR_LIGHTBLUE);
        handler->PSendSysMessage("Hint: You should kick or make him/her fully relog with the account for the changes to take place.");

        return true;
    }

    // Enables or disables hiding of the staff badge
    static bool HandleGMChatCommand(ChatHandler* handler, char const* args)
    {
        if (WorldSession* session = handler->GetSession())
        {
            if (!*args)
            {
                if (session->HasPermission(rbac::RBAC_PERM_CHAT_USE_STAFF_BADGE) && session->GetPlayer()->isGMChat())
                    session->SendNotification(LANG_GM_CHAT_ON);
                else
                    session->SendNotification(LANG_GM_CHAT_OFF);
                return true;
            }

            std::string param = (char*)args;

            if (param == "on")
            {
                session->GetPlayer()->SetGMChat(true);
                session->SendNotification(LANG_GM_CHAT_ON);
                return true;
            }

            if (param == "off")
            {
                session->GetPlayer()->SetGMChat(false);
                session->SendNotification(LANG_GM_CHAT_OFF);
                return true;
            }
        }

        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }

    static bool HandleGMFlyCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        Player* target = handler->getSelectedPlayer();
        if (!target)
            target = handler->GetSession()->GetPlayer();

        WorldPacket data;
        if (strncmp(args, "on", 3) == 0)
            target->SetCanFly(true);
        else if (strncmp(args, "off", 4) == 0)
            target->SetCanFly(false);
        else
        {
            handler->SendSysMessage(LANG_USE_BOL);
            return false;
        }
        //handler->PSendSysMessage("TEST");
        handler->PSendSysMessage(LANG_COMMAND_FLYMODE_STATUS, handler->GetNameLink(target).c_str(), args);
        return true;
    }

    static bool HandleGMListIngameCommand(ChatHandler* handler, char const* /*args*/)
    {
        bool first = true;
        bool footer = false;

        TRINITY_READ_GUARD(HashMapHolder<Player>::LockType, *HashMapHolder<Player>::GetLock());
        HashMapHolder<Player>::MapType const& m = sObjectAccessor->GetPlayers();
        for (HashMapHolder<Player>::MapType::const_iterator itr = m.begin(); itr != m.end(); ++itr)
        {
            AccountTypes itrSec = itr->second->GetSession()->GetSecurity();
            if ((itr->second->IsGameMaster() ||
                (itr->second->GetSession()->HasPermission(rbac::RBAC_PERM_COMMANDS_APPEAR_IN_GM_LIST) &&
                itrSec <= AccountTypes(sWorld->getIntConfig(CONFIG_GM_LEVEL_IN_GM_LIST)))) &&
                (!handler->GetSession() || itr->second->IsVisibleGloballyFor(handler->GetSession()->GetPlayer())))
            {
                if (first)
                {
                    first = false;
                    footer = true;
                    handler->SendSysMessage(LANG_GMS_ON_SRV);
                    handler->SendSysMessage("========================");
                }
                std::string const& name = itr->second->GetName();
                uint8 size = name.size();
                uint8 security = itrSec;
                uint8 max = ((16 - size) / 2);
                uint8 max2 = max;
                if ((max + max2 + size) == 16)
                    max2 = max - 1;
                if (handler->GetSession())
                    handler->PSendSysMessage("|    %s GMLevel %u", name.c_str(), security);
                else
                    handler->PSendSysMessage("|%*s%s%*s|   %u  |", max, " ", name.c_str(), max2, " ", security);
            }
        }
        if (footer)
            handler->SendSysMessage("========================");
        if (first)
            handler->SendSysMessage(LANG_GMS_NOT_LOGGED);
        return true;
    }

    /// Display the list of GMs
    static bool HandleGMListFullCommand(ChatHandler* handler, char const* /*args*/)
    {
        ///- Get the accounts with GM Level >0
        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_GM_ACCOUNTS);
        stmt->setUInt8(0, uint8(SEC_MODERATOR));
        stmt->setInt32(1, int32(realmID));
        PreparedQueryResult result = LoginDatabase.Query(stmt);

        if (result)
        {
            handler->SendSysMessage(LANG_GMLIST);
            handler->SendSysMessage("========================");
            ///- Cycle through them. Display username and GM level
            do
            {
                Field* fields = result->Fetch();
                char const* name = fields[0].GetCString();
                uint8 security = fields[1].GetUInt8();
                uint8 max = (16 - strlen(name)) / 2;
                uint8 max2 = max;
                if ((max + max2 + strlen(name)) == 16)
                    max2 = max - 1;
                if (handler->GetSession())
                    handler->PSendSysMessage("|    %s GMLevel %u", name, security);
                else
                    handler->PSendSysMessage("|%*s%s%*s|   %u  |", max, " ", name, max2, " ", security);
            } while (result->NextRow());
            handler->SendSysMessage("========================");
        }
        else
            handler->PSendSysMessage(LANG_GMLIST_EMPTY);
        return true;
    }

    //Enable\Disable Invisible mode
    static bool HandleGMVisibleCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(LANG_YOU_ARE, handler->GetSession()->GetPlayer()->isGMVisible() ? handler->GetTrinityString(LANG_VISIBLE) : handler->GetTrinityString(LANG_INVISIBLE));
            return true;
        }

        const uint32 VISUAL_AURA = 37800;
        std::string param = (char*)args;
        Player* player = handler->GetSession()->GetPlayer();

        if (param == "on")
        {
            if (player->HasAura(VISUAL_AURA, 0))
                player->RemoveAurasDueToSpell(VISUAL_AURA);

            player->SetGMVisible(true);
            handler->GetSession()->SendNotification(LANG_INVISIBLE_VISIBLE);
            return true;
        }

        if (param == "off")
        {
            handler->GetSession()->SendNotification(LANG_INVISIBLE_INVISIBLE);
            player->SetGMVisible(false);

            player->AddAura(VISUAL_AURA, player);

            return true;
        }

        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }

    //Enable\Disable GM Mode
    static bool HandleGMCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            if (handler->GetSession()->GetPlayer()->IsGameMaster())
                handler->GetSession()->SendNotification(LANG_GM_ON);
            else
                handler->GetSession()->SendNotification(LANG_GM_OFF);
            return true;
        }

        std::string param = (char*)args;

        if (param == "on")
        {
            handler->GetSession()->GetPlayer()->SetGameMaster(true);
            handler->GetSession()->SendNotification(LANG_GM_ON);
            handler->GetSession()->GetPlayer()->UpdateTriggerVisibility();
#ifdef _DEBUG_VMAPS
            VMAP::IVMapManager* vMapManager = VMAP::VMapFactory::createOrGetVMapManager();
            vMapManager->processCommand("stoplog");
#endif
            return true;
        }

        if (param == "off")
        {
            handler->GetSession()->GetPlayer()->SetGameMaster(false);
            handler->GetSession()->SendNotification(LANG_GM_OFF);
            handler->GetSession()->GetPlayer()->UpdateTriggerVisibility();
#ifdef _DEBUG_VMAPS
            VMAP::IVMapManager* vMapManager = VMAP::VMapFactory::createOrGetVMapManager();
            vMapManager->processCommand("startlog");
#endif
            return true;
        }

        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }
};

void AddSC_fgm_commandscript()
{
    new fgm_commandscript();
}

#endif
