#include "stdafx.h"
#include "enemy_factory.h"
#include "name_generator.h"
#include "technology.h"
#include "attack_trigger.h"
#include "external_enemies.h"
#include "immigrant_info.h"
#include "settlement_info.h"
#include "enemy_info.h"

EnemyFactory::EnemyFactory(RandomGen& r) : random(r) {
}

EnemyInfo::EnemyInfo(SettlementInfo s, CollectiveConfig c, optional<VillageBehaviour> v,
    optional<LevelConnection> l)
  : settlement(s), config(c), villain(v), levelConnection(l) {
}

EnemyInfo& EnemyInfo::setVillainType(VillainType type) {
  villainType = type;
  return *this;
}

EnemyInfo& EnemyInfo::setId(EnemyId i) {
  id = i;
  return *this;
}

EnemyInfo& EnemyInfo::setNonDiscoverable() {
  discoverable = false;
  return *this;
}

static EnemyInfo getVault(SettlementType type, CreatureId creature, TribeId tribe, int num,
    optional<ItemFactory> itemFactory = none) {
  return EnemyInfo(CONSTRUCT(SettlementInfo,
      c.type = type;
      c.inhabitants.fighters = make_pair(num, makeVec(make_pair(1, creature)));
      c.tribe = tribe;
      c.buildingId = BuildingId::DUNGEON;
      c.shopFactory = itemFactory;
    ), CollectiveConfig::noImmigrants())
    .setNonDiscoverable();
}

struct FriendlyVault {
  CreatureId id;
  int min;
  int max;
};

static vector<FriendlyVault> friendlyVaults {
 // {CreatureId::SPECIAL_HUMANOID, 1, 2},
  {CreatureId::ORC, 3, 8},
  {CreatureId::OGRE, 2, 5},
  {CreatureId::VAMPIRE, 2, 5},
};

vector<EnemyInfo> EnemyFactory::getVaults() {
  vector<EnemyInfo> ret {
 /*   getVault(SettlementType::VAULT, CreatureFactory::insects(TribeId::getMonster()),
        TribeId::getMonster(), random.get(6, 12)),*/
    getVault(SettlementType::VAULT, CreatureId::RAT, TribeId::getPest(), random.get(3, 8),
        ItemFactory::armory()),
  };
  for (int i : Range(random.get(1, 3))) {
    FriendlyVault v = random.choose(friendlyVaults);
    ret.push_back(getVault(SettlementType::VAULT, v.id, TribeId::getKeeper(), random.get(v.min, v.max)));
  }
  return ret;
}

static string getVillageName() {
  return NameGenerator::get(NameGeneratorId::TOWN)->getNext();
}

EnemyInfo EnemyFactory::get(EnemyId id){
  return getById(id).setId(id);
}

EnemyInfo EnemyFactory::getById(EnemyId enemyId) {
  switch (enemyId) {
     case EnemyId::UNICORN_HERD:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::FOREST;
            c.inhabitants.fighters = make_pair(
                random.get(10, 15),
                makeVec(make_pair(1, CreatureId::UNICORN)));
            c.stockpiles = LIST({StockpileInfo::GOLD, 100});
            c.tribe = TribeId::getMonster();
            c.race = "unicorns"_s;
            c.buildingId = BuildingId::WOOD;),
          CollectiveConfig::withImmigrants(200, 15, {
              ImmigrantInfo(CreatureId::UNICORN, {MinionTrait::FIGHTER}).setFrequency(1),
          })); 
    case EnemyId::ANTS_CLOSED:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::ANT_NEST;
            c.inhabitants.leader = CreatureId::ANT_QUEEN;
            c.inhabitants.civilians = make_pair(
                random.get(5, 7),
                makeVec(make_pair(1, CreatureId::ANT_WORKER)));
            c.inhabitants.fighters = make_pair(
                random.get(5, 7),
                makeVec(make_pair(1, CreatureId::ANT_SOLDIER)));
            c.tribe = TribeId::getAnt();
            c.race = "ants"_s;
            c.buildingId = BuildingId::DUNGEON;),
          CollectiveConfig::withImmigrants(500, 15, {
            ImmigrantInfo(CreatureId::ANT_WORKER, {}).setFrequency(1),
            ImmigrantInfo(CreatureId::ANT_SOLDIER, {MinionTrait::FIGHTER}).setFrequency(1)}),
          CONSTRUCT(VillageBehaviour,
            c.minPopulation = 1;
            c.minTeamSize = 4;
            c.triggers = LIST(AttackTriggerId::ENTRY);
            c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);));
    case EnemyId::ANTS_OPEN: {
        auto ants = get(EnemyId::ANTS_CLOSED);
        ants.settlement.type = SettlementType::MINETOWN;
        return ants;
      }
    case EnemyId::ORC_VILLAGE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::VILLAGE;
            c.tribe = TribeId::getGreenskin();
            c.inhabitants.fighters = make_pair(
                random.get(12, 16),
                makeVec(make_pair(1, CreatureId::ORC), make_pair(1, CreatureId::OGRE)));
            c.locationName = getVillageName();
            c.race = "greenskins"_s;
            c.buildingId = BuildingId::BRICK;
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);
            c.outsideFeatures = FurnitureFactory::villageOutside(c.tribe);),
          CollectiveConfig::withImmigrants(300, 16, {
              ImmigrantInfo(CreatureId::ORC, {MinionTrait::FIGHTER}).setFrequency(3),
              ImmigrantInfo(CreatureId::OGRE, {MinionTrait::FIGHTER}).setFrequency(1)
            }));
    case EnemyId::DEMON_DEN_ABOVE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::VILLAGE;
            c.tribe = TribeId::getWildlife();
            c.inhabitants.fighters = make_pair(
                random.get(2, 3),
                makeVec(make_pair(1, CreatureId::GHOST)));
            c.buildingId = BuildingId::DUNGEON_SURFACE;
            c.locationName = "Darkshrine Town"_s;
            c.race = "ghosts"_s;
            c.furniture = FurnitureFactory::dungeonOutside(c.tribe);
            ),
            CollectiveConfig::noImmigrants()).setNonDiscoverable();
    case EnemyId::DEMON_DEN:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.tribe = TribeId::getMonster();
            c.inhabitants.leader = CreatureId::DEMON_LORD;
            c.inhabitants.fighters = make_pair(
                random.get(5, 10),
                makeVec(make_pair(1, CreatureId::DEMON_DWELLER)));
            c.locationName = "Darkshrine"_s;
            c.race = "demons"_s;
            c.furniture = FurnitureFactory::dungeonOutside(c.tribe);
            c.outsideFeatures = FurnitureFactory::dungeonOutside(c.tribe);),
          CollectiveConfig::withImmigrants(300, 16, {
              ImmigrantInfo(CreatureId::DEMON_DWELLER, {MinionTrait::FIGHTER}).setFrequency(1),
          }),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 0;
              c.minTeamSize = 3;
              c.triggers = LIST(
                  {AttackTriggerId::ROOM_BUILT, FurnitureType::DEMON_SHRINE});
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);
              c.ransom = make_pair(0.5, random.get(50, 100));),
          LevelConnection{LevelConnection::CRYPT, get(EnemyId::DEMON_DEN_ABOVE)});
    case EnemyId::VILLAGE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::VILLAGE;
            c.tribe = TribeId::getHuman();
            c.inhabitants.leader = CreatureId::KNIGHT;
            c.inhabitants.fighters = make_pair(
                random.get(4, 8),
                makeVec(make_pair(1, CreatureId::KNIGHT), make_pair(1, CreatureId::ARCHER)));
            c.inhabitants.civilians = make_pair(
                random.get(4, 8),
                makeVec(make_pair(1, CreatureId::PESEANT), make_pair(1, CreatureId::CHILD),
                    make_pair(1, CreatureId::DONKEY), make_pair(1, CreatureId::HORSE),
                    make_pair(1, CreatureId::COW), make_pair(1, CreatureId::PIG), make_pair(1, CreatureId::DOG)));
            c.locationName = getVillageName();
            c.race = "humans"_s;
            c.buildingId = BuildingId::WOOD;
            c.shopFactory = ItemFactory::armory();
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);
            ), CollectiveConfig::noImmigrants().setGhostSpawns(0.1, 4));
    case EnemyId::WARRIORS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CASTLE2;
            c.tribe = TribeId::getHuman();
            c.inhabitants.leader = CreatureId::SHAMAN;
            c.inhabitants.fighters = make_pair(
                random.get(7, 10),
                makeVec(make_pair(1, CreatureId::WARRIOR)));
            c.locationName = getVillageName();
            c.race = "humans"_s;
            c.buildingId = BuildingId::WOOD_CASTLE;
            c.stockpiles = LIST({StockpileInfo::GOLD, 160});
            c.guardId = CreatureId::WARRIOR;
            c.elderLoot = ItemType(ItemType::TechBook{TechId::BEAST_MUT});
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);
            c.outsideFeatures = FurnitureFactory::castleOutside(c.tribe);),
          CollectiveConfig::withImmigrants(300, 10, {
              ImmigrantInfo(CreatureId::WARRIOR, {MinionTrait::FIGHTER}).setFrequency(1),
            }).setGhostSpawns(0.1, 6),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 3;
              c.minTeamSize = 5;
              c.triggers = LIST(
                  {AttackTriggerId::ROOM_BUILT, FurnitureType::THRONE},
                  AttackTriggerId::SELF_VICTIMS,
                  {AttackTriggerId::NUM_CONQUERED, 2},
                  AttackTriggerId::STOLEN_ITEMS,
                  {AttackTriggerId::ROOM_BUILT, FurnitureType::IMPALED_HEAD},
                  AttackTriggerId::FINISH_OFF,
                  AttackTriggerId::PROXIMITY);
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);
              c.ransom = make_pair(0.8, random.get(100, 140));));
    case EnemyId::KNIGHTS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CASTLE;
            c.tribe = TribeId::getHuman();
            c.inhabitants.leader = CreatureId::AVATAR;
            c.inhabitants.fighters = make_pair(
                random.get(12, 17),
                makeVec(make_pair(1, CreatureId::KNIGHT), make_pair(1, CreatureId::ARCHER)));
            c.inhabitants.civilians = make_pair(
                random.get(6, 9),
                makeVec(make_pair(1, CreatureId::PESEANT), make_pair(1, CreatureId::CHILD),
                    make_pair(1, CreatureId::DONKEY), make_pair(1, CreatureId::HORSE),
                    make_pair(1, CreatureId::COW), make_pair(1, CreatureId::PIG), make_pair(1, CreatureId::DOG)));
            c.locationName = getVillageName();
            c.race = "humans"_s;
            c.stockpiles = LIST({StockpileInfo::GOLD, 140});
            c.buildingId = BuildingId::BRICK;
            c.guardId = CreatureId::KNIGHT;
            c.shopFactory = ItemFactory::villageShop();
            c.furniture = FurnitureFactory::castleFurniture(c.tribe);
            c.outsideFeatures = FurnitureFactory::castleOutside(c.tribe);),
          CollectiveConfig::withImmigrants(300, 26, {
              ImmigrantInfo(CreatureId::ARCHER, {MinionTrait::FIGHTER}).setFrequency(1),
              ImmigrantInfo(CreatureId::KNIGHT, {MinionTrait::FIGHTER}).setFrequency(1)
            }).setGhostSpawns(0.1, 6),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 12;
              c.minTeamSize = 10;
              c.triggers = LIST(
                  {AttackTriggerId::ROOM_BUILT, FurnitureType::THRONE},
                  AttackTriggerId::SELF_VICTIMS,
                  {AttackTriggerId::NUM_CONQUERED, 3},
                  AttackTriggerId::STOLEN_ITEMS,
                  {AttackTriggerId::ROOM_BUILT, FurnitureType::IMPALED_HEAD},
                  AttackTriggerId::FINISH_OFF,
                  AttackTriggerId::PROXIMITY);
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);
              c.ransom = make_pair(0.9, random.get(280, 400));),
          random.roll(4) ? LevelConnection{LevelConnection::MAZE, get(EnemyId::MINOTAUR)}
              : optional<LevelConnection>(none));
    case EnemyId::MINOTAUR:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.tribe = TribeId::getMonster();
            c.inhabitants.leader = CreatureId::MINOTAUR;
            c.locationName = "maze"_s;
            c.race = "monsters"_s;
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);
            c.buildingId = BuildingId::BRICK;), CollectiveConfig::noImmigrants());
    case EnemyId::RED_DRAGON:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.inhabitants.leader = CreatureId::RED_DRAGON;
            c.race = "dragon"_s;
            c.tribe = TribeId::getMonster();
            c.buildingId = BuildingId::DUNGEON;
            c.shopFactory = ItemFactory::dragonCave();), CollectiveConfig::noImmigrants().setLeaderAsFighter()
          .setGhostSpawns(0.03, 1),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 0;
              c.minTeamSize = 1;
              c.triggers = LIST(
                  {AttackTriggerId::ENEMY_POPULATION, 22},
                  AttackTriggerId::STOLEN_ITEMS,
                  {AttackTriggerId::NUM_CONQUERED, 3},
                  AttackTriggerId::FINISH_OFF,
                  AttackTriggerId::PROXIMITY);
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_MEMBERS, 12);
              c.welcomeMessage = VillageBehaviour::DRAGON_WELCOME;));
    case EnemyId::GREEN_DRAGON:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.inhabitants.leader = CreatureId::RED_DRAGON;
            c.tribe = TribeId::getMonster();
            c.race = "dragon"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.shopFactory = ItemFactory::dragonCave();), CollectiveConfig::noImmigrants().setLeaderAsFighter()
          .setGhostSpawns(0.03, 1),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 0;
              c.minTeamSize = 1;
              c.triggers = LIST(
                  {AttackTriggerId::ENEMY_POPULATION, 18},
                  AttackTriggerId::STOLEN_ITEMS,
                  {AttackTriggerId::NUM_CONQUERED, 2},
                  AttackTriggerId::FINISH_OFF,
                  AttackTriggerId::PROXIMITY);
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_MEMBERS, 7);
              c.welcomeMessage = VillageBehaviour::DRAGON_WELCOME;));
    case EnemyId::DWARVES:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::MINETOWN;
            c.tribe = TribeId::getDwarf();
            c.inhabitants.leader = CreatureId::DWARF_BARON;
            c.inhabitants.fighters = make_pair(
                random.get(6, 9),
                makeVec(make_pair(1, CreatureId::DWARF)));
            c.inhabitants.civilians = make_pair(
                random.get(3, 5),
                makeVec(make_pair(2, CreatureId::DWARF_FEMALE), make_pair(1, CreatureId::RAT)));
            c.locationName = getVillageName();
            c.race = "dwarves"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.stockpiles = LIST({StockpileInfo::GOLD, 200}, {StockpileInfo::MINERALS, 120});
            c.shopFactory = ItemFactory::dwarfShop();
            c.outsideFeatures = FurnitureFactory::dungeonOutside(c.tribe);
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::withImmigrants(500, 15, {
              ImmigrantInfo(CreatureId::DWARF, {MinionTrait::FIGHTER}).setFrequency(1),
            }).setGhostSpawns(0.1, 4),
          CONSTRUCT(VillageBehaviour,
            c.minPopulation = 3;
            c.minTeamSize = 4;
            c.triggers = LIST(
                {AttackTriggerId::ROOM_BUILT, FurnitureType::THRONE},
                AttackTriggerId::SELF_VICTIMS,
                AttackTriggerId::STOLEN_ITEMS,
                {AttackTriggerId::ROOM_BUILT, FurnitureType::IMPALED_HEAD},
                {AttackTriggerId::NUM_CONQUERED, 3},
                AttackTriggerId::FINISH_OFF,
                AttackTriggerId::PROXIMITY);
            c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_MEMBERS, 3);
            c.ransom = make_pair(0.8, random.get(240, 320));));
    case EnemyId::ELVES:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::FORREST_VILLAGE;
            c.inhabitants.leader = CreatureId::ELF_LORD;
            c.inhabitants.fighters = make_pair(
                random.get(6, 9),
                makeVec(make_pair(1, CreatureId::ELF_ARCHER)));
            c.inhabitants.civilians = make_pair(
                random.get(6, 9),
                makeVec(make_pair(1, CreatureId::ELF), make_pair(1, CreatureId::ELF_CHILD),
                    make_pair(1, CreatureId::HORSE)));
            c.locationName = getVillageName();
            c.tribe = TribeId::getElf();
            c.race = "elves"_s;
            c.stockpiles = LIST({StockpileInfo::GOLD, 100});
            c.buildingId = BuildingId::WOOD;
            c.elderLoot = ItemType(ItemType::TechBook{TechId::SPELLS_MAS});
            c.furniture = FurnitureFactory::roomFurniture(TribeId::getPest());),
          CollectiveConfig::withImmigrants(500, 18, {
              ImmigrantInfo(CreatureId::ELF_ARCHER, {MinionTrait::FIGHTER}).setFrequency(1),
            }).setGhostSpawns(0.1, 4),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 4;
              c.minTeamSize = 4;
              c.triggers = LIST(AttackTriggerId::STOLEN_ITEMS);
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);));
    case EnemyId::ELEMENTALIST_ENTRY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::TOWER;
            c.tribe = TribeId::getHuman();
            c.buildingId = BuildingId::BRICK;),
          CollectiveConfig::noImmigrants())
          .setNonDiscoverable();
    case EnemyId::ELEMENTALIST:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::TOWER;
            c.inhabitants.leader = CreatureId::ELEMENTALIST;
            c.tribe = TribeId::getHuman();
            c.buildingId = BuildingId::BRICK;
            c.furniture = FurnitureFactory::roomFurniture(TribeId::getPest());),
          CollectiveConfig::noImmigrants().setLeaderAsFighter(),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 0;
              c.minTeamSize = 1;
              c.triggers = LIST(
                  {AttackTriggerId::ROOM_BUILT, FurnitureType::THRONE},
                  AttackTriggerId::PROXIMITY,
                  {AttackTriggerId::ROOM_BUILT, FurnitureType::IMPALED_HEAD},
                  AttackTriggerId::FINISH_OFF,
                  {AttackTriggerId::NUM_CONQUERED, 3});
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::CAMP_AND_SPAWN,
                CreatureFactory::elementals(TribeId::getHuman()));
              c.ransom = make_pair(0.5, random.get(40, 80));),
          LevelConnection{LevelConnection::TOWER, get(EnemyId::ELEMENTALIST_ENTRY)});
    case EnemyId::NO_AGGRO_BANDITS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.inhabitants.fighters = make_pair(
                random.get(4, 9),
                makeVec(make_pair(1, CreatureId::BANDIT)));
            c.tribe = TribeId::getBandit();
            c.race = "bandits"_s;
            c.buildingId = BuildingId::DUNGEON;),
          CollectiveConfig::withImmigrants(1000, 10, {
              ImmigrantInfo(CreatureId::BANDIT, {MinionTrait::FIGHTER}).setFrequency(1),
            }));
    case EnemyId::BANDITS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.inhabitants.fighters = make_pair(
                random.get(4, 9),
                makeVec(make_pair(1, CreatureId::BANDIT)));
            c.tribe = TribeId::getBandit();
            c.race = "bandits"_s;
            c.buildingId = BuildingId::DUNGEON;),
          CollectiveConfig::withImmigrants(1000, 10, {
              ImmigrantInfo(CreatureId::BANDIT, {MinionTrait::FIGHTER}).setFrequency(1),
            }),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 0;
              c.minTeamSize = 3;
              c.triggers = LIST({AttackTriggerId::GOLD, 100});
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::STEAL_GOLD);
              c.ransom = make_pair(0.5, random.get(40, 80));));
    case EnemyId::LIZARDMEN:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::VILLAGE;
            c.tribe = TribeId::getLizard();
            c.inhabitants.leader = CreatureId::LIZARDLORD;
            c.inhabitants.fighters = make_pair(
                random.get(7, 10),
                makeVec(make_pair(1, CreatureId::LIZARDMAN)));
            c.locationName = getVillageName();
            c.race = "lizardmen"_s;
            c.buildingId = BuildingId::MUD;
            c.elderLoot = ItemType(ItemType::TechBook{TechId::HUMANOID_MUT});
            c.shopFactory = ItemFactory::mushrooms();
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);
            c.outsideFeatures = FurnitureFactory::villageOutside(c.tribe);),
          CollectiveConfig::withImmigrants(140, 11, {
              ImmigrantInfo(CreatureId::LIZARDMAN, {MinionTrait::FIGHTER}).setFrequency(1),
            }).setGhostSpawns(0.1, 4),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 4;
              c.minTeamSize = 4;
              c.triggers = LIST(
                  AttackTriggerId::POWER,
                  AttackTriggerId::SELF_VICTIMS,
                  AttackTriggerId::STOLEN_ITEMS,
                  {AttackTriggerId::ROOM_BUILT, FurnitureType::IMPALED_HEAD},
                  {AttackTriggerId::NUM_CONQUERED, 2},
                  AttackTriggerId::FINISH_OFF,
                  AttackTriggerId::PROXIMITY);
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);));
    case EnemyId::DARK_ELVES:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::MINETOWN;
            c.tribe = TribeId::getDarkElf();
            c.inhabitants.leader = CreatureId::DARK_ELF_LORD;
            c.inhabitants.fighters = make_pair(
                random.get(6, 9),
                makeVec(make_pair(1, CreatureId::DARK_ELF_WARRIOR)));
            c.inhabitants.civilians = make_pair(
                random.get(6, 9),
                makeVec(make_pair(1, CreatureId::DARK_ELF), make_pair(1, CreatureId::DARK_ELF_CHILD),
                    make_pair(1, CreatureId::RAT)));
            c.locationName = getVillageName();
            c.race = "dark elves"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.outsideFeatures = FurnitureFactory::dungeonOutside(c.tribe);
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::withImmigrants(500, 15, {
              ImmigrantInfo(CreatureId::DARK_ELF_WARRIOR, {MinionTrait::FIGHTER}).setFrequency(1),
          }), none,
          LevelConnection{LevelConnection::GNOMISH_MINES, get(EnemyId::DARK_ELVES_ENTRY)});
    case EnemyId::DARK_ELVES_ENTRY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SMALL_MINETOWN;
            c.tribe = TribeId::getDarkElf();
            c.inhabitants.fighters = make_pair(
                random.get(3, 7),
                makeVec(make_pair(1, CreatureId::DARK_ELF_WARRIOR)));
            c.race = "dark elves"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.outsideFeatures = FurnitureFactory::dungeonOutside(c.tribe);
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::noImmigrants(), {})
          .setNonDiscoverable();
    case EnemyId::GNOMES:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::MINETOWN;
            c.tribe = TribeId::getGnome();
            c.inhabitants.leader = CreatureId::GNOME_CHIEF;
            c.inhabitants.fighters = make_pair(
                random.get(8, 14),
                makeVec(make_pair(1, CreatureId::GNOME)));
            c.locationName = getVillageName();
            c.race = "gnomes"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.shopFactory = ItemFactory::gnomeShop();
            c.outsideFeatures = FurnitureFactory::dungeonOutside(c.tribe);
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::noImmigrants(), none,
          LevelConnection{LevelConnection::GNOMISH_MINES, get(EnemyId::GNOMES_ENTRY)});
    case EnemyId::GNOMES_ENTRY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SMALL_MINETOWN;
            c.tribe = TribeId::getGnome();
            c.inhabitants.fighters = make_pair(
                random.get(3, 7),
                makeVec(make_pair(1, CreatureId::GNOME)));
            c.race = "gnomes"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.outsideFeatures = FurnitureFactory::dungeonOutside(c.tribe);
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::noImmigrants())
          .setNonDiscoverable();
    case EnemyId::ENTS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::FOREST;
            c.inhabitants.fighters = make_pair(
                random.get(7, 10),
                makeVec(make_pair(1, CreatureId::ENT)));
            c.tribe = TribeId::getMonster();
            c.race = "tree spirits"_s;
            c.buildingId = BuildingId::WOOD;),
          CollectiveConfig::withImmigrants(300, 10, {
              ImmigrantInfo(CreatureId::ENT, {MinionTrait::FIGHTER}).setFrequency(1),
          }));
    case EnemyId::DRIADS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::FOREST;
            c.inhabitants.fighters = make_pair(
                random.get(7, 13),
                makeVec(make_pair(1, CreatureId::DRIAD)));
            c.tribe = TribeId::getMonster();
            c.race = "driads"_s;
            c.buildingId = BuildingId::WOOD;),
          CollectiveConfig::withImmigrants(300, 15, {
              ImmigrantInfo(CreatureId::DRIAD, {MinionTrait::FIGHTER}).setFrequency(1),
          }));
    case EnemyId::SHELOB:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.tribe = TribeId::getShelob();
            c.inhabitants.leader = CreatureId::SHELOB;
            c.race = "giant spider"_s;
            c.buildingId = BuildingId::DUNGEON;
            ), CollectiveConfig::noImmigrants().setLeaderAsFighter());
    case EnemyId::CYCLOPS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.inhabitants.leader = CreatureId::CYCLOPS;
            c.race = "cyclops"_s;
            c.tribe = TribeId::getHostile();
            c.buildingId = BuildingId::DUNGEON;
            c.shopFactory = ItemFactory::mushrooms(true);), CollectiveConfig::noImmigrants().setLeaderAsFighter()
          .setGhostSpawns(0.03, 1),
          CONSTRUCT(VillageBehaviour,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.triggers = LIST(
                {AttackTriggerId::ENEMY_POPULATION, 13},
                AttackTriggerId::PROXIMITY);
            c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_MEMBERS, 4);));
    case EnemyId::HYDRA:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SWAMP;
            c.inhabitants.leader = CreatureId::HYDRA;
            c.race = "hydra"_s;
            c.tribe = TribeId::getHostile();), CollectiveConfig::noImmigrants().setLeaderAsFighter()
          .setGhostSpawns(0.03, 1));
    case EnemyId::KRAKEN:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::MOUNTAIN_LAKE;
            c.inhabitants.leader = CreatureId::KRAKEN;
            c.race = "kraken"_s;
            c.tribe = TribeId::getMonster();), CollectiveConfig::noImmigrants().setLeaderAsFighter())
          .setNonDiscoverable();
    case EnemyId::CEMETERY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CEMETERY;
            c.inhabitants.fighters = make_pair(
                random.get(8, 12),
                makeVec(make_pair(1, CreatureId::ZOMBIE)));
            c.locationName = "cemetery"_s;
            c.tribe = TribeId::getMonster();
            c.race = "undead"_s;
            c.furniture = FurnitureFactory::cryptCoffins(TribeId::getKeeper());
            c.buildingId = BuildingId::BRICK;), CollectiveConfig::noImmigrants(), {},
          LevelConnection{LevelConnection::CRYPT, get(EnemyId::CEMETERY_ENTRY)});
    case EnemyId::CEMETERY_ENTRY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CEMETERY;
            c.inhabitants.fighters = make_pair(
                1,
                makeVec(make_pair(1, CreatureId::ZOMBIE)));
            c.locationName = "cemetery"_s;
            c.race = "undead"_s;
            c.tribe = TribeId::getMonster();
            c.buildingId = BuildingId::BRICK;), CollectiveConfig::noImmigrants())
          .setNonDiscoverable();
    case EnemyId::OGRE_CAVE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.tribe = TribeId::getGreenskin();
            c.inhabitants.fighters = make_pair(
                random.get(4, 8),
                makeVec(make_pair(1, CreatureId::OGRE)));
            c.buildingId = BuildingId::DUNGEON;
            c.closeToPlayer = true;
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);
            c.outsideFeatures = FurnitureFactory::villageOutside(c.tribe);),
          CollectiveConfig::withImmigrants(300, 10, {
              ImmigrantInfo(CreatureId::OGRE, {MinionTrait::FIGHTER}).setFrequency(1),
          }));
    case EnemyId::HARPY_CAVE: {
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.tribe = TribeId::getGreenskin();
            c.inhabitants.fighters = make_pair(
                random.get(4, 8),
                makeVec(make_pair(1, CreatureId::HARPY)));
            c.buildingId = BuildingId::DUNGEON;
            c.closeToPlayer = true;
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);
            c.outsideFeatures = FurnitureFactory::villageOutside(c.tribe);),
          CollectiveConfig::withImmigrants(300, 10, {
              ImmigrantInfo(CreatureId::HARPY, {MinionTrait::FIGHTER}).setFrequency(1),
          }));
      }
    case EnemyId::SOKOBAN_ENTRY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::ISLAND_VAULT_DOOR;
            c.buildingId = BuildingId::DUNGEON;
            ), CollectiveConfig::noImmigrants());
    case EnemyId::SOKOBAN:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::ISLAND_VAULT;
            c.inhabitants.leader = random.choose(
                CreatureId::SPECIAL_HLBN, CreatureId::SPECIAL_HLBW, CreatureId::SPECIAL_HLGN, CreatureId::SPECIAL_HLGW);
            c.tribe = TribeId::getKeeper();
            c.buildingId = BuildingId::DUNGEON;
            ), CollectiveConfig::noImmigrants(), none,
          LevelConnection{LevelConnection::SOKOBAN, get(EnemyId::SOKOBAN_ENTRY)})
          .setNonDiscoverable();
    case EnemyId::WITCH:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::WITCH_HOUSE;
            c.tribe = TribeId::getMonster();
            c.inhabitants.leader = CreatureId::WITCH;
            c.race = "witch"_s;
            c.buildingId = BuildingId::WOOD;
            c.elderLoot = ItemType(ItemType::TechBook{TechId::ALCHEMY_ADV});
            c.furniture = FurnitureFactory(c.tribe, FurnitureType::LABORATORY);), CollectiveConfig::noImmigrants());
    case EnemyId::HUMAN_COTTAGE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::COTTAGE;
            c.tribe = TribeId::getHuman();
            c.inhabitants.leader = random.choose(CreatureId::KNIGHT, CreatureId::ARCHER);
            c.inhabitants.fighters = make_pair(
                random.get(2, 4),
                makeVec(make_pair(1, CreatureId::PESEANT)));
            c.inhabitants.civilians = make_pair(
                random.get(3, 7),
                makeVec(
                    make_pair(1, CreatureId::CHILD), make_pair(1, CreatureId::HORSE),
                    make_pair(1, CreatureId::DONKEY), make_pair(1, CreatureId::COW),
                    make_pair(1, CreatureId::PIG),make_pair(1, CreatureId::DOG)));
            c.race = "humans"_s;
            c.buildingId = BuildingId::WOOD;
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::noImmigrants().setGuardian({CreatureId::WITCHMAN, 0.001, 1, 2}));
    case EnemyId::TUTORIAL_VILLAGE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SMALL_VILLAGE;
            c.tribe = TribeId::getHuman();
            c.inhabitants.fighters = make_pair(
                random.get(2, 4),
                makeVec(make_pair(1, CreatureId::PESEANT)));
            c.inhabitants.civilians = make_pair(
                random.get(3, 7),
                makeVec(
                    make_pair(1, CreatureId::CHILD), make_pair(1, CreatureId::HORSE),
                    make_pair(1, CreatureId::DONKEY), make_pair(1, CreatureId::COW),
                    make_pair(1, CreatureId::PIG),make_pair(1, CreatureId::DOG)));
            c.race = "humans"_s;
            c.buildingId = BuildingId::WOOD;
            c.stockpiles = LIST({StockpileInfo::GOLD, 50});
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::noImmigrants());
    case EnemyId::ELVEN_COTTAGE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::FORREST_COTTAGE;
            c.tribe = TribeId::getElf();
            c.inhabitants.leader = CreatureId::ELF_ARCHER;
            c.inhabitants.fighters = make_pair(
                random.get(2, 3),
                makeVec(make_pair(1, CreatureId::ELF)));
            c.inhabitants.civilians = make_pair(
                random.get(2, 5),
                makeVec(
                    make_pair(1, CreatureId::ELF_CHILD), make_pair(1, CreatureId::HORSE),
                    make_pair(1, CreatureId::COW), make_pair(1, CreatureId::DOG)));
            c.race = "elves"_s;
            c.buildingId = BuildingId::WOOD;
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::noImmigrants());
    case EnemyId::KOBOLD_CAVE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SMALL_MINETOWN;
            c.tribe = TribeId::getDwarf();
            c.inhabitants.fighters = make_pair(
                random.get(3, 7),
                makeVec(make_pair(1, CreatureId::KOBOLD)));
            c.race = "kobolds"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.stockpiles = LIST({StockpileInfo::MINERALS, 60});),
          CollectiveConfig::noImmigrants());
    case EnemyId::DWARF_CAVE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SMALL_MINETOWN;
            c.tribe = TribeId::getDwarf();
            c.inhabitants.fighters = make_pair(
                random.get(2, 5),
                makeVec(make_pair(1, CreatureId::DWARF)));
            c.inhabitants.civilians = make_pair(
                random.get(2, 5),
                makeVec(make_pair(1, CreatureId::DWARF_FEMALE)));
            c.race = "dwarves"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.stockpiles = LIST(random.choose(StockpileInfo{StockpileInfo::MINERALS, 60},
                StockpileInfo{StockpileInfo::GOLD, 60}));
            c.outsideFeatures = FurnitureFactory::dungeonOutside(c.tribe);
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::noImmigrants(),
          CONSTRUCT(VillageBehaviour,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.triggers = LIST(AttackTriggerId::SELF_VICTIMS, AttackTriggerId::STOLEN_ITEMS);
            c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);
            c.ransom = make_pair(0.5, random.get(40, 80));));
  }
}

vector<ExternalEnemy> EnemyFactory::getExternalEnemies() {
  return {
    ExternalEnemy{
        CreatureFactory::singleCreature(TribeId::getHuman(), CreatureId::BANDIT),
        Range(1,2),
        AttackBehaviourId::KILL_LEADER,
        "a curious bandit",
        Range(1000, 10000),
        1
    },
    ExternalEnemy{
        CreatureFactory::singleCreature(TribeId::getHuman(), CreatureId::LIZARDMAN),
        Range(1,2),
        AttackBehaviourId::KILL_LEADER,
        "a lizardman scout",
        Range(1000, 10000),
        1
    },
    ExternalEnemy{
        CreatureFactory::singleCreature(TribeId::getHuman(), CreatureId::ANT_WORKER),
        Range(3,5),
        AttackBehaviourId::KILL_LEADER,
        "an ant patrol",
        Range(0, 5000),
        3
    },
    ExternalEnemy{
        CreatureFactory::singleCreature(TribeId::getHuman(), CreatureId::ANT_SOLDIER),
        Range(3,5),
        AttackBehaviourId::KILL_LEADER,
        "an ant soldier patrol",
        Range(1000, 10000),
        3
    },
    ExternalEnemy{
        CreatureFactory::antNest(TribeId::getHuman()),
        Range(10,15),
        AttackBehaviourId::KILL_LEADER,
        "an army of ants",
        Range(0, 15000),
        3
    },
    ExternalEnemy{
        CreatureFactory::singleCreature(TribeId::getHuman(), CreatureId::ENT),
        Range(4,8),
        AttackBehaviourId::KILL_LEADER,
        "a group of ents",
        Range(7000, 12000),
        3
    },
    ExternalEnemy{
        CreatureFactory::singleCreature(TribeId::getHuman(), CreatureId::DWARF),
        Range(4,8),
        AttackBehaviourId::KILL_LEADER,
        "a band of dwarves",
        Range(7000, 12000),
        3
    },
    ExternalEnemy{
        CreatureFactory::singleCreature(TribeId::getHuman(), CreatureId::LIZARDMAN),
        Range(4,8),
        AttackBehaviourId::KILL_LEADER,
        "a nest of lizardmen",
        Range(7000, 12000),
        3
    },
    ExternalEnemy{
        CreatureFactory::dwarfTown(TribeId::getHuman()),
        Range(10,13),
        AttackBehaviourId::KILL_LEADER,
        "a dwarf army",
        Range(0, 18000),
        1
    },
    ExternalEnemy{
        CreatureFactory::singleCreature(TribeId::getHuman(), CreatureId::ARCHER),
        Range(4,8),
        AttackBehaviourId::KILL_LEADER,
        "a patrol of archers",
        Range(0, 12000),
        3
    },
    ExternalEnemy{
        CreatureFactory::singleCreature(TribeId::getHuman(), CreatureId::KNIGHT),
        Range(4,8),
        AttackBehaviourId::KILL_LEADER,
        "a group of knights",
        Range(0, 12000),
        3
    },
    ExternalEnemy{
        CreatureFactory::singleCreature(TribeId::getHuman(), CreatureId::WARRIOR),
        Range(4,8),
        AttackBehaviourId::KILL_LEADER,
        "a group of warriors",
        Range(4000, 6000),
        3
    },
    ExternalEnemy{
        CreatureFactory::vikingTown(TribeId::getHuman()),
        Range(12, 17),
        AttackBehaviourId::KILL_LEADER,
        "an army of warriors",
        Range(0, 12000),
        3
    },
    ExternalEnemy{
        CreatureFactory::humanCastle(TribeId::getHuman()),
        Range(12,17),
        AttackBehaviourId::KILL_LEADER,
        "an army of knights",
        Range(0, 30000),
        3
    },
    ExternalEnemy{
        CreatureFactory::singleCreature(TribeId::getHuman(), CreatureId::CYCLOPS),
        Range(1,2),
        AttackBehaviourId::KILL_LEADER,
        "a cyclops",
        Range(6000, 12000),
        1
    },
    ExternalEnemy{
        CreatureFactory::singleCreature(TribeId::getHuman(), CreatureId::WITCHMAN),
        Range(1,2),
        AttackBehaviourId::KILL_LEADER,
        "a witchman",
        Range(6000, 12000),
        1
    },
    ExternalEnemy{
        CreatureFactory::singleCreature(TribeId::getHuman(), CreatureId::MINOTAUR),
        Range(1,2),
        AttackBehaviourId::KILL_LEADER,
        "a minotaur",
        Range(10000, 30000),
        1
    },
    ExternalEnemy{
        CreatureFactory::singleCreature(TribeId::getHuman(), CreatureId::ELEMENTALIST),
        Range(1,2),
        {AttackBehaviourId::CAMP_AND_SPAWN, CreatureFactory::elementals(TribeId::getHuman())},
        "an elementalist",
        Range(15000, 30000),
        1
    },
    ExternalEnemy{
        CreatureFactory::singleCreature(TribeId::getHuman(), CreatureId::GREEN_DRAGON),
        Range(1,2),
        AttackBehaviourId::KILL_LEADER,
        "a green dragon",
        Range(10000, 30000),
        1
    },
    ExternalEnemy{
        CreatureFactory::singleCreature(TribeId::getHuman(), CreatureId::RED_DRAGON),
        Range(1,2),
        AttackBehaviourId::KILL_LEADER,
        "a red dragon",
        Range(15000, 40000),
        1
    },
  };
}
