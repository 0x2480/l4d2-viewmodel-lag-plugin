//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $NoKeywords: $
//===========================================================================//

#ifndef ICONVAR_L4D_H
#define ICONVAR_L4D_H

#if _WIN32
#pragma once
#endif

#include "tier0/dbg.h"
#include "tier0/platform.h"
#include "tier1/strtools.h"
#include "iconvar.h"

//-----------------------------------------------------------------------------
// Abstract interface for ConVars
//-----------------------------------------------------------------------------
abstract_class IConVar_L4D
{
public:
	// Value set
	virtual void SetValue( const char *pValue ) = 0;
	virtual void SetValue( float flValue ) = 0;
	virtual void SetValue( int nValue ) = 0;

	// Return name of command
	virtual const char *GetName( void ) const = 0;

	// Return name of command (usually == GetName(), except in case of FCVAR_SS_ADDED vars
	virtual const char *GetBaseName( void ) const = 0;

	// Accessors.. not as efficient as using GetState()/GetInfo()
	// if you call these methods multiple times on the same IConVar
	virtual bool IsFlagSet( int nFlag ) const = 0;

	virtual int GetSplitScreenPlayerSlot() const = 0;
};


#endif // ICONVAR_L4D_H
