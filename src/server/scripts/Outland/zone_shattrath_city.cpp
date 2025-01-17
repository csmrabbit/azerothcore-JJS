/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* ScriptData
SDName: Shattrath_City
SD%Complete: 100
SDComment: Quest support: 10004, 10009, 10211. Flask vendors, Teleport to Caverns of Time
SDCategory: Shattrath City
EndScriptData */

/* ContentData
npc_raliq_the_drunk
npc_salsalabim
npc_shattrathflaskvendors
npc_zephyr
npc_kservant
EndContentData */

#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"
#include "ScriptedGossip.h"

/*######
## npc_raliq_the_drunk
######*/

#define GOSSIP_RALIQ            "你欠萨萨拉比姆的钱。把钱交出来，否则就得死!"

enum Raliq
{
    SPELL_UPPERCUT          = 10966,
    QUEST_CRACK_SKULLS      = 10009
};

class npc_raliq_the_drunk : public CreatureScript
{
public:
    npc_raliq_the_drunk() : CreatureScript("npc_raliq_the_drunk") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        ClearGossipMenuFor(player);
        if (action == GOSSIP_ACTION_INFO_DEF + 1)
        {
            CloseGossipMenuFor(player);
            creature->SetFaction(FACTION_OGRE);
            creature->AI()->AttackStart(player);
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (player->GetQuestStatus(QUEST_CRACK_SKULLS) == QUEST_STATUS_INCOMPLETE)
            AddGossipItemFor(player, GOSSIP_ICON_VENDOR, GOSSIP_RALIQ, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

        SendGossipMenuFor(player, 9440, creature->GetGUID());
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_raliq_the_drunkAI(creature);
    }

    struct npc_raliq_the_drunkAI : public ScriptedAI
    {
        npc_raliq_the_drunkAI(Creature* creature) : ScriptedAI(creature)
        {
            m_uiNormFaction = creature->GetFaction();
        }

        uint32 m_uiNormFaction;
        uint32 Uppercut_Timer;

        void Reset() override
        {
            Uppercut_Timer = 5000;
            me->RestoreFaction();
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (Uppercut_Timer <= diff)
            {
                DoCastVictim(SPELL_UPPERCUT);
                Uppercut_Timer = 15000;
            }
            else Uppercut_Timer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

/*######
# npc_salsalabim
######*/

enum Salsalabim
{
    // Quests
    QUEST_10004                    = 10004,

    // Spells
    SPELL_MAGNETIC_PULL            = 31705
};

class npc_salsalabim : public CreatureScript
{
public:
    npc_salsalabim() : CreatureScript("npc_salsalabim") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (player->GetQuestStatus(QUEST_10004) == QUEST_STATUS_INCOMPLETE)
        {
            creature->SetFaction(FACTION_DEMON);
            creature->AI()->AttackStart(player);
        }
        else
        {
            if (creature->IsQuestGiver())
                player->PrepareQuestMenu(creature->GetGUID());
            SendGossipMenuFor(player, player->GetGossipTextId(creature), creature->GetGUID());
        }
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_salsalabimAI(creature);
    }

    struct npc_salsalabimAI : public ScriptedAI
    {
        npc_salsalabimAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 MagneticPull_Timer;

        void Reset() override
        {
            MagneticPull_Timer = 15000;
            me->RestoreFaction();
        }

        void DamageTaken(Unit* done_by, uint32& damage, DamageEffectType, SpellSchoolMask) override
        {
            // xinef: some corrections
            if (done_by)
                if (Player* player = done_by->GetCharmerOrOwnerPlayerOrPlayerItself())
                    if (me->HealthBelowPctDamaged(20, damage))
                    {
                        player->GroupEventHappens(QUEST_10004, me);
                        damage = 0;
                        EnterEvadeMode();
                    }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (MagneticPull_Timer <= diff)
            {
                DoCastVictim(SPELL_MAGNETIC_PULL);
                MagneticPull_Timer = 15000;
            }
            else MagneticPull_Timer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

/*
##################################################
Shattrath City Flask Vendors provides flasks to people exalted with 3 fActions:
Haldor the Compulsive
Arcanist Xorith
Both sell special flasks for use in Outlands 25man raids only,
purchasable for one Mark of Illidari each
Purchase requires exalted reputation with Scryers/Aldor, Cenarion Expedition and The Sha'tar
##################################################
*/

class npc_shattrathflaskvendors : public CreatureScript
{
public:
    npc_shattrathflaskvendors() : CreatureScript("npc_shattrathflaskvendors") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        ClearGossipMenuFor(player);
        if (action == GOSSIP_ACTION_TRADE)
            player->GetSession()->SendListInventory(creature->GetGUID());

        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (creature->GetEntry() == 23484)
        {
            // Aldor vendor
            if (creature->IsVendor() && (player->GetReputationRank(932) == REP_EXALTED) && (player->GetReputationRank(935) == REP_EXALTED) && (player->GetReputationRank(942) == REP_EXALTED))
            {
                AddGossipItemFor(player, GOSSIP_ICON_VENDOR, GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);
                SendGossipMenuFor(player, 11085, creature->GetGUID());
            }
            else
            {
                SendGossipMenuFor(player, 11083, creature->GetGUID());
            }
        }

        if (creature->GetEntry() == 23483)
        {
            // Scryers vendor
            if (creature->IsVendor() && (player->GetReputationRank(934) == REP_EXALTED) && (player->GetReputationRank(935) == REP_EXALTED) && (player->GetReputationRank(942) == REP_EXALTED))
            {
                AddGossipItemFor(player, GOSSIP_ICON_VENDOR, GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);
                SendGossipMenuFor(player, 11085, creature->GetGUID());
            }
            else
            {
                SendGossipMenuFor(player, 11084, creature->GetGUID());
            }
        }

        return true;
    }
};

/*######
# npc_zephyr
######*/

#define GOSSIP_HZ "送我去时光之穴。"

class npc_zephyr : public CreatureScript
{
public:
    npc_zephyr() : CreatureScript("npc_zephyr") { }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action) override
    {
        ClearGossipMenuFor(player);
        if (action == GOSSIP_ACTION_INFO_DEF + 1)
            player->CastSpell(player, 37778, false);

        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (player->GetReputationRank(989) >= REP_REVERED)
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, GOSSIP_HZ, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

        SendGossipMenuFor(player, player->GetGossipTextId(creature), creature->GetGUID());

        return true;
    }
};

/*######
# npc_kservant
######*/

enum KServant
{
    SAY1       = 0,
    WHISP1     = 1,
    WHISP2     = 2,
    WHISP3     = 3,
    WHISP4     = 4,
    WHISP5     = 5,
    WHISP6     = 6,
    WHISP7     = 7,
    WHISP8     = 8,
    WHISP9     = 9,
    WHISP10    = 10,
    WHISP11    = 11,
    WHISP12    = 12,
    WHISP13    = 13,
    WHISP14    = 14,
    WHISP15    = 15,
    WHISP16    = 16,
    WHISP17    = 17,
    WHISP18    = 18,
    WHISP19    = 19,
    WHISP20    = 20,
    WHISP21    = 21
};

class npc_kservant : public CreatureScript
{
public:
    npc_kservant() : CreatureScript("npc_kservant") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_kservantAI(creature);
    }

    struct npc_kservantAI : public npc_escortAI
    {
    public:
        npc_kservantAI(Creature* creature) : npc_escortAI(creature) { }

        void WaypointReached(uint32 waypointId) override
        {
            Player* player = GetPlayerForEscort();
            if (!player)
                return;

            switch (waypointId)
            {
                case 0:
                    Talk(SAY1, player);
                    break;
                case 4:
                    Talk(WHISP1, player);
                    break;
                case 6:
                    Talk(WHISP2, player);
                    break;
                case 7:
                    Talk(WHISP3, player);
                    break;
                case 8:
                    Talk(WHISP4, player);
                    break;
                case 17:
                    Talk(WHISP5, player);
                    break;
                case 18:
                    Talk(WHISP6, player);
                    break;
                case 19:
                    Talk(WHISP7, player);
                    break;
                case 33:
                    Talk(WHISP8, player);
                    break;
                case 34:
                    Talk(WHISP9, player);
                    break;
                case 35:
                    Talk(WHISP10, player);
                    break;
                case 36:
                    Talk(WHISP11, player);
                    break;
                case 43:
                    Talk(WHISP12, player);
                    break;
                case 44:
                    Talk(WHISP13, player);
                    break;
                case 49:
                    Talk(WHISP14, player);
                    break;
                case 50:
                    Talk(WHISP15, player);
                    break;
                case 51:
                    Talk(WHISP16, player);
                    break;
                case 52:
                    Talk(WHISP17, player);
                    break;
                case 53:
                    Talk(WHISP18, player);
                    break;
                case 54:
                    Talk(WHISP19, player);
                    break;
                case 55:
                    Talk(WHISP20, player);
                    break;
                case 56:
                    Talk(WHISP21, player);
                    player->GroupEventHappens(10211, me);
                    break;
            }
        }

        void IsSummonedBy(WorldObject* summoner) override
        {
            if (!summoner)
                return;

            Player* player = summoner->ToPlayer();
            if (player && player->GetQuestStatus(10211) == QUEST_STATUS_INCOMPLETE)
                Start(false, false, summoner->GetGUID());
        }

        void Reset() override { }
    };
};

void AddSC_shattrath_city()
{
    new npc_raliq_the_drunk();
    new npc_salsalabim();
    new npc_shattrathflaskvendors();
    new npc_zephyr();
    new npc_kservant();
}
