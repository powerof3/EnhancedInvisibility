#pragma once

class Settings : public REX::Singleton<Settings>
{
public:
	enum class DoNotDispel : std::uint32_t
	{
		kDisabled = 0,
		kOnActivate,
		kOnAll
	};

	enum class DetectionState : std::uint32_t
	{
		kDisabled = 0,
		kOnlyPlayer,
		kEveryone
	};

	void LoadSettings();

	[[nodiscard]] bool GetAllowRefractionFix() const;
	[[nodiscard]] bool GetAllowRefractBlood() const;
	[[nodiscard]] bool GetAllowRefractArrows() const;
	[[nodiscard]] bool GetAllowAlphaBlendFix() const;

	[[nodiscard]] const DoNotDispel GetInvisState() const;
	[[nodiscard]] const DoNotDispel GetEtherealState() const;

	[[nodiscard]] const DetectionState GetInvisDetection() const;
	[[nodiscard]] const DetectionState GetEtherealDetection() const;

	[[nodiscard]] bool ShouldMakeSuperInvisible(RE::Actor* a_target) const;

private:
	struct
	{
		bool fixRefraction{ true };
		bool refractBlood{ true };
		bool refractAttachedArrows{ true };
		bool fixArmorAlphaBlend{ true };

		DoNotDispel    state{ DoNotDispel::kOnActivate };
		DetectionState detectState{ DetectionState::kDisabled };

	} invisibility;

	struct
	{
		DoNotDispel    state{ DoNotDispel::kOnActivate };
		DetectionState detectState{ DetectionState::kDisabled };
	} etherealForm;
};
