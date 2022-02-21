#include "Settings.h"

//for AE/VR
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

//for AE/VR
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
			}
            if (a_target->HasEffectWithArchetype(Archetype::kEtherealize)) {
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
	inline void set_refraction(RE::NiAVObject* a_object, bool a_enable, float a_power, bool a_unk04)
	{
		using func_t = decltype(&set_refraction);
		REL::Relocation<func_t> func{ REL::ID(99868) };
		return func(a_object, a_enable, a_power, a_unk04);
	}

	namespace Alt
	{
		struct SetShaderFlag
		{
			static void thunk(RE::BSShaderProperty*, RE::BSShaderProperty::EShaderPropertyFlag8, bool)
			{
				return;
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL::ID(99868) };
			stl::write_thunk_call<SetShaderFlag>(target.address() + 0x97);
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
		static bool has_refraction(const RE::Actor* a_actor)
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
						Refraction::set_refraction(a_this->decal.get(), true, 1.0f, true);
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
						Refraction::set_refraction(a_projectile3D.get(), true, 0.5f, true);
					}
				}

				func(a_extraList, a_projectile3D, a_projectile);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL::ID(42856) };
			stl::write_thunk_call<AddAttachedArrow3D>(target.address() +
#ifndef SKYRIMVR
			0x53A
#else
			0x737
#endif
			);
		}
	}

	namespace AlphaBlendedArmor
	{
		struct detail
		{
			static void bseffectshader_blending_on_armor_fix(const RE::BSTSmartPointer<RE::BipedAnim>& a_biped, float a_power)
			{
				if (a_biped) {
					for (auto& bipedObject : a_biped->objects) {
						auto& model = bipedObject.partClone;
						if (model) {
							RE::BSVisit::TraverseScenegraphGeometries(model.get(), [&](RE::BSGeometry* a_geometry) -> RE::BSVisit::BSVisitControl {
								if (const auto shape = a_geometry->AsTriShape(); shape) {
									const auto effectProp = netimmerse_cast<RE::BSEffectShaderProperty*>(a_geometry->properties[RE::BSGeometry::States::kEffect].get());
									const auto alphaProp = netimmerse_cast<RE::NiAlphaProperty*>(a_geometry->properties[RE::BSGeometry::States::kProperty].get());
									if (effectProp && alphaProp && alphaProp->GetAlphaBlending()) {
										shape->SetAppCulled(a_power > 0.0f);
									}
								}
								return RE::BSVisit::BSVisitControl::kContinue;
							});
						}
					}
				}
			}
		};

		namespace Player
		{
			struct SetRefraction
			{
				static void thunk(RE::PlayerCharacter* a_this, bool a_enable, float a_refraction)
				{
					func(a_this, a_enable, a_refraction);

					if (const auto& fBiped = a_this->GetBiped(true)) {
						detail::bseffectshader_blending_on_armor_fix(fBiped, a_refraction);
					}
					if (const auto& tBiped = a_this->GetBiped(false)) {
						detail::bseffectshader_blending_on_armor_fix(tBiped, a_refraction);
					}
				}
				static inline REL::Relocation<decltype(thunk)> func;

#ifndef SKYRIMVR
				static inline constexpr size_t size = 0x0C3;
#else
				static inline constexpr size_t size = 0x0C5;
#endif
			};
		}

		namespace Character
		{
			struct SetRefraction
			{
				static void thunk(RE::Character* a_this, bool a_enable, float a_refraction)
				{
					func(a_this, a_enable, a_refraction);

					if (const auto& biped = a_this->GetBiped()) {
						detail::bseffectshader_blending_on_armor_fix(biped, a_refraction);
					}
				}
				static inline REL::Relocation<decltype(thunk)> func;

#ifndef SKYRIMVR
				static inline constexpr size_t size = 0x0C3;
#else
				static inline constexpr size_t size = 0x0C5;
#endif
			};
		}

		void Install()
		{
			stl::write_vfunc<RE::PlayerCharacter, Player::SetRefraction>();
			stl::write_vfunc<RE::Character, Character::SetRefraction>();
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
		if (settings->GetAllowAlphaBlendFix()) {
			AlphaBlendedArmor::Install();
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
	spdlog::set_pattern("[%H:%M:%S:%e] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "Enhanced Invisibility";
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver <
#ifndef SKYRIMVR
		SKSE::RUNTIME_1_5_39
#else
		SKSE::RUNTIME_VR_1_4_15
#endif
	) {
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
		SKSE::AllocTrampoline(
#ifndef SKYRIMVR
			28
#else
			56
#endif
		);

		Refraction::Install();
		MakeInvisible::Install();

#ifdef SKYRIMVR
		UninterruptedAlteredStates::Install();
		Detection::Install();
#endif
	}

	return true;
}
