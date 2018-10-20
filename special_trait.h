#pragma once

#include "util.h"
#include "experience_type.h"

struct ExtraTraining {
  ExperienceType HASH(type);
  int HASH(increase);
  COMPARE_ALL(type, increase)
  HASH_ALL(type, increase)
};

struct AttrBonus {
  AttrType HASH(attr);
  int HASH(increase);
  COMPARE_ALL(attr, increase)
  HASH_ALL(attr, increase)
};

class SpecialTrait;

struct OneOfTraits {
  vector<SpecialTrait> HASH(traits);
  COMPARE_ALL(traits)
  HASH_ALL(traits)
};

class SpecialTrait : public variant<ExtraTraining, LastingEffect, SkillId, AttrBonus, OneOfTraits> {
  using variant::variant;
};

extern void applySpecialTrait(SpecialTrait, WCreature);
extern SpecialTrait transformBeforeApplying(SpecialTrait);
