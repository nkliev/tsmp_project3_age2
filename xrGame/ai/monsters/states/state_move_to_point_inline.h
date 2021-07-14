#pragma once

#define TEMPLATE_SPECIALIZATION template < \
	typename _Object>

#define CStateMonsterMoveToPointAbstract CStateMonsterMoveToPoint<_Object>

TEMPLATE_SPECIALIZATION
void CStateMonsterMoveToPointAbstract::initialize()
{
	inherited::initialize();
	object->path().prepare_builder();
}

TEMPLATE_SPECIALIZATION
void CStateMonsterMoveToPointAbstract::execute()
{
	object->set_action(data.action.action);
	object->anim().SetSpecParams(data.action.spec_params);

	object->path().set_target_point(data.point, data.vertex);
	object->path().set_generic_parameters();
	object->path().set_distance_to_end(data.completion_dist);

	if (data.accelerated)
	{
		object->anim().accel_activate(EAccelType(data.accel_type));
		object->anim().accel_set_braking(data.braking);
	}

	if (data.action.sound_type != u32(-1))	
		object->set_state_sound(data.action.sound_type, 0, data.action.sound_delay == u32(-1));	
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterMoveToPointAbstract::check_completion()
{
	if (data.action.time_out != 0)
	{
		if (time_state_started + data.action.time_out < Device.dwTimeGlobal)
			return true;
	}

	bool real_path_end = ((fis_zero(data.completion_dist)) ? (data.point.distance_to_xz(object->Position()) < ai().level_graph().header().cell_size()) : true);
	if (object->control().path_builder().is_path_end(data.completion_dist) && real_path_end)
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////
// CStateMonsterMoveToPointEx with path rebuild options
//////////////////////////////////////////////////////////////////////////

#define CStateMonsterMoveToPointExAbstract CStateMonsterMoveToPointEx<_Object>

TEMPLATE_SPECIALIZATION
void CStateMonsterMoveToPointExAbstract::initialize()
{
	inherited::initialize();
	object->path().prepare_builder();
}

TEMPLATE_SPECIALIZATION
void CStateMonsterMoveToPointExAbstract::execute()
{
	object->set_action(data.action.action);
	object->anim().SetSpecParams(data.action.spec_params);

	object->path().set_target_point(data.point, data.vertex);
	object->path().set_rebuild_time(data.time_to_rebuild);
	object->path().set_distance_to_end(data.completion_dist);
	object->path().set_use_covers();
	object->path().set_cover_params(5.f, 30.f, 1.f, 30.f);

	if (data.accelerated)
	{
		object->anim().accel_activate(EAccelType(data.accel_type));
		object->anim().accel_set_braking(data.braking);
	}

	if (data.action.sound_type != u32(-1))	
		object->set_state_sound(data.action.sound_type, 0, data.action.sound_delay == u32(-1));	
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterMoveToPointExAbstract::check_completion()
{
	if (data.action.time_out != 0)
	{
		if (time_state_started + data.action.time_out < Device.dwTimeGlobal)
			return true;
	}

	bool real_path_end = ((fis_zero(data.completion_dist)) ? (data.point.distance_to_xz(object->Position()) < ai().level_graph().header().cell_size()) : true);
	if (object->control().path_builder().is_path_end(data.completion_dist) && real_path_end)
		return true;

	return false;
}
