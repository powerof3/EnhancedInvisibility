#include "Settings.h"

Settings* Settings::GetSingleton()
{
	static Settings singleton;
	return std::addressof(singleton);
}

void Settings::LoadSettings()
{
	constexpr auto path = L"Data/SKSE/Plugins/po3_EnhancedInvisibility.ini";

	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(path);

	detail::get_value(ini, invisibility.fixRefraction, "Invisibility", "Refraction Shader Fix", ";Fixes a refraction shader bug where refraction wasn't affected by normal maps");
	detail::get_value(ini, invisibility.refractBlood, "Invisibility", "Dynamic Blood Refraction", ";Blood splatter on an invisible character will turn invisible");
	detail::get_value(ini, invisibility.refractAttachedArrows, "Invisibility", "Dynamic Attached Arrow Refraction", ";Arrows hitting an invisible character will turn invisible");
	detail::get_value(ini, invisibility.fixArmorAlphaBlend, "Invisibility", "Armor Alpha Blend Fix", ";Hide alpha blended armor meshes with BSEffectShaderProperty as they will not refract");

	detail::get_value(ini, invisibility.state, "Invisibility", "Uninterrupted Actions", ";Invisibility will not break when performing these actions\n;0 - disabled, 1 - activation, 2 - everything (attack, spell cast, activate)");
	detail::get_value(ini, invisibility.detectState, "Invisibility", "Make Undetectable", ";NPCs will never detect an invisible player/NPC\n; 0 - disabled, 1 - only player, 2 - player and NPCs");

	detail::get_value(ini, etherealForm.state, "Ethereal Form", "Uninterrupted Actions", ";Ethereal form  will not break when performing these actions\n;0 - disabled, 1 - activation, 2 - everything (attack, spell cast, activate)");
	detail::get_value(ini, etherealForm.detectState, "Ethereal Form", "Make Undetectable", ";NPCs will never detect an ethereal player/NPC\n; 0 - disabled, 1 - only player, 2 - player and NPCs");

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

const Settings::DoNotDispel& Settings::GetInvisState() const
{
	return invisibility.state;
}

const Settings::DoNotDispel& Settings::GetEtherealState() const
{
	return etherealForm.state;
}

const Settings::Detection& Settings::GetInvisDetection() const
{
	return invisibility.detectState;
}

const Settings::Detection& Settings::GetEtherealDetection() const
{
	return etherealForm.detectState;
}
