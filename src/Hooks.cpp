#include "Hooks.h"
#include "Settings.h"

namespace EnhancedInvisibility
{
	namespace MakeUninterrupted
	{
		struct detail
		{
			static void dispel_invisibility(RE::Actor* a_actor, RE::EffectArchetype a_archetype)
			{
				if ((a_actor->flags & 2) != 0 && a_archetype != RE::EffectArchetype::kInvisibility) {
					a_actor->DispelEffectsWithArchetype(RE::EffectArchetype::kInvisibility, true);
				}
			}

			static void dispel_ethereal_form(RE::Actor* a_actor, RE::EffectArchetype a_archetype)
			{
				if (!a_actor->IsGhost()) {
					return;
				}
				if (a_archetype != RE::EffectArchetype::kEtherealize) {
					a_actor->DispelEffectsWithArchetype(RE::EffectArchetype::kEtherealize, true);
				}
			}
		};

		namespace Activate
		{
			struct DispelAlteredStates
			{
				static void thunk(RE::Actor* a_actor, RE::EffectArchetype a_archetype)
				{
					const auto settings = Settings::GetSingleton();

					if (settings->GetInvisState() == DoNotDispel::kDisabled) {
						detail::dispel_invisibility(a_actor, a_archetype);
					}

					if (settings->GetEtherealState() == DoNotDispel::kDisabled) {
						detail::dispel_ethereal_form(a_actor, a_archetype);
					}
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			void Install()
			{
				REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(19369, 19796), OFFSET(0x61A, 0x74A) };
				stl::write_thunk_call<DispelAlteredStates>(target.address());
			}
		}

		namespace All
		{
			struct DispelAlteredStates
			{
				static void func(RE::Actor* a_actor, RE::EffectArchetype a_archetype)
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
				REL::Relocation<std::uintptr_t> func{ RELOCATION_ID(37864, 38819) };
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
				
				logger::info("Installing Uninterrupted Actions [Activate] hook");
			}
			if (invisState == DoNotDispel::kOnAll || etherealState == DoNotDispel::kOnAll) {
				All::Install();
				
				logger::info("Installing Uninterrupted Actions [All] hook");
			}
		}
	}

	namespace Detection
	{
		struct CalculateDetection
		{
			static void thunk(
				RE::Actor*     a_source,
				RE::Actor*     a_target,
				std::int32_t&  a_detectionValue,
				std::uint8_t&  a_unk04,
				std::uint8_t&  a_unk05,
				std::uint32_t& a_unk06,
				RE::NiPoint3&  a_pos,
				float&         a_unk08,
				float&         a_unk09,
				float&         a_unk10)
			{
				if (Settings::GetSingleton()->ShouldMakeSuperInvisible(a_target)) {
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

			if (settings->GetInvisDetection() != DetectionState::kDisabled || settings->GetEtherealDetection() != DetectionState::kDisabled) {
				REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(41659, 42742), OFFSET(0x526, 0x67B) };
				stl::write_thunk_call<CalculateDetection>(target.address());

				logger::info("Installing Make Undetectable hook");
			}
		}
	}

	namespace Refraction
	{
		void set_refraction(RE::NiAVObject* a_object, bool a_enable, float a_power, bool a_unk04)
		{
			using func_t = decltype(&set_refraction);
			static REL::Relocation<func_t> func{ RELOCATION_ID(99868, 106513) };
			return func(a_object, a_enable, a_power, a_unk04);
		}

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
			const auto settings = Settings::GetSingleton();

			if (settings->GetAllowRefractionFix()) {
				REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(99868, 106513), OFFSET(0x97, 0xAE) };
				stl::write_thunk_call<SetShaderFlag>(target.address());

				logger::info("Installing Refraction Shader Fix");
			}
		}
	}

	namespace MakeInvisible
	{
		namespace Blood
		{
			struct Initialize
			{
				static void thunk(RE::BSTempEffectGeometryDecal* a_this)
				{
					func(a_this);

					if (a_this && a_this->decal && a_this->attachedGeometry) {
						const auto user = a_this->attachedGeometry->GetUserData();
						const auto actor = user ? user->As<RE::Actor>() : nullptr;

						if (actor && actor->extraList.HasType<RE::ExtraRefractionProperty>()) {  //doesn't matter what refraction power is
							Refraction::set_refraction(a_this->decal.get(), true, 1.0f, true);
						}
					}
				}
				static inline REL::Relocation<decltype(thunk)> func;
				static constexpr std::size_t                   index = 0x25;
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
						if (actor && actor->extraList.HasType<RE::ExtraRefractionProperty>()) {
							Refraction::set_refraction(a_projectile3D.get(), true, 0.5f, true);
						}
					}

					func(a_extraList, a_projectile3D, a_projectile);
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			void Install()
			{
				REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(42856, 44031) };
				stl::write_thunk_call<AddAttachedArrow3D>(target.address() + OFFSET_3(0x53A, 0x688, 0x737));
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
							if (auto& model = bipedObject.partClone) {
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
					static constexpr std::size_t                   index = OFFSET_VTABLE(0x0C3, 0x0C5);
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
					static constexpr std::size_t                   index = OFFSET_VTABLE(0x0C3, 0x0C5);
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

				logger::info("Installing Dynamic Blood Refraction Fix");
			}
			if (settings->GetAllowRefractBlood()) {
				Blood::Install();

				logger::info("Installing Dynamic Attached Arrow Refraction Fix");
			}
			if (settings->GetAllowAlphaBlendFix()) {
				AlphaBlendedArmor::Install();

				logger::info("Installing Armor Alpha Blend Fix");
			}
		}
	}

	void Install()
	{
		logger::info("{:*^30}", "HOOKS");

		Settings::GetSingleton()->LoadSettings();

		SKSE::AllocTrampoline(14*4);
		
		Refraction::Install();
		MakeInvisible::Install();

#if !defined(SKYRIM_AE) && !defined(SKYRIMVR)  // SSE only
		if (GetModuleHandle(L"Data\\DLLPlugins\\NetScriptFramework.Runtime.dll")) {
			const auto pluginPath = std::filesystem::path(R"(Data\NetScriptFramework\Plugins\)");

			const auto invisibilityDLL = pluginPath / "UninterruptedInvisibility.dll";
			const auto etherealFormDLL = pluginPath / "UninterruptedEtherealForm.dll";

			std::error_code ec;
			if (std::filesystem::exists(invisibilityDLL, ec) || std::filesystem::exists(etherealFormDLL, ec)) {
				logger::info("UninterruptedEtherealForm/UninterruptedInvisibility plugins detected, skipping hooks");
				return;
			}
		}
#endif
		MakeUninterrupted::Install();
		Detection::Install();
	}
}
