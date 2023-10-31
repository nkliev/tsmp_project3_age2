#include "stdafx.h"
#pragma hdrstop

#include "ModelPool.h"

#include "IGame_Persistent.h"
#include "fmesh.h"
#include "fhierrarhyvisual.h"
#include "SkeletonAnimated.h"
#include "fvisual.h"
#include "fprogressive.h"
#include "fskinned.h"
#include "flod.h"
#include "ftreevisual.h"
#include "ParticleGroup.h"

dxRender_Visual *CModelPool::Instance_Create(u32 type)
{
	dxRender_Visual *V = NULL;

	// Check types
	switch (type)
	{
	case MT_NORMAL: // our base visual
		V = xr_new<Fvisual>();
		break;
	case MT_HIERRARHY:
		V = xr_new<FHierrarhyVisual>();
		break;
	case MT_PROGRESSIVE: // dynamic-resolution visual
		V = xr_new<FProgressive>();
		break;
	case MT_SKELETON_ANIM:
		V = xr_new<CKinematicsAnimated>();
		break;
	case MT_SKELETON_RIGID:
		V = xr_new<CKinematics>();
		break;
	case MT_SKELETON_GEOMDEF_PM:
		V = xr_new<CSkeletonX_PM>();
		break;
	case MT_SKELETON_GEOMDEF_ST:
		V = xr_new<CSkeletonX_ST>();
		break;
	case MT_PARTICLE_EFFECT:
		V = xr_new<PS::CParticleEffect>();
		break;
	case MT_PARTICLE_GROUP:
		V = xr_new<PS::CParticleGroup>();
		break;

	case MT_LOD:
		V = xr_new<FLOD>();
		break;
	case MT_TREE_ST:
		V = xr_new<FTreeVisual_ST>();
		break;
	case MT_TREE_PM:
		V = xr_new<FTreeVisual_PM>();
		break;

	default:
		FATAL("Unknown visual type");
		break;
	}
	R_ASSERT(V);
	V->Type = type;
	return V;
}

dxRender_Visual *CModelPool::Instance_Duplicate(dxRender_Visual *V)
{
	R_ASSERT(V);
	dxRender_Visual *N = Instance_Create(V->Type);
	N->Copy(V);
	N->Spawn();
	// inc ref counter
	for (xr_vector<ModelDef>::iterator I = Models.begin(); I != Models.end(); I++)
		if (I->model == V)
		{
			I->refs++;
			break;
		}
	return N;
}

dxRender_Visual *CModelPool::Instance_Load(const char *N, BOOL allow_register)
{
	dxRender_Visual *V;
	string_path fn;
	string_path name;

	// Add default ext if no ext at all
	if (0 == strext(N))
		strconcat(sizeof(name), name, N, ".ogf");
	else
		strcpy_s(name, sizeof(name), N);

	// Load data from MESHES or LEVEL
	if (!FS.exist(N))
	{
		if (!FS.exist(fn, "$level$", name))
			if (!FS.exist(fn, "$game_meshes$", name))
			{

				Debug.fatal(DEBUG_INFO, "Can't find model file '%s'.", name);
			}
	}
	else
	{
		strcpy(fn, N);
	}

	// Actual loading
#ifdef DEBUG
	if (bLogging)
		Msg("- Uncached model loading: %s", fn);
#endif // DEBUG

	IReader *data = FS.r_open(fn);
	ogf_header H;
	data->r_chunk_safe(OGF_HEADER, &H, sizeof(H));
	V = Instance_Create(H.type);
	V->Load(N, data, 0);
	FS.r_close(data);
	g_pGamePersistent->RegisterModel(V);

	// Registration
	if (allow_register)
		Instance_Register(N, V);

	return V;
}

dxRender_Visual *CModelPool::Instance_Load(LPCSTR name, IReader *data, BOOL allow_register)
{
	dxRender_Visual *V;

	ogf_header H;
	data->r_chunk_safe(OGF_HEADER, &H, sizeof(H));
	V = Instance_Create(H.type);
	V->Load(name, data, 0);

	// Registration
	if (allow_register)
		Instance_Register(name, V);
	return V;
}

void CModelPool::Instance_Register(LPCSTR N, dxRender_Visual *V)
{
	// Registration
	ModelDef M;
	M.name = N;
	M.model = V;
	Models.push_back(M);
}

void CModelPool::Destroy()
{
	// Pool
	Pool.clear();

	// Registry
	while (!Registry.empty())
	{
		REGISTRY_IT it = Registry.begin();
		dxRender_Visual *V = it->first;
#ifdef _DEBUG
		Msg("ModelPool: Destroy object: '%s'", *V->dbg_name);
#endif
		DeleteInternal(V, TRUE);
	}

	// Base/Reference
	xr_vector<ModelDef>::iterator I = Models.begin();
	xr_vector<ModelDef>::iterator E = Models.end();
	for (; I != E; I++)
	{
		I->model->Release();
		xr_delete(I->model);
	}

	Models.clear();

	// cleanup motions container
	g_pMotionsContainer->clean(false);
}

CModelPool::CModelPool()
{
	bLogging = TRUE;
	bForceDiscard = FALSE;
	bAllowChildrenDuplicate = TRUE;
	g_pMotionsContainer = xr_new<motions_container>();
}

CModelPool::~CModelPool()
{
	Destroy();
	xr_delete(g_pMotionsContainer);
}

dxRender_Visual *CModelPool::Instance_Find(LPCSTR N)
{
	dxRender_Visual *Model = 0;
	xr_vector<ModelDef>::iterator I;
	for (I = Models.begin(); I != Models.end(); I++)
	{
		if (I->name[0] && (0 == xr_strcmp(*I->name, N)))
		{
			Model = I->model;
			break;
		}
	}
	return Model;
}

dxRender_Visual *CModelPool::Create(const char *name, IReader *data)
{

	string_path low_name;
	VERIFY(xr_strlen(name) < sizeof(low_name));
	strcpy(low_name, name);
	strlwr(low_name);
	if (strext(low_name))
		*strext(low_name) = 0;
	//	Msg						("-CREATE %s",low_name);

	// 0. Search POOL
	POOL_IT it = Pool.find(low_name);
	if (it != Pool.end())
	{
		// 1. Instance found
		dxRender_Visual *Model = it->second;
		Model->Spawn();
		Pool.erase(it);
		return Model;
	}
	else
	{
		// 1. Search for already loaded model (reference, base model)
		dxRender_Visual *Base = Instance_Find(low_name);

		if (0 == Base)
		{
			// 2. If not found
			bAllowChildrenDuplicate = FALSE;
			if (data)
				Base = Instance_Load(low_name, data, TRUE);
			else
				Base = Instance_Load(low_name, TRUE);
			bAllowChildrenDuplicate = TRUE;
		}
		// 3. If found - return (cloned) reference
		dxRender_Visual *Model = Instance_Duplicate(Base);
		Registry.insert(mk_pair(Model, low_name));
		return Model;
	}
}

dxRender_Visual *CModelPool::CreateChild(LPCSTR name, IReader *data)
{
	string256 low_name;
	VERIFY(xr_strlen(name) < 256);
	strcpy(low_name, name);
	strlwr(low_name);
	if (strext(low_name))
		*strext(low_name) = 0;

	// 1. Search for already loaded model
	dxRender_Visual *Base = Instance_Find(low_name);
	//.	if (0==Base) Base	 	= Instance_Load(name,data,FALSE);
	if (0 == Base)
	{
		if (data)
			Base = Instance_Load(low_name, data, FALSE);
		else
			Base = Instance_Load(low_name, FALSE);
	}

	dxRender_Visual *Model = bAllowChildrenDuplicate ? Instance_Duplicate(Base) : Base;
	return Model;
}

extern ENGINE_API BOOL g_bRendering;
void CModelPool::DeleteInternal(dxRender_Visual *&V, BOOL bDiscard)
{
	VERIFY(!g_bRendering);
	if (!V)
		return;
	V->Depart();
	if (bDiscard || bForceDiscard)
	{
		Discard(V, TRUE);
	}
	else
	{
		//
		REGISTRY_IT it = Registry.find(V);
		if (it != Registry.end())
		{
			// Registry entry found - move it to pool
			Pool.insert(mk_pair(it->second, V));
		}
		else
		{
			// Registry entry not-found - just special type of visual / particles / etc.
			xr_delete(V);
		}
	}
	V = NULL;
}

void CModelPool::Delete(dxRender_Visual *&V, BOOL bDiscard)
{
	if (NULL == V)
		return;
	if (g_bRendering)
	{
		VERIFY(!bDiscard);
		ModelsToDelete.push_back(V);
	}
	else
	{
		DeleteInternal(V, bDiscard);
	}
	V = NULL;
}

void CModelPool::DeleteQueue()
{
	for (u32 it = 0; it < ModelsToDelete.size(); it++)
		DeleteInternal(ModelsToDelete[it]);
	ModelsToDelete.clear();
}

void CModelPool::Discard(dxRender_Visual *&V, BOOL b_complete)
{
	//
	REGISTRY_IT it = Registry.find(V);
	if (it != Registry.end())
	{
		// Pool - OK

		// Base
		const shared_str &name = it->second;
		xr_vector<ModelDef>::iterator I = Models.begin();
		xr_vector<ModelDef>::iterator I_e = Models.end();

		for (; I != I_e; ++I)
		{
			if (I->name == name)
			{
				if (b_complete || strchr(*name, '#'))
				{
					VERIFY(I->refs > 0);
					I->refs--;
					if (0 == I->refs)
					{
						bForceDiscard = TRUE;
						I->model->Release();
						xr_delete(I->model);
						Models.erase(I);
						bForceDiscard = FALSE;
					}
					break;
				}
				else
				{
					if (I->refs > 0)
						I->refs--;
					break;
				}
			}
		}
		// Registry
		xr_delete(V);
		//.		xr_free			(name);
		Registry.erase(it);
	}
	else
	{
		// Registry entry not-found - just special type of visual / particles / etc.
		xr_delete(V);
	}
	V = NULL;
}

void CModelPool::Prefetch()
{
	Logging(FALSE);
	// prefetch visuals
	string256 section;
	string256 gameTypeName;
	strcpy(gameTypeName, g_pGamePersistent->m_game_params.m_game_type);

	if (!strcmp(gameTypeName, "hardmatch"))
		strcpy(gameTypeName, "deathmatch");

	if (!strcmp(gameTypeName, "freeplay"))
		strcpy(gameTypeName, "deathmatch");

	if (!strcmp(gameTypeName, "race"))
		strcpy(gameTypeName, "deathmatch");

	if (!strcmp(gameTypeName, "carfight"))
		strcpy(gameTypeName, "teamdeathmatch");

	strconcat(sizeof(section), section, "prefetch_visuals_", gameTypeName);
	CInifile::Sect &sect = pSettings->r_section(section);
	for (CInifile::SectCIt I = sect.Data.begin(); I != sect.Data.end(); I++)
	{
		const CInifile::Item &item = *I;
		dxRender_Visual *V = Create(item.first.c_str());
		Delete(V, FALSE);
	}
	Logging(TRUE);
}

void CModelPool::ClearPool(BOOL b_complete)
{
	POOL_IT _I = Pool.begin();
	POOL_IT _E = Pool.end();
	for (; _I != _E; _I++)
	{
		Discard(_I->second, b_complete);
	}
	Pool.clear();
}

dxRender_Visual *CModelPool::CreatePE(PS::CPEDef *source)
{
	PS::CParticleEffect *V = (PS::CParticleEffect *)Instance_Create(MT_PARTICLE_EFFECT);
	V->Compile(source);
	return V;
}

dxRender_Visual *CModelPool::CreatePG(PS::CPGDef *source)
{
	PS::CParticleGroup *V = (PS::CParticleGroup *)Instance_Create(MT_PARTICLE_GROUP);
	V->Compile(source);
	return V;
}

void CModelPool::dump()
{
	Log("--- model pool --- begin:");
	u32 sz = 0;
	u32 k = 0;
	for (xr_vector<ModelDef>::iterator I = Models.begin(); I != Models.end(); I++)
	{
		CKinematics *K = PCKinematics(I->model);
		if (K)
		{
			u32 cur = K->mem_usage(false);
			sz += cur;
			Msg("#%3d: [%3d/%5d Kb] - %s", k++, I->refs, cur / 1024, I->name.c_str());
		}
	}
	Msg("--- models: %d, mem usage: %d Kb ", k, sz / 1024);
	sz = 0;
	k = 0;
	int free_cnt = 0;
	for (REGISTRY_IT it = Registry.begin(); it != Registry.end(); it++)
	{
		CKinematics *K = PCKinematics((dxRender_Visual *)it->first);
		VERIFY(K);
		if (K)
		{
			u32 cur = K->mem_usage(true);
			sz += cur;
			bool b_free = (Pool.find(it->second) != Pool.end());
			if (b_free)
				++free_cnt;
			Msg("#%3d: [%s] [%5d Kb] - %s", k++, (b_free) ? "free" : "used", cur / 1024, it->second.c_str());
		}
	}
	Msg("--- instances: %d, free %d, mem usage: %d Kb ", k, free_cnt, sz / 1024);
	Log("--- model pool --- end.");
}
