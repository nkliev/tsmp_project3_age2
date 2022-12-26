#pragma once

class IKinematics;
class IKinematicsAnimated;
//class IParticleCustom; // TSMP: TODO

struct vis_data;

class ENGINE_API IRenderVisual
{
public:
	virtual ~IRenderVisual() { ; }

	virtual vis_data &getVisData() = 0;
	virtual u32 getType() = 0;

#ifdef DEBUG
	virtual shared_str getDebugName() = 0;
#endif

	virtual IKinematics *dcast_PKinematics() { return nullptr; }
	virtual IKinematicsAnimated *dcast_PKinematicsAnimated() { return nullptr; }
	//virtual IParticleCustom *dcast_ParticleCustom() { return nullptr; } // TSMP: TODO
};
