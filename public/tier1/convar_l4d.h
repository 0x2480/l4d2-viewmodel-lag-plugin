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

#ifndef CONVARL4D_H
#define CONVARL4D_H

#if _WIN32
#pragma once
#endif

#include "tier0/dbg.h"
#include "tier1/iconvar_l4d.h"
#include "tier1/utlvector.h"
#include "tier1/utlstring.h"
#include "icvar.h"
#include "convar.h"

//-----------------------------------------------------------------------------
// Purpose: A console variable
//-----------------------------------------------------------------------------
class ConVar_L4D : public ConCommandBase, public IConVar_L4D
{
friend class CCvar;
friend class ConVarRef;


public:
	typedef ConCommandBase BaseClass;

								ConVar_L4D( const char *pName, const char *pDefaultValue, int flags = 0);

								ConVar_L4D( const char *pName, const char *pDefaultValue, int flags, 
									const char *pHelpString );
								ConVar_L4D( const char *pName, const char *pDefaultValue, int flags, 
									const char *pHelpString, bool bMin, float fMin, bool bMax, float fMax );
								ConVar_L4D( const char *pName, const char *pDefaultValue, int flags, 
									const char *pHelpString, FnChangeCallback_t callback );
								ConVar_L4D( const char *pName, const char *pDefaultValue, int flags, 
									const char *pHelpString, bool bMin, float fMin, bool bMax, float fMax,
									FnChangeCallback_t callback );

	virtual						~ConVar_L4D( void );

	virtual	bool				IsCommand( void ) const;
	virtual bool				IsFlagSet( int flag ) const;
	virtual void				AddFlags( int flags );
	virtual int					GetFlags( void ) const;
	virtual const char			*GetName( void ) const;
	virtual const char*			GetHelpText( void ) const;
	virtual bool				IsRegistered( void ) const;
		
	// Install a change callback (there shouldn't already be one....)
	void InstallChangeCallback( FnChangeCallback_t callback );

	// Retrieve value
	FORCEINLINE_CVAR float			GetFloat( void ) const;
	FORCEINLINE_CVAR int			GetInt( void ) const;
	FORCEINLINE_CVAR bool			GetBool() const {  return !!GetInt(); }
	FORCEINLINE_CVAR char const	   *GetString( void ) const;
	
	// Used internally by OneTimeInit to initialize.
	virtual void				Init();
	
	virtual const char 			*GetBaseName( void ) const;
	virtual int					GetSplitScreenPlayerSlot ( void ) const;

	// Any function that allocates/frees memory needs to be virtual or else you'll have crashes
	//  from alloc/free across dll/exe boundaries.
	
	// These just call into the IConCommandBaseAccessor to check flags and set the var (which ends up calling InternalSetValue).
	virtual void				SetValue( const char *value );
	virtual void				SetValue( float value );
	virtual void				SetValue( int value );
	
	// Reset to default value
	void						Revert( void );

	// True if it has a min/max setting
	bool						GetMin( float& minVal ) const;
	bool						GetMax( float& maxVal ) const;
	const char					*GetDefault( void ) const;

private:
	// Called by CCvar when the value of a var is changing.
	virtual void				InternalSetValue(const char *value);
	// For CVARs marked FCVAR_NEVER_AS_STRING
	virtual void				InternalSetFloatValue( float fNewValue );
	virtual void				InternalSetIntValue( int nValue );

	virtual bool				ClampValue( float& value );
	virtual void				ChangeStringValue( const char *tempVal, float flOldValue );

	virtual void				Create( const char *pName, const char *pDefaultValue, int flags = 0,
									const char *pHelpString = 0, bool bMin = false, float fMin = 0.0,
									bool bMax = false, float fMax = false, FnChangeCallback_t callback = 0 );

private:

	// This either points to "this" or it points to the original declaration of a ConVar_L4D.
	// This allows ConVars to exist in separate modules, and they all use the first one to be declared.
	// m_pParent->m_pParent must equal m_pParent (ie: m_pParent must be the root, or original, ConVar_L4D).
	ConVar_L4D						*m_pParent;

	// Static data
	const char					*m_pszDefaultValue;
	
	// Value
	// Dynamically allocated
	char						*m_pszString;
	int							m_StringLength;

	// Values
	float						m_fValue;
	int							m_nValue;

	// Min/Max values
	bool						m_bHasMin;
	float						m_fMinVal;
	bool						m_bHasMax;
	float						m_fMaxVal;
	
	// Call this function when ConVar_L4D changes
	FnChangeCallback_t			m_fnChangeCallback;
};


//-----------------------------------------------------------------------------
// Purpose: Return ConVar_L4D value as a float
// Output : float
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR float ConVar_L4D::GetFloat( void ) const
{
	return m_pParent->m_fValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar_L4D value as an int
// Output : int
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR int ConVar_L4D::GetInt( void ) const 
{
	return m_pParent->m_nValue;
}


//-----------------------------------------------------------------------------
// Purpose: Return ConVar_L4D value as a string, return "" for bogus string pointer, etc.
// Output : const char *
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR const char *ConVar_L4D::GetString( void ) const 
{
	if ( m_nFlags & FCVAR_NEVER_AS_STRING )
		return "FCVAR_NEVER_AS_STRING";

	return ( m_pParent->m_pszString ) ? m_pParent->m_pszString : "";
}

#endif // CONVARL4D_H
