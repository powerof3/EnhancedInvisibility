#pragma once

class Settings
{
public:
	enum class DoNotDispel : std::uint32_t
	{
		kDisabled = 0,
		kOnActivate,
		kOnAll
	};

	enum class Detection : std::uint32_t
	{
		kDisabled = 0,
		kOnlyPlayer,
		kEveryone
	};

	[[nodiscard]] static Settings* GetSingleton();
	[[nodiscard]] bool LoadSettings();

	bool GetAllowRefractionFix();
	bool GetAllowRefractBlood();
	bool GetAllowRefractArrows();

	const DoNotDispel& GetInvisState();
	const DoNotDispel& GetEtherealState();

	const Detection& GetInvisDetection();
	const Detection& GetEtherealDetection();

private:
	struct
	{
		bool fixRefraction{ true };
		bool refractBlood{ true };
		bool refractAttachedArrows{ true };

		DoNotDispel state{ DoNotDispel::kOnActivate };
		Detection detectState{ Detection::kDisabled };

	} invisibility;

	struct
	{
		DoNotDispel state{ DoNotDispel::kOnActivate };
		Detection detectState{ Detection::kDisabled };
	} etherealForm;

	struct detail
	{
		template <class T>
		static void get_value(CSimpleIniA& a_ini, T& a_value, const char* a_section, const char* a_key, const char* a_comment)
		{
			a_value = string::lexical_cast<T>(a_ini.GetValue(a_section, a_key, std::to_string(stl::to_underlying(a_value)).c_str()));
			a_ini.SetValue(a_section, a_key, std::to_string(stl::to_underlying(a_value)).c_str(), a_comment);
		};

		static void get_value(CSimpleIniA& a_ini, bool& a_value, const char* a_section, const char* a_key, const char* a_comment)
		{
			a_value = a_ini.GetBoolValue(a_section, a_key, a_value);
			a_ini.SetBoolValue(a_section, a_key, a_value, a_comment);
		};
	};
};
