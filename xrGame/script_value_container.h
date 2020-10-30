////////////////////////////////////////////////////////////////////////////
//	Module 		: script_value_container.h
//	Created 	: 16.07.2004
//  Modified 	: 16.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Script value container
////////////////////////////////////////////////////////////////////////////

#pragma once

class CScriptValue
{
public:
	virtual ~CScriptValue() {}
};

class CScriptValueContainer
{
protected:
	xr_vector<CScriptValue *> m_values;

public:
	virtual ~CScriptValueContainer();
	IC void assign();
	IC void clear();
	IC void add(CScriptValue *value);
};
