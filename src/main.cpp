#include "Settings.h"

//for AE
namespace UninterruptedAlteredStates
{
	using Archetype = RE::EffectArchetypes::ArchetypeID;
	using DoNotDispel = Settings::DoNotDispel;

	struct detail
	{
		static void dispel_invisibility(RE::Actor* a_actor, Archetype a_archetype)
		{
			if ((a_actor->flags & 2) != 0 && a_archetype != Archetype::kInvisibility) {
				a_actor->DispelEffectsWithArchetype(Archetype::kInvisibility, true);
			}
		}

		static void dispel_ethereal_form(RE::Actor* a_actor, Archetype a_archetype)
		{
			if (!a_actor->IsGhost()) {
				return;
			}
			if (a_archetype != Archetype::kEtherealize) {
				a_actor->DispelEffectsWithArchetype(Archetype::kEtherealize, true);
			}
		}
	};

	namespace Activate
	{
		struct DispelAlteredStates
		{
			static void thunk(RE::Actor* a_actor, Archetype a_archetype)
			{
				const auto settings = Settings::GetSingleton();

				if (settings->GetInvisState() != DoNotDispel::kOnActivate) {
					detail::dispel_invisibility(a_actor, a_archetype);
				}

				if (settings->GetEtherealState() != DoNotDispel::kOnActivate) {
					detail::dispel_ethereal_form(a_actor, a_archetype);
				}
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL::ID(19369) };
			stl::write_thunk_call<DispelAlteredStates>(target.address() + 0x61A);
		}
	}

	namespace All
	{
		struct DispelAlteredStates
		{
			static void func(RE::Actor* a_actor, Archetype a_archetype)
			{
				const auto settings = Settings::GetSingleton();

				if (settings->GetInvisState() != DoNotDispel::kOnAll) {
					detail::dispel_invisibility(a_actor, a_archetype);
				}

				if (settings->GetEtherealState() != DoNotDispel::kOnAll) {
					detail::dispel_ethereal_form(a_actor, a_archetype);
				}
			}
			static inline constexpr std::size_t size = 0x5F;
		};

		void Install()
		{
			REL::Relocation<std::uintptr_t> func{ REL::ID(37864) };
			stl::asm_replace<DispelAlteredStates>(func.address());
		}
	}

	void Install()
	{
		const auto settings = Settings::GetSingleton();

		const auto invisState = settings->GetInvisState();
		const auto etherealState = settings->GetEtherealState();

		if (invisState == DoNotDispel::kOnActivate || etherealState == DoNotDispel::kOnActivate) {
			Activate::Install();
		}
		if (invisState == DoNotDispel::kOnAll || etherealState == DoNotDispel::kOnAll) {
			All::Install();
		}
	}
}

//for AE
namespace Detection
{
	using Archetype = RE::EffectArchetypes::ArchetypeID;
	using Detection = Settings::Detection;

	struct detail
	{
		static bool should_make_super_invisible(RE::Actor* a_target)
		{
			const auto settings = Settings::GetSingleton();

			if (a_target->HasEffectWithArchetype(Archetype::kInvisibility)) {
				const auto state = settings->GetInvisDetection();
				return state == Detection::kEveryone || state == Detection::kOnlyPlayer && a_target->IsPlayerRef();
			} else if (a_target->HasEffectWithArchetype(Archetype::kEtherealize)) {
				const auto state = settings->GetEtherealDetection();
				return state == Detection::kEveryone || state == Detection::kOnlyPlayer && a_target->IsPlayerRef();
			}
			return false;
		}
	};

	struct CalculateDetection
	{
		static void thunk(
			RE::Actor* a_source,
			RE::Actor* a_target,
			std::int32_t& a_detectionValue,
			std::uint8_t& a_unk04,
			std::uint8_t& a_unk05,
			std::uint32_t& a_unk06,
			RE::NiPoint3& a_pos,
			float& a_unk08,
			float& a_unk09,
			float& a_unk10)
		{
			if (detail::should_make_super_invisible(a_target)) {
				a_detectionValue = -1000;
				return;
			}
			return func(a_source, a_target, a_detectionValue, a_unk04, a_unk05, a_unk06, a_pos, a_unk08, a_unk09, a_unk10);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		const auto settings = Settings::GetSingleton();
		if (settings->GetInvisDetection() != Detection::kDisabled || settings->GetEtherealDetection() != Detection::kDisabled) {
			REL::Relocation<std::uintptr_t> target{ REL::ID(41659), 0x526 };
			stl::write_thunk_call<CalculateDetection>(target.address());
		}
	}
}

namespace Refraction
{
	inline void set_refraction(RE::NiAVObject* a_object, bool a_enable, float a_power)
	{
		using Flag = RE::BSShaderProperty::EShaderPropertyFlag;
		using Flag8 = RE::BSShaderProperty::EShaderPropertyFlag8;

		const auto power = std::fminf(std::fmaxf(a_power, 0.0f), 1.0f);

		RE::BSVisit::TraverseScenegraphGeometries(a_object, [&](RE::BSGeometry* a_geometry) -> RE::BSVisit::BSVisitControl {
			if (const auto shape = a_geometry->AsTriShape(); shape) {
				const auto shaderProp = netimmerse_cast<RE::BSShaderProperty*>(a_geometry->properties[RE::BSGeometry::States::kEffect].get());
				if (shaderProp) {
					shaderProp->SetFlags(Flag8::kTempRefraction, a_enable);

					if (const auto lightingShader = netimmerse_cast<RE::BSLightingShaderProperty*>(shaderProp); lightingShader && lightingShader->flags.none(Flag::kRefraction)) {
						if (auto material = lightingShader->material; material) {
							//set_unique
							if (!material->unk30) {
								lightingShader->SetMaterial(material, true);
							}

							//this is what breaks the refraction shader
							//lightingShader->SetFlags(Flag8::kNonProjectiveShadows, a_enable);

							static_cast<RE::BSLightingShaderMaterialBase*>(lightingShader->material)->refractionPower = power;
						}
					}
				}
			}

			return RE::BSVisit::BSVisitControl::kContinue;
		});
	}

	namespace Alt
	{
		struct SetRefractionRecursive
		{
			static void func(RE::NiAVObject* a_object, bool a_enable, float a_power)
			{
				set_refraction(a_object, a_enable, a_power);
			}
			static inline constexpr std::size_t size = 0x126;
		};

		void Install()
		{
			REL::Relocation<std::uintptr_t> func{ REL::ID(99868) };
			stl::asm_replace<Alt::SetRefractionRecursive>(func.address());
		}
	}

	void Install()
	{
		const auto settings = Settings::GetSingleton();
		if (settings->GetAllowRefractionFix()) {
			Alt::Install();
		}
	}
}

namespace MakeInvisible
{
	struct detail
	{
		static bool has_refraction(RE::Actor* a_actor)
		{
			return a_actor->extraList.HasType<RE::ExtraRefractionProperty>();
		}
	};

	namespace Blood
	{
		struct Initialize
		{
			static void thunk(RE::BSTempEffectGeometryDecal* a_this)
			{
				func(a_this);

				if (a_this->decal && a_this->attachedGeometry) {
					const auto user = a_this->attachedGeometry->GetUserData();
					const auto actor = user ? user->As<RE::Actor>() : nullptr;
					if (actor && detail::has_refraction(actor)) {
						//doesn't matter what refraction power is
						Refraction::set_refraction(a_this->decal.get(), true, 1.0f);
					}
				}
			}
			static inline REL::Relocation<decltype(thunk)> func;
			static inline size_t size = 0x25;
		};

		void Install()
		{
			stl::write_vfunc<RE::BSTempEffectGeometryDecal, Initialize>();
		}
	}

	namespace Arrows
	{
		struct AddAttachedArrow3D
		{
			static void thunk(RE::ExtraDataList* a_extraList, const RE::NiPointer<RE::NiAVObject>& a_projectile3D, RE::BGSProjectile* a_projectile)
			{
				if (a_projectile3D) {
					const auto actor = stl::adjust_pointer<RE::Actor>(a_extraList, -0x70);
					if (actor && detail::has_refraction(actor)) {
						Refraction::set_refraction(a_projectile3D.get(), true, 0.5f);
					}
				}

				func(a_extraList, a_projectile3D, a_projectile);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL::ID(42856) };
			stl::write_thunk_call<AddAttachedArrow3D>(target.address() + 0x53A);
		}
	}

	void Install()
	{
		const auto settings = Settings::GetSingleton();
		if (settings->GetAllowRefractArrows()) {
			Arrows::Install();
		}
		if (settings->GetAllowRefractBlood()) {
			Blood::Install();
		}
	}
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%l] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "Enhanced Invisibility";
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	logger::info("loaded plugin");

	SKSE::Init(a_skse);

	if (Settings::GetSingleton()->LoadSettings()) {
		SKSE::AllocTrampoline(14);

		Refraction::Install();
		MakeInvisible::Install();

		//UninterruptedAlteredStates::Install();
		//Detection::Install();
	}

	return true;
}
