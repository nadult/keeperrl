#pragma once

#include "util.h"

RICH_ENUM(FXVariantName,
  BLIND,
  SPEED,
  SLEEP,
  FLYING,
  FIRE_SPHERE,
  SPIRAL_BLUE,
  SPIRAL_GREEN,
  DEBUFF_RED,
  DEBUFF_GREEN,
  DEBUFF_GRAY,
  DEBUFF_PINK,
  DEBUFF_BLACK,
  DEBUFF_WHITE,
  DEBUFF_ORANGE,
  LABORATORY_GREEN,
  LABORATORY_BLUE,
  LABORATORY_RED,
  LABORATORY_BLACK,
  FORGE_ORANGE
);

struct FXDef;

extern FXDef getDef(FXVariantName);