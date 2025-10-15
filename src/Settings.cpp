#include "Settings.h"

void Settings::LoadSettings()
{
	constexpr auto path = L"Data/SKSE/Plugins/po3_EnhancedInvisibility.ini";

	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(path);

	ini::get_value(ini, invisibility.fixRefraction, "Invisibility", "bRefractionShaderFix", ";Fixes a refraction shader bug where refraction wasn't affected by normal maps");
	ini::get_value(ini, invisibility.refractBlood, "Invisibility", "bDynamicBloodRefraction", ";Blood splatter on an invisible character will turn invisible");
	ini::get_value(ini, invisibility.refractAttachedArrows, "Invisibility", "bDynamicAttachedArrowRefraction", ";Arrows hitting an invisible character will turn invisible");
	ini::get_value(ini, invisibility.fixArmorAlphaBlend, "Invisibility", "bArmorAlphaBlendFix", ";Hide alpha blended armor meshes with BSEffectShaderProperty as they will not refract");

	ini::get_value(ini, invisibility.state, "Invisibility", "iUninterruptedActions", ";Invisibility will not break when performing these actions\n;0 - disabled, 1 - activation, 2 - everything (attack, spell cast, activate)");
	ini::get_value(ini, invisibility.detectState, "Invisibility", "iMakeUndetectable", ";NPCs will never detect an invisible player/NPC\n; 0 - disabled, 1 - only player, 2 - player and NPCs");

	ini::get_value(ini, etherealForm.state, "Ethereal Form", "iUninterruptedActions", ";Ethereal form  will not break when performing these actions\n;0 - disabled, 1 - activation, 2 - everything (attack, spell cast, activate)");
	ini::get_value(ini, etherealForm.detectState, "Ethereal Form", "iMakeUndetectable", ";NPCs will never detect an ethereal player/NPC\n; 0 - disabled, 1 - only player, 2 - player and NPCs");

	ini.SaveFile(path);
}

bool Settings::GetAllowRefractionFix() const
{
	return invisibility.fixRefraction;
}

bool Settings::GetAllowRefractBlood() const
{
	return invisibility.refractBlood;
}

bool Settings::GetAllowRefractArrows() const
{
	return invisibility.refractAttachedArrows;
}

bool Settings::GetAllowAlphaBlendFix() const
{
	return invisibility.fixArmorAlphaBlend;
}

const Settings::DoNotDispel Settings::GetInvisState() const
{
	return invisibility.state;
}

const Settings::DoNotDispel Settings::GetEtherealState() const
{
	return etherealForm.state;
}

const Settings::DetectionState Settings::GetInvisDetection() const
{
	return invisibility.detectState;
}

const Settings::DetectionState Settings::GetEtherealDetection() const
{
	return etherealForm.detectState;
}

bool Settings::ShouldMakeSuperInvisible(RE::Actor* a_target) const
{
	constexpr auto hasEffectWithArchetype = [](RE::Actor* a_target, RE::EffectArchetype a_type)
	{
		auto effects = a_target->GetActiveEffectList();
		if (!effects) {
			return false;
		}

		RE::EffectSetting* setting = nullptr;
		for (auto& effect : *effects) {
			setting = effect ? effect->GetBaseObject() : nullptr;
			if (setting && setting->HasArchetype(a_type) && effect->flags.none(RE::ActiveEffect::Flag::kInactive) && effect->flags.none(RE::ActiveEffect::Flag::kDispelled)) {
				return true;
			}
		}
		return false;
	};

	if (hasEffectWithArchetype(a_target, RE::EffectArchetype::kInvisibility)) {
		const auto state = GetInvisDetection();
		return state == DetectionState::kEveryone || state == DetectionState::kOnlyPlayer && a_target->IsPlayerRef();
	}
	if (hasEffectWithArchetype(a_target, RE::EffectArchetype::kEtherealize)) {
		const auto state = GetEtherealDetection();
		return state == DetectionState::kEveryone || state == DetectionState::kOnlyPlayer && a_target->IsPlayerRef();
	}

	return false;
}
