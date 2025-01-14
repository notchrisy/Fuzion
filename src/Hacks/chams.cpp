#include "chams.h"

#include "lagcomp.h"

#include "../Utils/xorstring.h"
#include "../Utils/entity.h"
#include "../settings.h"
#include "../interfaces.h"
#include "../Hooks/hooks.h"

bool Settings::ESP::Chams::enabled = false;
bool Settings::ESP::Chams::Arms::enabled = false;
bool Settings::ESP::Chams::Weapon::enabled = false;
ArmsType Settings::ESP::Chams::Arms::type = ArmsType::DEFAULT;
WeaponType Settings::ESP::Chams::Weapon::type = WeaponType::DEFAULT;
HealthColorVar Settings::ESP::Chams::allyColor = ImColor(0, 0, 255, 255);
HealthColorVar Settings::ESP::Chams::allyVisibleColor = ImColor(0, 255, 0, 255);
HealthColorVar Settings::ESP::Chams::enemyColor = ImColor(255, 0, 0, 255);
HealthColorVar Settings::ESP::Chams::enemyVisibleColor = ImColor(255, 255, 0, 255);
HealthColorVar Settings::ESP::Chams::localplayerColor = ImColor(0, 255, 255, 255);
ColorVar Settings::ESP::Chams::Arms::color = ImColor(255, 255, 255, 255);
ColorVar Settings::ESP::Chams::Weapon::color = ImColor(255, 255, 255, 255);
ChamsType Settings::ESP::Chams::type = ChamsType::CHAMS;

IMaterial* materialChams;
IMaterial* materialChamsIgnorez;
IMaterial* materialChamsFlat;
IMaterial* materialChamsFlatIgnorez;
IMaterial* materialChamsCrystal;
IMaterial* materialChamsGold;
IMaterial* materialChamsArms;
IMaterial* materialChamsWeapons;

typedef void (*DrawModelExecuteFn) (void*, void*, void*, const ModelRenderInfo_t&, matrix3x4_t*);

static void DrawPlayer(void* thisptr, void* context, void *state, const ModelRenderInfo_t &pInfo, matrix3x4_t* pCustomBoneToWorld)
{
	if (!Settings::ESP::Chams::enabled)
		return;

	C_BasePlayer* localplayer = (C_BasePlayer*) entityList->GetClientEntity(engine->GetLocalPlayer());
	if (!localplayer)
		return;

	C_BasePlayer* entity = (C_BasePlayer*) entityList->GetClientEntity(pInfo.entity_index);
	if (!entity
		|| entity->GetDormant()
		|| !entity->GetAlive())
		return;

	if (entity == localplayer && !Settings::ESP::Filters::localplayer)
		return;

	if (!Entity::IsTeamMate(entity, localplayer) && !Settings::ESP::Filters::enemies)
		return;

	if (entity != localplayer && Entity::IsTeamMate(entity,localplayer) && !Settings::ESP::Filters::allies)
		return;

	IMaterial* visible_material = nullptr;
	IMaterial* hidden_material = nullptr;

	switch (Settings::ESP::Chams::type)
	{
		case ChamsType::CHAMS:
		case ChamsType::CHAMS_XQZ:
			visible_material = materialChams;
			hidden_material = materialChamsIgnorez;
			break;
		case ChamsType::CHAMS_FLAT:
		case ChamsType::CHAMS_FLAT_XQZ:
			visible_material = materialChamsFlat;
			hidden_material = materialChamsFlatIgnorez;
			break;
	}

	visible_material->AlphaModulate(1.0f);
	hidden_material->AlphaModulate(1.0f);

	if (entity == localplayer)
	{
		Color visColor = Color::FromImColor(Settings::ESP::Chams::localplayerColor.Color(entity));
		Color color = visColor;
		color *= 0.45f;

		visible_material->ColorModulate(visColor);
		hidden_material->ColorModulate(color);

		visible_material->AlphaModulate(Settings::ESP::Chams::localplayerColor.Color(entity).Value.w);
		hidden_material->AlphaModulate(Settings::ESP::Chams::localplayerColor.Color(entity).Value.w);
	}
	else if (Entity::IsTeamMate(entity, localplayer))
	{
		Color visColor = Color::FromImColor(Settings::ESP::Chams::allyVisibleColor.Color(entity));
		Color color = Color::FromImColor(Settings::ESP::Chams::allyColor.Color(entity));

		visible_material->ColorModulate(visColor);
		hidden_material->ColorModulate(color);
	}
	else if (!Entity::IsTeamMate(entity, localplayer))
	{
		Color visColor = Color::FromImColor(Settings::ESP::Chams::enemyVisibleColor.Color(entity));
		Color color = Color::FromImColor(Settings::ESP::Chams::enemyColor.Color(entity));

		visible_material->ColorModulate(visColor);
		hidden_material->ColorModulate(color);
	}
	else
	{
		return;
	}

	if (entity->GetImmune())
	{
		visible_material->AlphaModulate(0.5f);
		hidden_material->AlphaModulate(0.5f);
	}

	if (!Settings::ESP::Filters::legit && (Settings::ESP::Chams::type == ChamsType::CHAMS_XQZ || Settings::ESP::Chams::type == ChamsType::CHAMS_FLAT_XQZ))
	{
		modelRender->ForcedMaterialOverride(hidden_material);
		modelRenderVMT->GetOriginalMethod<DrawModelExecuteFn>(21)(thisptr, context, state, pInfo, pCustomBoneToWorld);
	}

	modelRender->ForcedMaterialOverride(visible_material);
	// No need to call DME again, it already gets called in DrawModelExecute.cpp
}

static void DrawRecord(void* thisptr, void* context, void *state, const ModelRenderInfo_t &pInfo, matrix3x4_t* pCustomBoneToWorld)
{
	if (!Settings::LagComp::enabled)
        return;

    C_BasePlayer* localplayer = (C_BasePlayer*) entityList->GetClientEntity(engine->GetLocalPlayer());
	if (!localplayer)
		return;
	if (LagComp::ticks.empty())
		return;

 	IMaterial* visible_material = materialChams;
 	Color visColor = Color(255, 0, 0, 255);
 	visible_material->ColorModulate(visColor);
 	visible_material->AlphaModulate(1.0f);
	auto &tick = LagComp::ticks.back();
	for (auto &record : tick.records)
	{
		if (!record.boneMatrix)
			continue;

		(Vector)pInfo.origin = record.origin;
		modelRender->ForcedMaterialOverride(visible_material);
		modelRenderVMT->GetOriginalMethod<DrawModelExecuteFn>(21)(thisptr, context, state, pInfo, (matrix3x4_t*)record.boneMatrix);
	}
}

static void DrawWeapon(const ModelRenderInfo_t& pInfo)
{
	if (!Settings::ESP::Chams::Weapon::enabled)
		return;

	std::string modelName = modelInfo->GetModelName(pInfo.pModel);
	IMaterial* mat = materialChamsWeapons;

	if (!Settings::ESP::Chams::Weapon::enabled)
		mat = material->FindMaterial(modelName.c_str(), TEXTURE_GROUP_MODEL);

	mat->ColorModulate(Settings::ESP::Chams::Weapon::color.Color());
	mat->AlphaModulate(Settings::ESP::Chams::Weapon::color.Color().Value.w);

	mat->SetMaterialVarFlag(MATERIAL_VAR_WIREFRAME, Settings::ESP::Chams::Weapon::type == WeaponType::WIREFRAME);
	mat->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, Settings::ESP::Chams::Weapon::type == WeaponType::NONE);
	modelRender->ForcedMaterialOverride(mat);
}

static void DrawArms(const ModelRenderInfo_t& pInfo)
{
	if (!Settings::ESP::Chams::Arms::enabled)
		return;

	std::string modelName = modelInfo->GetModelName(pInfo.pModel);
	IMaterial* mat = materialChamsArms;

	if (!Settings::ESP::Chams::Arms::enabled)
		mat = material->FindMaterial(modelName.c_str(), TEXTURE_GROUP_MODEL);

	mat->ColorModulate(Settings::ESP::Chams::Arms::color.Color());
	mat->AlphaModulate(Settings::ESP::Chams::Arms::color.Color().Value.w);

	mat->SetMaterialVarFlag(MATERIAL_VAR_WIREFRAME, Settings::ESP::Chams::Arms::type == ArmsType::WIREFRAME);
	mat->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, Settings::ESP::Chams::Arms::type == ArmsType::NONE);
	modelRender->ForcedMaterialOverride(mat);

	switch (Settings::ESP::Chams::Arms::type)
	{
		default:
			break;
		case ArmsType::DEFAULT:
			mat->AlphaModulate(1.0f);
			mat->ColorModulate(Settings::ESP::Chams::Arms::color.Color());
			break;
			case ArmsType::CRYSTAL:
			mat = material->FindMaterial("models/inventory_items/trophy_majors/crystal_clear", TEXTURE_GROUP_OTHER);
			mat->AlphaModulate(0.6f);
			mat->ColorModulate(Settings::ESP::Chams::Arms::color.Color());
			break;
			case ArmsType::GOLD:
			mat = material->FindMaterial("models/inventory_items/trophy_majors/gold", TEXTURE_GROUP_OTHER);
			mat->AlphaModulate(0.7f);
			mat->ColorModulate(Settings::ESP::Chams::Arms::color.Color());
			break;
	}
}

void Chams::DrawModelExecute(void* thisptr, void* context, void *state, const ModelRenderInfo_t &pInfo, matrix3x4_t* pCustomBoneToWorld)
{
	if (!engine->IsInGame())
		return;

	if (!Settings::ESP::enabled)
		return;

	if (!pInfo.pModel)
		return;

	static bool materialsCreated = false;
	if (!materialsCreated)
	{
		materialChams = Util::CreateMaterial(XORSTR("VertexLitGeneric"), XORSTR("VGUI/white_additive"), false, true, true, true, true);
		materialChamsIgnorez = Util::CreateMaterial(XORSTR("VertexLitGeneric"), XORSTR("VGUI/white_additive"), true, true, true, true, true);
		materialChamsFlat = Util::CreateMaterial(XORSTR("UnlitGeneric"), XORSTR("VGUI/white_additive"), false, true, true, true, true);
		materialChamsFlatIgnorez = Util::CreateMaterial(XORSTR("UnlitGeneric"), XORSTR("VGUI/white_additive"), true, true, true, true, true);
		materialChamsCrystal = material->FindMaterial("models/inventory_items/trophy_majors/crystal_clear",TEXTURE_GROUP_OTHER);
		materialChamsGold = material->FindMaterial("models/inventory_items/trophy_majors/gold",TEXTURE_GROUP_OTHER);
		materialChamsArms = Util::CreateMaterial(XORSTR("VertexLitGeneric"), XORSTR("VGUI/white_additive"), false, true, true, true, true);
		materialChamsWeapons = Util::CreateMaterial(XORSTR("VertexLitGeneric"), XORSTR("VGUI/white_additive"), false, true, true, true, true);
		materialsCreated = true;
	}

	std::string modelName = modelInfo->GetModelName(pInfo.pModel);

		if (modelName.find(XORSTR("models/player")) != std::string::npos){
		DrawPlayer(thisptr, context, state, pInfo, pCustomBoneToWorld);
		}
	else if (modelName.find(XORSTR("arms")) != std::string::npos)
		DrawArms(pInfo);
	else if (modelName.find(XORSTR("weapon")) != std::string::npos)
		DrawWeapon(pInfo);
}
