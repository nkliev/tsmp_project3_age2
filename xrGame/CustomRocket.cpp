//////////////////////////////////////////////////////////////////////
// CustomRocket.cpp:	������, ������� �������� RocketLauncher
//						(����� ������, ��������� � ���������� ��������)
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "customrocket.h"
#include "ParticlesObject.h"
#include "PhysicsShell.h"
#include "extendedgeom.h"
#include "level.h"
#include "xrMessages.h"
#include "gamemtllib.h"
#include "tri-colliderknoopc/dTriList.h"
#include "..\Include\xrRender\RenderVisual.h"
#include "CalculateTriangle.h"
#include "actor.h"
#include "holder_custom.h"

#ifdef DEBUG
#include "PHDebug.h"
#include "game_base_space.h"
#endif

#define CHOOSE_MAX(x, inst_x, y, inst_y, z, inst_z) \
	if (x > y)                                      \
		if (x > z)                                  \
		{                                           \
			inst_x;                                 \
		}                                           \
		else                                        \
		{                                           \
			inst_z;                                 \
		}                                           \
	else if (y > z)                                 \
	{                                               \
		inst_y;                                     \
	}                                               \
	else                                            \
	{                                               \
		inst_z;                                     \
	}
CCustomRocket::CCustomRocket()
{
	m_eState = eInactive;
	m_bEnginePresent = false;
	m_bStopLightsWithEngine = true;
	m_bLightsEnabled = false;

	m_vPrevVel.set(0, 0, 0);

	m_pTrailLight = NULL;
	m_LaunchXForm.identity();
	m_vLaunchVelocity.set(0, 0, 0);
	m_vLaunchAngularVelocity.set(0, 0, 0);
	m_bLaunched = false;
}

CCustomRocket::~CCustomRocket()
{
	m_pTrailLight.destroy();
}

void CCustomRocket::reinit()
{
	inherited::reinit();

	m_pTrailLight.destroy();
	m_pTrailLight = ::Render->light_create();
	m_pTrailLight->set_shadow(true);

	m_pEngineParticles = NULL;
	m_pFlyParticles = NULL;

	m_pOwner = NULL;

	m_vPrevVel.set(0, 0, 0);
}

BOOL CCustomRocket::net_Spawn(CSE_Abstract *DC)
{
	m_eState = eInactive;
	BOOL result = inherited::net_Spawn(DC);
	m_LaunchXForm.set(XFORM());
	return result;
}

void CCustomRocket::net_Destroy()
{
	//	Msg("---------net_Destroy [%d] frame[%d]",ID(), Device.CurrentFrameNumber);
	inherited::net_Destroy();
	CPHUpdateObject::Deactivate();

	StopEngine();
	StopFlying();
}

void CCustomRocket::SetLaunchParams(const Fmatrix &xform,
									const Fvector &vel,
									const Fvector &angular_vel)
{
	VERIFY2(_valid(xform), "SetLaunchParams. Invalid xform argument!");
	m_LaunchXForm = xform;
	m_vLaunchVelocity = vel;
	//	if(m_pOwner->ID()==Actor()->ID())
	//	{
	//		Msg("set p start v:	%f,%f,%f	\n",m_vLaunchVelocity.x,m_vLaunchVelocity.y,m_vLaunchVelocity.z);
	//	}
	m_vLaunchAngularVelocity = angular_vel;
	m_time_to_explode = Device.fTimeGlobal + pSettings->r_float(cNameSect(), "force_explode_time") / 1000.0f;
#ifdef DEBUG
	gbg_rocket_speed1 = 0;
	gbg_rocket_speed2 = 0;
#endif
}

void CCustomRocket::activate_physic_shell()
{
	VERIFY(H_Parent());
	VERIFY(!m_pPhysicsShell);
	create_physic_shell();
	if (m_pPhysicsShell->isActive())
		return;
	VERIFY2(_valid(m_LaunchXForm), "CCustomRocket::activate_physic_shell. Invalid m_LaunchXForm!");

	//	if(m_pOwner->ID()==Actor()->ID())
	//	{
	//		Msg("start v:	%f,%f,%f	\n",m_vLaunchVelocity.x,m_vLaunchVelocity.y,m_vLaunchVelocity.z);
	//	}
	m_pPhysicsShell->Activate(m_LaunchXForm, m_vLaunchVelocity, m_vLaunchAngularVelocity);
	m_pPhysicsShell->Update();

	XFORM().set(m_pPhysicsShell->mXFORM);
	Position().set(m_pPhysicsShell->mXFORM.c);
	m_pPhysicsShell->set_PhysicsRefObject(this);
	m_pPhysicsShell->set_ObjectContactCallback(ObjectContactCallback);
	m_pPhysicsShell->set_ContactCallback(NULL);
	m_pPhysicsShell->SetAirResistance(0.f, 0.f);
	m_pPhysicsShell->set_DynamicScales(1.f, 1.f);
	m_pPhysicsShell->SetAllGeomTraced();
}

void CCustomRocket::create_physic_shell()
{
	VERIFY(!m_pPhysicsShell);
	Fobb obb;
	Visual()->getVisData().box.get_CD(obb.m_translate, obb.m_halfsize);
	obb.m_rotate.identity();

	// Physics (Elements)
	CPhysicsElement *E = P_create_Element();
	R_ASSERT(E);

	Fvector ax;
	float radius;
	CHOOSE_MAX(
		obb.m_halfsize.x, ax.set(obb.m_rotate.i); ax.mul(obb.m_halfsize.x); radius = _min(obb.m_halfsize.y, obb.m_halfsize.z); obb.m_halfsize.y /= 2.f; obb.m_halfsize.z /= 2.f,
																																						obb.m_halfsize.y, ax.set(obb.m_rotate.j);
		ax.mul(obb.m_halfsize.y); radius = _min(obb.m_halfsize.x, obb.m_halfsize.z); obb.m_halfsize.x /= 2.f; obb.m_halfsize.z /= 2.f,
																											  obb.m_halfsize.z, ax.set(obb.m_rotate.k);
		ax.mul(obb.m_halfsize.z); radius = _min(obb.m_halfsize.y, obb.m_halfsize.x); obb.m_halfsize.y /= 2.f; obb.m_halfsize.x /= 2.f)
	//radius*=1.4142f;
	Fsphere sphere1, sphere2;
	sphere1.P.add(obb.m_translate, ax);
	sphere1.R = radius * 1.4142f;

	sphere2.P.sub(obb.m_translate, ax);
	sphere2.R = radius / 2.f;

	E->add_Box(obb);
	E->add_Sphere(sphere1);
	E->add_Sphere(sphere2);

	// Physics (Shell)
	m_pPhysicsShell = P_create_Shell();
	R_ASSERT(m_pPhysicsShell);
	m_pPhysicsShell->add_Element(E);
	m_pPhysicsShell->setMass(7.f);
	m_pPhysicsShell->SetAirResistance();
}

//////////////////////////////////////////////////////////////////////////
// Rocket specific functions
//////////////////////////////////////////////////////////////////////////

void CCustomRocket::ObjectContactCallback(bool& do_colide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2)
{
	do_colide = false;

	dxGeomUserData* geomData1 = retrieveGeomUserData(c.geom.g1);
	dxGeomUserData* geomData2 = retrieveGeomUserData(c.geom.g2);

	SGameMtl* material = 0;
	CCustomRocket* rocket = geomData1 ? smart_cast<CCustomRocket*>(geomData1->ph_ref_object) : nullptr;
	Fvector vUp;

	if (!rocket)
	{
		rocket = geomData2 ? smart_cast<CCustomRocket*>(geomData2->ph_ref_object) : nullptr;
		vUp.invert(*(Fvector*)&c.geom.normal);
		material = material_1;
	}
	else
	{
		vUp.set(*(Fvector*)&c.geom.normal);
		material = material_2;
	}

	VERIFY(material);
	if (material->Flags.is(SGameMtl::flPassable))
		return;

	if (!rocket || rocket->m_contact.contact)
		return;

	CGameObject* contactedObj = geomData1 ? smart_cast<CGameObject*>(geomData1->ph_ref_object) : nullptr;
	if (!contactedObj || contactedObj == (CGameObject*)rocket)
		contactedObj = geomData2 ? smart_cast<CGameObject*>(geomData2->ph_ref_object) : nullptr;

	if (contactedObj && contactedObj == rocket->m_pOwner)
		return;

	if (!rocket->m_pOwner)
		return;

	if (auto act = smart_cast<CActor*>(rocket->m_pOwner))
	{
		if (smart_cast<CGameObject*>(act->Holder()) == contactedObj)
			return;
	}

	Fvector contactPos;
	contactPos.set(rocket->Position());

#ifdef DEBUG
	bool corrected_pos = false;
#endif

	if (!geomData1 || !geomData2)
	{
		dxGeomUserData*& geom = geomData1 ? geomData1 : geomData2;
		dGeomID g = geomData1 ? c.geom.g1 : c.geom.g2;

		if (geom->pushing_neg)
		{
			Fvector velocity;
			rocket->PHGetLinearVell(velocity);

			if (velocity.square_magnitude() > EPS)
			{
				// desync?
				velocity.normalize();
				Triangle neg_tri;
				CalculateTriangle(geom->neg_tri, g, neg_tri);
				float cosinus = velocity.dotproduct(*((Fvector*)neg_tri.norm));
				VERIFY(_valid(neg_tri.dist));
				float dist = neg_tri.dist / cosinus;
				velocity.mul(dist * 1.1f);
				contactPos.sub(velocity);

#ifdef DEBUG
				corrected_pos = true;
#endif
			}
		}
	}

#ifdef DEBUG
	if (ph_dbg_draw_mask.test(phDbgDrawExplosionPos))
		DBG_DrawPoint(contactPos, 0.05f, D3DCOLOR_XRGB(255, 255, (!corrected_pos) * 255));
#endif

	rocket->Contact(contactPos, vUp);
	rocket->m_pPhysicsShell->DisableCollision();
	rocket->m_pPhysicsShell->set_LinearVel(Fvector().set(0, 0, 0));
	rocket->m_pPhysicsShell->set_AngularVel(Fvector().set(0, 0, 0));
	rocket->m_pPhysicsShell->setForce(Fvector().set(0, 0, 0));
	rocket->m_pPhysicsShell->setTorque(Fvector().set(0, 0, 0));
	rocket->m_pPhysicsShell->set_ApplyByGravity(false);
	rocket->setEnabled(FALSE);
}

void CCustomRocket::Load(LPCSTR section)
{
	inherited::Load(section);

	reload(section);
}

void CCustomRocket::reload(LPCSTR section)
{
	inherited::reload(section);
	m_eState = eInactive;

	m_bEnginePresent = !!pSettings->r_bool(section, "engine_present");
	if (m_bEnginePresent)
	{
		m_dwEngineWorkTime = pSettings->r_u32(section, "engine_work_time");
		m_fEngineImpulse = pSettings->r_float(section, "engine_impulse");
		m_fEngineImpulseUp = pSettings->r_float(section, "engine_impulse_up");
	}

	m_bLightsEnabled = !!pSettings->r_bool(section, "lights_enabled");
	if (m_bLightsEnabled)
	{
		sscanf(pSettings->r_string(section, "trail_light_color"), "%f,%f,%f",
			   &m_TrailLightColor.r, &m_TrailLightColor.g, &m_TrailLightColor.b);
		m_fTrailLightRange = pSettings->r_float(section, "trail_light_range");
	}

	if (pSettings->line_exist(section, "engine_particles"))
		m_sEngineParticles = pSettings->r_string(section, "engine_particles");
	if (pSettings->line_exist(section, "fly_particles"))
		m_sFlyParticles = pSettings->r_string(section, "fly_particles");

	if (pSettings->line_exist(section, "snd_fly_sound"))
	{
		m_flyingSound.create(pSettings->r_string(section, "snd_fly_sound"), st_Effect, sg_SourceType);
	}
}

void CCustomRocket::Contact(const Fvector &pos, const Fvector &normal)
{
	m_contact.contact = true;
	m_contact.pos.set(pos);
	m_contact.up.set(normal);
}
void CCustomRocket::PlayContact()
{

	if (!m_contact.contact)
		return;
	if (eCollide == m_eState)
		return;

	StopEngine();
	StopFlying();

	m_eState = eCollide;

	//������������� ���������� ��������,���� ������ �� ������ ������
	if (m_pPhysicsShell)
	{
		m_pPhysicsShell->set_LinearVel(zero_vel);
		m_pPhysicsShell->set_AngularVel(zero_vel);
		m_pPhysicsShell->set_ObjectContactCallback(NULL);
		m_pPhysicsShell->Disable();
	}
	//	if (OnClient()) return;

	Position().set(m_contact.pos);
	m_contact.contact = false;
}

void CCustomRocket::OnH_B_Chield()
{
	VERIFY(m_eState == eInactive);
	inherited::OnH_B_Chield();
	//	Msg("! CCustomRocket::OnH_B_Chield called, id[%d] frame[%d]",ID(),Device.CurrentFrameNumber);
}
void CCustomRocket::OnH_A_Chield()
{
	VERIFY(m_eState == eInactive);
	inherited::OnH_A_Chield();
	//	Msg("! CCustomRocket::OnH_A_Chield called, id[%d] frame[%d]",ID(),Device.CurrentFrameNumber);
}

void CCustomRocket::OnH_B_Independent(bool just_before_destroy)
{
	inherited::OnH_B_Independent(just_before_destroy);
	//-------------------------------------------
	m_pOwner = H_Parent() ? smart_cast<CGameObject *>(H_Parent()->H_Root()) : NULL;
	//-------------------------------------------
}

void CCustomRocket::OnH_A_Independent()
{
	inherited::OnH_A_Independent();

	if (!g_pGameLevel->bReady || !m_bLaunched)
		return;
	setVisible(true);
	StartFlying();
	StartEngine();
	//	Msg("! CCustomRocket::OnH_A_Independent called, id[%d] frame[%d]",ID(),Device.CurrentFrameNumber);
}

void CCustomRocket::UpdateCL()
{
	inherited::UpdateCL();

	PlayContact();
	switch (m_eState)
	{
	case eInactive:
		break;
	//��������� eEngine � eFlying ����������, ���
	//��� ���������� UpdateEngine � eEngine, ���������
	//������� �����
	case eEngine:
		UpdateEngine();
	case eFlying:
		UpdateLights();
		UpdateParticles();
		break;
	}
	if (m_eState == eEngine || m_eState == eFlying)
	{
		if (m_time_to_explode < Device.fTimeGlobal)
		{
			Contact(Position(), Direction());
			//			Msg("--contact");
		}
	}
}

void CCustomRocket::StartEngine()
{
	VERIFY(NULL == H_Parent());

	if (!m_bEnginePresent)
	{
		m_eState = eFlying;
		return;
	}

	m_eState = eEngine;
	m_dwEngineTime = m_dwEngineWorkTime;

	StartEngineParticles();

	CPHUpdateObject::Activate();
}

void CCustomRocket::StopEngine()
{
	m_eState = eFlying;

	m_dwEngineTime = 0;

	if (m_bStopLightsWithEngine)
		StopLights();

	StopEngineParticles();

	CPHUpdateObject::Deactivate();
}

void CCustomRocket::UpdateEnginePh()
{
	if (Level().In_NetCorrectionPrediction())
		return;
	float force = m_fEngineImpulse * fixed_step; // * Device.fTimeDelta;
	float k_back = 1.f;
	Fvector l_pos, l_dir;
	l_pos.set(0, 0, -2.f);
	l_dir.set(XFORM().k);

	l_dir.normalize();
	m_pPhysicsShell->applyImpulse(l_dir, (1.f + k_back) * force);
	m_pPhysicsShell->get_LinearVel(l_dir);
	l_dir.normalize_safe();
	l_dir.invert();
	m_pPhysicsShell->applyImpulseTrace(l_pos, l_dir, force);
	l_dir.set(0, 1.f, 0);
	force = m_fEngineImpulseUp * fixed_step; // * Device.fTimeDelta;
	m_pPhysicsShell->applyImpulse(l_dir, force);

	//m_pPhysicsShell->set_AngularVel()
}

void CCustomRocket::UpdateEngine()
{
	//	VERIFY( getVisible() );
	//	VERIFY( m_pPhysicsShell);
	if (!m_pPhysicsShell)
		Msg("! CCustomRocket::UpdateEngine called, but 0==m_pPhysicsShell");

	if (!getVisible())
	{
		Msg("! CCustomRocket::UpdateEngine called, but false==getVisible() id[%d] frame[%d]", ID(), Device.CurrentFrameNumber);
	}

	if (m_dwEngineTime <= 0)
	{

		StopEngine();
		return;
	}

	m_dwEngineTime -= Device.dwTimeDelta;
}

//////////////////////////////////////////////////////////////////////////
//	Lights
//////////////////////////////////////////////////////////////////////////
void CCustomRocket::StartLights()
{
	if (!m_bLightsEnabled)
		return;

	//�������� �������� ��������� �� ���������
	m_pTrailLight->set_color(m_TrailLightColor.r,
							 m_TrailLightColor.g,
							 m_TrailLightColor.b);

	m_pTrailLight->set_range(m_fTrailLightRange);
	m_pTrailLight->set_position(Position());
	m_pTrailLight->set_active(true);
}

void CCustomRocket::StopLights()
{
	if (!m_bLightsEnabled)
		return;
	m_pTrailLight->set_active(false);
}

void CCustomRocket::UpdateLights()
{
	if (!m_bLightsEnabled || !m_pTrailLight->get_active())
		return;
	m_pTrailLight->set_position(Position());
}

void CCustomRocket::PhDataUpdate(float step)
{
}
void CCustomRocket::PhTune(float step)
{
	UpdateEnginePh();
}

//////////////////////////////////////////////////////////////////////////
//	Particles
//////////////////////////////////////////////////////////////////////////

void CCustomRocket::UpdateParticles()
{
	if (m_flyingSound._handle() && m_flyingSound._feedback())
		m_flyingSound.set_position(XFORM().c);

	if (!m_pEngineParticles && !m_pFlyParticles)
		return;

	Fvector vel;
	PHGetLinearVell(vel);

	vel.add(m_vPrevVel, vel);
	vel.mul(0.5f);
	m_vPrevVel.set(vel);

	Fmatrix particles_xform;
	particles_xform.identity();
	particles_xform.k.set(XFORM().k);
	particles_xform.k.mul(-1.f);
	Fvector::generate_orthonormal_basis(particles_xform.k,
										particles_xform.j,
										particles_xform.i);
	particles_xform.c.set(XFORM().c);

	if (m_pEngineParticles)
		m_pEngineParticles->UpdateParent(particles_xform, vel);
	if (m_pFlyParticles)
		m_pFlyParticles->UpdateParent(particles_xform, vel);
}

void CCustomRocket::StartEngineParticles()
{
	VERIFY(m_pEngineParticles == NULL);
	if (!m_sEngineParticles)
		return;
	m_pEngineParticles = CParticlesObject::Create(*m_sEngineParticles, FALSE);

	UpdateParticles();
	m_pEngineParticles->Play();

	VERIFY(m_pEngineParticles);
	VERIFY3(m_pEngineParticles->IsLooped(), "must be a looped particle system for rocket engine: %s", *m_sEngineParticles);
}
void CCustomRocket::StopEngineParticles()
{
	if (m_pEngineParticles == NULL)
		return;
	m_pEngineParticles->Stop();
	m_pEngineParticles->SetAutoRemove(true);
	m_pEngineParticles = NULL;
}
void CCustomRocket::StartFlyParticles()
{
	if (m_flyingSound._handle())
		m_flyingSound.play_at_pos(0, XFORM().c, sm_Looped);

	VERIFY(m_pFlyParticles == NULL);

	if (!m_sFlyParticles)
		return;
	m_pFlyParticles = CParticlesObject::Create(*m_sFlyParticles, FALSE);

	UpdateParticles();
	m_pFlyParticles->Play();

	VERIFY(m_pFlyParticles);
	VERIFY3(m_pFlyParticles->IsLooped(), "must be a looped particle system for rocket fly: %s", *m_sFlyParticles);
}
void CCustomRocket::StopFlyParticles()
{
	if (m_flyingSound._handle())
		m_flyingSound.stop();

	if (m_pFlyParticles == NULL)
		return;
	m_pFlyParticles->Stop();
	m_pFlyParticles->SetAutoRemove(true);
	m_pFlyParticles = NULL;
}

void CCustomRocket::StartFlying()
{
	StartFlyParticles();
	StartLights();
}
void CCustomRocket::StopFlying()
{
	StopFlyParticles();
	StopLights();
}

void CCustomRocket::OnEvent(NET_Packet &P, u16 type)
{
	switch (type)
	{
	case GE_GRENADE_EXPLODE:
	{
		if (m_eState != eCollide && OnClient())
		{
			CCustomRocket::Contact(Position(), Direction());
		};
	}
	break;
	}
	inherited::OnEvent(P, type);
};