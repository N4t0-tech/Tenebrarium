#include "Player.hpp"
#include "SkillTree.hpp"
#include <algorithm>

Player::Player(const std::string& name, PlayerClass playerClass)
    : Entity(name, baseHp(playerClass), baseAttack(playerClass), baseDefense(playerClass)),
      class_(playerClass),
      level_(1),
      xp_(0),
      xpToNextLevel_(100),
      mana_(baseMana(playerClass)),
      maxMana_(baseMana(playerClass)),
      coins_(0),
      dungeonFloor_(1),
      baseAttack_(baseAttack(playerClass)),
      baseDefense_(baseDefense(playerClass)),
      inventory_(20)
{
    resetEquippedArts();
}

void Player::gainXp(int amount) {
    xp_ += amount;
    while (xp_ >= xpToNextLevel_) {
        xp_ -= xpToNextLevel_;
        levelUp();
    }
}

void Player::levelUp() {
    level_++;
    xpToNextLevel_ = std::min(999999, static_cast<int>(xpToNextLevel_ * 1.5));  // umbral crece x1.5 cada nivel
    maxHp_ += 10;
    hp_ = maxHp_;   // curación completa al subir nivel
    baseAttack_  += 2;
    baseDefense_ += 1;
    // Recalcular ATK/DEF efectivo incluyendo el bonus del equipo actual
    attack_  = baseAttack_  + (equippedWeapon_ ? equippedWeapon_->statBonus : 0);
    defense_ = baseDefense_ + (equippedArmor_  ? equippedArmor_->statBonus  : 0);
    maxMana_ += 5;
    mana_ = maxMana_;
    // Desbloqueo de habilidad cada 5 niveles
    refreshPendingSkillUnlock();
}

void Player::refreshPendingSkillUnlock() {
    // Un tier por cada 5 niveles; si se sube varios niveles de golpe quedan
    // pendientes todos los tiers ganados y el árbol se abre una vez por cada uno.
    int earned = std::min(level_ / 5, static_cast<int>(getSkillTree(class_).tiers.size()));
    pendingSkillUnlock_ = static_cast<int>(unlockedSkills_.size()) < earned;
}

int Player::baseHp(PlayerClass c) {
    switch (c) {
        case PlayerClass::Warrior:    return 120;
        case PlayerClass::Mage:       return 100;
        case PlayerClass::Ranger:     return 100;
        case PlayerClass::Halley:     return 130;
        case PlayerClass::Nato:       return 140;
    }
    return 100;
}

int Player::baseAttack(PlayerClass c) {
    switch (c) {
        case PlayerClass::Warrior:    return 20;
        case PlayerClass::Mage:       return 10;
        case PlayerClass::Ranger:     return 12;
        case PlayerClass::Halley:     return 18;
        case PlayerClass::Nato:       return 22;
    }
    return 10;
}

int Player::baseDefense(PlayerClass c) {
    switch (c) {
        case PlayerClass::Warrior:    return 8;
        case PlayerClass::Mage:       return 3;
        case PlayerClass::Ranger:     return 5;
        case PlayerClass::Halley:     return 7;
        case PlayerClass::Nato:       return 8;
    }
    return 5;
}

void Player::addCoins(int amount)  { coins_ += amount; }

void Player::descendFloor()        { dungeonFloor_++; }

void Player::beginIncursion() {
    dungeonFloor_ = 1;
    incursions_++;
}

void Player::trainHealth(int amount)  { maxHp_ += amount; }
void Player::trainAttack(int amount) {
    baseAttack_ += amount;
    attack_ = baseAttack_ + (equippedWeapon_ ? equippedWeapon_->statBonus : 0);
}
void Player::trainDefense(int amount) {
    baseDefense_ += amount;
    defense_ = baseDefense_ + (equippedArmor_  ? equippedArmor_->statBonus  : 0);
}
void Player::trainMana(int amount)     { maxMana_ += amount; }

bool Player::upgradeEquippedWeapon() {
    if (!equippedWeapon_) return false;
    equippedWeapon_->statBonus++;
    attack_ = baseAttack_ + equippedWeapon_->statBonus;
    return true;
}

bool Player::upgradeEquippedArmor() {
    if (!equippedArmor_) return false;
    equippedArmor_->statBonus++;
    defense_ = baseDefense_ + equippedArmor_->statBonus;
    return true;
}



bool Player::pickupItem(const Item& item) {
    if (item.type == ItemType::Weapon && !equippedWeapon_) {
        equippedWeapon_ = item;
        attack_ = baseAttack_ + item.statBonus;
        return true;
    }
    if (item.type == ItemType::Armor && !equippedArmor_) {
        equippedArmor_ = item;
        defense_ = baseDefense_ + item.statBonus;
        return true;
    }
    return inventory_.addItem(item);
}

void Player::equipItem(int idx) {
    const auto& bag = inventory_.items();
    if (idx < 0 || idx >= static_cast<int>(bag.size())) return;
    Item chosen = bag[idx];  // copy before mutating
    if (chosen.type == ItemType::Weapon) {
        inventory_.removeItem(chosen.name);
        if (equippedWeapon_) inventory_.addItem(*equippedWeapon_);
        equippedWeapon_ = chosen;
        attack_ = baseAttack_ + chosen.statBonus;
    } else if (chosen.type == ItemType::Armor) {
        inventory_.removeItem(chosen.name);
        if (equippedArmor_) inventory_.addItem(*equippedArmor_);
        equippedArmor_ = chosen;
        defense_ = baseDefense_ + chosen.statBonus;
    }
}

void Player::unequipWeapon() {
    if (!equippedWeapon_) return;
    inventory_.addItem(*equippedWeapon_);
    equippedWeapon_.reset();
    attack_ = baseAttack_;
}

void Player::unequipArmor() {
    if (!equippedArmor_) return;
    inventory_.addItem(*equippedArmor_);
    equippedArmor_.reset();
    defense_ = baseDefense_;
}

int Player::useConsumable() {
    for (const auto& item : inventory_.items()) {
        // statBonus > 0 filtra cerveza/poción de mana (restauran mana, no HP)
        if (item.type == ItemType::Consumable && item.statBonus > 0) {
            int healed = std::min(item.statBonus, maxHp_ - hp_);
            if (healed <= 0) return 0;
            hp_ += healed;
            inventory_.removeItem(item.name);
            return healed;
        }
    }
    return 0;
}

int Player::countConsumables() const {
    int count = 0;
    for (const auto& item : inventory_.items())
        if (item.type == ItemType::Consumable) count += item.quantity;
    return count;
}

int Player::countHpPotions() const {
    int count = 0;
    for (const auto& item : inventory_.items())
        if (item.type == ItemType::Consumable && item.statBonus > 0)
            count += item.quantity;
    return count;
}

int Player::countManaPotions() const {
    int count = 0;
    for (const auto& item : inventory_.items())
        if (item.type == ItemType::Consumable && item.statBonus == 0)
            count += item.quantity;
    return count;
}

bool Player::useMana(int amount) {
    if (mana_ < amount) return false;
    mana_ -= amount;
    return true;
}

void Player::restoreMana(int amount) {
    mana_ = std::min(maxMana_, mana_ + amount);
}

Art Player::makeArt(const SkillChoice& sc) const {
    return { sc.name, sc.apCost, artManaCost(sc.effect), sc.effect, sc.desc, getArtRank(sc.effect) };
}

int Player::getArtRank(ArtEffect effect) const {
    int i = static_cast<int>(effect);
    return (i >= 0 && i < static_cast<int>(artRanks_.size())) ? artRanks_[i] : 1;
}

bool Player::upgradeArt(ArtEffect effect) {
    int i = static_cast<int>(effect);
    if (i < 0 || i >= static_cast<int>(artRanks_.size()) || artRanks_[i] >= kMaxArtRank) return false;
    artRanks_[i]++;
    return true;
}

int Player::artManaCost(ArtEffect effect) const {
    const SkillChoice* sc = findClassSkill(class_, effect);
    if (!sc) return 0;
    return getArtRank(effect) >= 3 ? sc->manaCost * 75 / 100 : sc->manaCost;
}

std::vector<Art> Player::getAvailableArts() const {
    std::vector<Art> arts;
    for (ArtEffect e : equippedArts_)
        if (const SkillChoice* sc = findClassSkill(class_, e))
            arts.push_back(makeArt(*sc));
    return arts;
}

std::vector<Art> Player::getKnownArts() const {
    std::vector<Art> arts;
    for (const auto& sc : getBaseSkills(class_))
        arts.push_back(makeArt(sc));
    const SkillTreeDef& tree = getSkillTree(class_);
    for (const auto& us : unlockedSkills_) {
        if (us.tier < 0 || us.tier >= static_cast<int>(tree.tiers.size())) continue;
        const SkillTier& tier = tree.tiers[us.tier];
        if (us.choiceIdx >= 0 && us.choiceIdx < static_cast<int>(tier.choices.size()))
            arts.push_back(makeArt(tier.choices[us.choiceIdx]));
    }
    return arts;
}

bool Player::knowsArt(ArtEffect effect) const {
    for (const auto& a : getKnownArts())
        if (a.effect == effect) return true;
    return false;
}

bool Player::hasEquippedArt(ArtEffect effect) const {
    return std::find(equippedArts_.begin(), equippedArts_.end(), effect) != equippedArts_.end();
}

bool Player::equipArt(int slot, ArtEffect effect) {
    if (slot < 0 || slot >= kEquippedArtSlots || !knowsArt(effect)) return false;
    if (slot >= static_cast<int>(equippedArts_.size())) equippedArts_.resize(slot + 1, effect);
    // Si ya estaba en otra ranura, las dos se intercambian
    auto it = std::find(equippedArts_.begin(), equippedArts_.end(), effect);
    if (it != equippedArts_.end()) *it = equippedArts_[slot];
    equippedArts_[slot] = effect;
    return true;
}

void Player::resetEquippedArts() {
    equippedArts_.clear();
    for (const auto& sc : getBaseSkills(class_))
        equippedArts_.push_back(sc.effect);
}

bool Player::unlockSkill(int tier, int choiceIdx) {
    // Verificar que el tier no esté ya desbloqueado
    for (const auto& us : unlockedSkills_) {
        if (us.tier == tier) return false;
    }
    const SkillTreeDef& tree = getSkillTree(class_);
    if (tier < 0 || tier >= static_cast<int>(tree.tiers.size())) return false;
    if (choiceIdx < 0 || choiceIdx >= static_cast<int>(tree.tiers[tier].choices.size())) return false;
    unlockedSkills_.push_back({ tier, choiceIdx });
    refreshPendingSkillUnlock();
    return true;
}

int Player::baseMana(PlayerClass c) {
    switch (c) {
        case PlayerClass::Warrior:    return 100;
        case PlayerClass::Mage:       return 100;
        case PlayerClass::Ranger:     return 50;
        case PlayerClass::Halley:     return 170;
        case PlayerClass::Nato:       return 130;
    }
    return 30;
}
