#include "stdafx.h"
#pragma hdrstop

#include "xr_efflensflare.h"

#include "igame_persistent.h"
#include "Environment.h"
#include "bone.h"
#include "../Include/xrRender/Kinematics.h"
#include "cl_intersect.h"

#include "xr_object.h"
#include "igame_level.h"

#include "../xrGame/object_broker.h"

#define FAR_DIST g_pGamePersistent->Environment().CurrentEnv.far_plane

//////////////////////////////////////////////////////////////////////////////
// Globals ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
#define BLEND_INC_SPEED 8.0f
#define BLEND_DEC_SPEED 4.0f

//------------------------------------------------------------------------------
void CLensFlareDescriptor::SetSource(float fRadius, BOOL ign_color, LPCSTR tex_name, LPCSTR sh_name)
{
	m_Source.fRadius = fRadius;
	m_Source.shader = sh_name;
	m_Source.texture = tex_name;
	m_Source.ignore_color = ign_color;
}

void CLensFlareDescriptor::SetGradient(float fMaxRadius, float fOpacity, LPCSTR tex_name, LPCSTR sh_name)
{
	m_Gradient.fRadius = fMaxRadius;
	m_Gradient.fOpacity = fOpacity;
	m_Gradient.shader = sh_name;
	m_Gradient.texture = tex_name;
}

void CLensFlareDescriptor::AddFlare(float fRadius, float fOpacity, float fPosition, LPCSTR tex_name, LPCSTR sh_name)
{
	SFlare F;
	F.fRadius = fRadius;
	F.fOpacity = fOpacity;
	F.fPosition = fPosition;
	F.shader = sh_name;
	F.texture = tex_name;
	m_Flares.push_back(F);
}

void CLensFlareDescriptor::load(CInifile *pIni, LPCSTR sect)
{
	section = sect;
	m_Flags.set(flSource, pIni->r_bool(sect, "source"));
	if (m_Flags.is(flSource))
	{
		LPCSTR S = pIni->r_string(sect, "source_shader");
		LPCSTR T = pIni->r_string(sect, "source_texture");
		float r = pIni->r_float(sect, "source_radius");
		BOOL i = pIni->r_bool(sect, "source_ignore_color");
		SetSource(r, i, T, S);
	}
	m_Flags.set(flFlare, pIni->r_bool(sect, "flares"));
	if (m_Flags.is(flFlare))
	{
		LPCSTR S = pIni->r_string(sect, "flare_shader");
		LPCSTR T = pIni->r_string(sect, "flare_textures");
		LPCSTR R = pIni->r_string(sect, "flare_radius");
		LPCSTR O = pIni->r_string(sect, "flare_opacity");
		LPCSTR P = pIni->r_string(sect, "flare_position");
		u32 tcnt = _GetItemCount(T);
		string256 name;
		for (u32 i = 0; i < tcnt; i++)
		{
			_GetItem(R, i, name);
			float r = (float)atof(name);
			_GetItem(O, i, name);
			float o = (float)atof(name);
			_GetItem(P, i, name);
			float p = (float)atof(name);
			_GetItem(T, i, name);
			AddFlare(r, o, p, name, S);
		}
	}
	m_Flags.set(flGradient, CInifile::IsBOOL(pIni->r_string(sect, "gradient")));
	if (m_Flags.is(flGradient))
	{
		LPCSTR S = pIni->r_string(sect, "gradient_shader");
		LPCSTR T = pIni->r_string(sect, "gradient_texture");
		float r = pIni->r_float(sect, "gradient_radius");
		float o = pIni->r_float(sect, "gradient_opacity");
		SetGradient(r, o, T, S);
	}
	m_StateBlendUpSpeed = 1.f / (_max(pIni->r_float(sect, "blend_rise_time"), 0.f) + EPS_S);
	m_StateBlendDnSpeed = 1.f / (_max(pIni->r_float(sect, "blend_down_time"), 0.f) + EPS_S);

	OnDeviceCreate();
}

void CLensFlareDescriptor::OnDeviceCreate()
{
	// shaders
	m_Gradient.m_pRender->CreateShader(*m_Gradient.shader, *m_Gradient.texture);
	m_Source.m_pRender->CreateShader(*m_Source.shader, *m_Source.texture);
	
	for (FlareIt it = m_Flares.begin(); it != m_Flares.end(); it++)
		it->m_pRender->CreateShader(*it->shader, *it->texture);	
}

void CLensFlareDescriptor::OnDeviceDestroy()
{
	// shaders
	m_Gradient.m_pRender->DestroyShader();
	m_Source.m_pRender->DestroyShader();

	for (FlareIt it = m_Flares.begin(); it != m_Flares.end(); it++)
		it->m_pRender->DestroyShader();
}

//------------------------------------------------------------------------------
CLensFlare::CLensFlare()
{
	// Device
	dwFrame = 0xfffffffe;

	fBlend = 0.f;

	LightColor.set(0xFFFFFFFF);
	fGradientValue = 0.f;

	m_Current = 0;

	m_State = lfsNone;
	m_StateBlend = 0.f;

	m_ray_cache.verts[0].set(0, 0, 0);
	m_ray_cache.verts[1].set(0, 0, 0);
	m_ray_cache.verts[2].set(0, 0, 0);

	OnDeviceCreate();
}

CLensFlare::~CLensFlare()
{
	OnDeviceDestroy();
	delete_data(m_Palette);
}

struct STranspParam
{
	Fvector P;
	Fvector D;
	float f;
	CLensFlare *parent;
	float vis;
	float vis_threshold;
	STranspParam(CLensFlare *p, const Fvector &_P, const Fvector &_D, float _f, float _vis_threshold) : P(_P), D(_D), f(_f), parent(p), vis(1.f), vis_threshold(_vis_threshold) {}
};

IC BOOL material_callback(collide::rq_result &result, LPVOID params)
{
	STranspParam *fp = (STranspParam *)params;
	float vis = 1.f;
	if (result.O)
	{
		vis = 0.f;
		IKinematics *K = PKinematics((IRenderVisual*)result.O->renderable.visual);		
		if (K && (result.element > 0))
			vis = g_pGamePersistent->MtlTransparent(K->LL_GetData(u16(result.element)).game_mtl_idx);
	}
	else
	{
		CDB::TRI *T = g_pGameLevel->ObjectSpace.GetStaticTris() + result.element;
		vis = g_pGamePersistent->MtlTransparent(T->material);
		if (fis_zero(vis))
		{
			Fvector *V = g_pGameLevel->ObjectSpace.GetStaticVerts();
			fp->parent->m_ray_cache.set(fp->P, fp->D, fp->f, TRUE);
			fp->parent->m_ray_cache.verts[0].set(V[T->verts[0]]);
			fp->parent->m_ray_cache.verts[1].set(V[T->verts[1]]);
			fp->parent->m_ray_cache.verts[2].set(V[T->verts[2]]);
		}
	}
	fp->vis *= vis;
	return (fp->vis > fp->vis_threshold);
}

IC void blend_lerp(float &cur, float tgt, float speed, float dt)
{
	float diff = tgt - cur;
	float diff_a = _abs(diff);
	if (diff_a < EPS_S)
		return;
	float mot = speed * dt;
	if (mot > diff_a)
		mot = diff_a;
	cur += (diff / diff_a) * mot;
}

void CLensFlare::OnFrame(int id)
{
	if (dwFrame == Device.CurrentFrameNumber)
		return;

	if (!g_pGameLevel)
		return;

	dwFrame = Device.CurrentFrameNumber;

	vSunDir.mul(g_pGamePersistent->Environment().CurrentEnv.sun_dir, -1);

	// color
	float tf = g_pGamePersistent->Environment().fTimeFactor;
	Fvector &c = g_pGamePersistent->Environment().CurrentEnv.sun_color;
	LightColor.set(c.x, c.y, c.z, 1.f);

	CLensFlareDescriptor *desc = (id == -1) ? 0 : &m_Palette[id];

	switch (m_State)
	{
	case lfsNone:
		m_State = lfsShow;
		m_Current = desc;
		break;
	case lfsIdle:
		if (desc != m_Current)
			m_State = lfsHide;
		break;
	case lfsShow:
		m_StateBlend = m_Current ? (m_StateBlend + m_Current->m_StateBlendUpSpeed * Device.fTimeDelta * tf) : 1.f + EPS;
		if (m_StateBlend >= 1.f)
			m_State = lfsIdle;
		break;
	case lfsHide:
		m_StateBlend = m_Current ? (m_StateBlend - m_Current->m_StateBlendDnSpeed * Device.fTimeDelta * tf) : 0.f - EPS;
		if (m_StateBlend <= 0.f)
		{
			m_State = lfsShow;
			m_Current = desc;
			m_StateBlend = m_Current ? m_Current->m_StateBlendUpSpeed * Device.fTimeDelta * tf : 0;
		}
		break;
	}
	clamp(m_StateBlend, 0.f, 1.f);

	if ((m_Current == 0) || (LightColor.magnitude_rgb() == 0.f))
	{
		bRender = false;
		return;
	}

	//
	// Compute center and axis of flares
	//
	float fDot;

	Fvector vecPos;

	Fmatrix matEffCamPos;
	matEffCamPos.identity();
	// Calculate our position and direction

	matEffCamPos.i.set(Device.vCameraRight);
	matEffCamPos.j.set(Device.vCameraTop);
	matEffCamPos.k.set(Device.vCameraDirection);
	vecPos.set(Device.vCameraPosition);

	vecDir.set(0.0f, 0.0f, 1.0f);
	matEffCamPos.transform_dir(vecDir);
	vecDir.normalize();

	// Figure out of light (or flare) might be visible
	vecLight.set(vSunDir);
	vecLight.normalize();

	fDot = vecLight.dotproduct(vecDir);

	if (fDot <= 0.01f)
	{
		bRender = false;
		return;
	}
	else
		bRender = true;

	// Calculate the point directly in front of us, on the far clip plane
	float fDistance = FAR_DIST * 0.75f;
	vecCenter.mul(vecDir, fDistance);
	vecCenter.add(vecPos);
	// Calculate position of light on the far clip plane
	vecLight.mul(fDistance / fDot);
	vecLight.add(vecPos);
	// Compute axis which goes from light through the center of the screen
	vecAxis.sub(vecLight, vecCenter);

	//
	// Figure out if light is behind something else
	vecX.set(1.0f, 0.0f, 0.0f);
	matEffCamPos.transform_dir(vecX);
	vecX.normalize();
	vecY.crossproduct(vecX, vecDir);

	CObject *o_main = g_pGameLevel->CurrentViewEntity();
	STranspParam TP(this, Device.vCameraPosition, vSunDir, 1000.f, EPS_L);
	collide::ray_defs RD(TP.P, TP.D, TP.f, CDB::OPT_CULL, collide::rqtBoth);
	if (m_ray_cache.result && m_ray_cache.similar(TP.P, TP.D, TP.f))
	{
		// similar with previous query == 0
		TP.vis = 0.f;
	}
	else
	{
		float _u, _v, _range;
		if (CDB::TestRayTri(TP.P, TP.D, m_ray_cache.verts, _u, _v, _range, false) && (_range > 0 && _range < TP.f))
		{
			TP.vis = 0.f;
		}
		else
		{
			// cache outdated. real query.
			r_dest.r_clear();
			if (g_pGameLevel->ObjectSpace.RayQuery(r_dest, RD, material_callback, &TP, NULL, o_main))
				m_ray_cache.result = FALSE;
		}
	}
	blend_lerp(fBlend, TP.vis, BLEND_DEC_SPEED, Device.fTimeDelta);

	clamp(fBlend, 0.0f, 1.0f);

	// gradient
	if (m_Current->m_Flags.is(CLensFlareDescriptor::flGradient))
	{
		Fvector scr_pos;
		Device.mFullTransform.transform(scr_pos, vecLight);
		float kx = 1, ky = 1;
		float sun_blend = 0.5f;
		float sun_max = 2.5f;
		scr_pos.y *= -1;

		if (_abs(scr_pos.x) > sun_blend)
			kx = ((sun_max - (float)_abs(scr_pos.x))) / (sun_max - sun_blend);
		if (_abs(scr_pos.y) > sun_blend)
			ky = ((sun_max - (float)_abs(scr_pos.y))) / (sun_max - sun_blend);

		if (!((_abs(scr_pos.x) > sun_max) || (_abs(scr_pos.y) > sun_max)))
		{
			float op = m_StateBlend * m_Current->m_Gradient.fOpacity;
			fGradientValue = kx * ky * op * fBlend;
		}
		else
			fGradientValue = 0;
	}
}

void CLensFlare::Render(BOOL bSun, BOOL bFlares, BOOL bGradient)
{
	if (!bRender)
		return;
	if (!m_Current)
		return;
	VERIFY(m_Current);

	m_pRender->Render(*this, bSun, bFlares, bGradient);
}

int CLensFlare::AppendDef(CInifile *pIni, LPCSTR sect)
{
	if (!sect || (0 == sect[0]))
		return -1;
	for (LensFlareDescIt it = m_Palette.begin(); it != m_Palette.end(); it++)
		if (0 == xr_strcmp(*it->section, sect))
			return int(it - m_Palette.begin());
	m_Palette.push_back(CLensFlareDescriptor());
	CLensFlareDescriptor &lf = m_Palette.back();
	lf.load(pIni, sect);
	return m_Palette.size() - 1;
}

void CLensFlare::OnDeviceCreate()
{
	// VS
	m_pRender->OnDeviceCreate();

	// palette
	for (LensFlareDescIt it = m_Palette.begin(); it != m_Palette.end(); it++)
		it->OnDeviceCreate();
}

void CLensFlare::OnDeviceDestroy()
{
	// palette
	for (LensFlareDescIt it = m_Palette.begin(); it != m_Palette.end(); it++)
		it->OnDeviceDestroy();

	// VS
	m_pRender->OnDeviceDestroy();
}
