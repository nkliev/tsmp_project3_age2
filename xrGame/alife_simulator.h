////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_simulator.h
//	Created 	: 25.12.2002
//  Modified 	: 13.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife Simulator
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "alife_update_manager.h"
#include "script_export_space.h"

#pragma warning(push)
#pragma warning(disable : 4005)

class CALifeSimulator : public CALifeUpdateManager
{
protected:
	virtual void setup_simulator(CSE_ALifeObject *object);
	virtual void reload(LPCSTR section);

public:
	CALifeSimulator(xrServer *server, shared_str *command_line);
	virtual ~CALifeSimulator();
	virtual void destroy();

	void kill_entity(CSE_ALifeMonsterAbstract* l_tpALifeMonsterAbstract, const GameGraph::_GRAPH_ID& l_tGraphID, CSE_ALifeSchedulable* schedulable);

#if 0  //def DEBUG
			void	validate			();
#endif //DEBUG

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CALifeSimulator)
#define script_type_list save_type_list(CALifeSimulator)

#pragma warning(pop)

#include "alife_simulator_inline.h"