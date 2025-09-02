//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "basetypes.h"
#include "tier1/convar_l4d.h"
#include "tier1/strtools.h"
#include "tier1/characterset.h"
#include "tier1/utlbuffer.h"
#include "tier1/tier1.h"
#include "tier1/convar_serverbounded.h"
#include "icvar.h"
#include "tier0/dbg.h"
#include "Color.h"
#if defined( _X360 )
#include "xbox/xbox_console.h"
#endif
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
//
// Console Variables
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Various constructors
//-----------------------------------------------------------------------------
ConVar_L4D::ConVar_L4D( const char *pName, const char *pDefaultValue, int flags /* = 0 */ )
{
	Create( pName, pDefaultValue, flags );
}

ConVar_L4D::ConVar_L4D( const char *pName, const char *pDefaultValue, int flags, const char *pHelpString )
{
	Create( pName, pDefaultValue, flags, pHelpString );
}

ConVar_L4D::ConVar_L4D( const char *pName, const char *pDefaultValue, int flags, const char *pHelpString, bool bMin, float fMin, bool bMax, float fMax )
{
	Create( pName, pDefaultValue, flags, pHelpString, bMin, fMin, bMax, fMax );
}

ConVar_L4D::ConVar_L4D( const char *pName, const char *pDefaultValue, int flags, const char *pHelpString, FnChangeCallback_t callback )
{
	Create( pName, pDefaultValue, flags, pHelpString, false, 0.0, false, 0.0, callback );
}

ConVar_L4D::ConVar_L4D( const char *pName, const char *pDefaultValue, int flags, const char *pHelpString, bool bMin, float fMin, bool bMax, float fMax, FnChangeCallback_t callback )
{
	Create( pName, pDefaultValue, flags, pHelpString, bMin, fMin, bMax, fMax, callback );
}


//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
ConVar_L4D::~ConVar_L4D( void )
{
	if ( m_pszString )
	{
		delete[] m_pszString;
		m_pszString = NULL;
	}
}


//-----------------------------------------------------------------------------
// Install a change callback (there shouldn't already be one....)
//-----------------------------------------------------------------------------
void ConVar_L4D::InstallChangeCallback( FnChangeCallback_t callback )
{
	Assert( !m_pParent->m_fnChangeCallback || !callback );
	m_pParent->m_fnChangeCallback = callback;

	if ( m_pParent->m_fnChangeCallback )
	{
		// Call it immediately to set the initial value...
		m_pParent->m_fnChangeCallback( (ConVar*)this, m_pszString, m_fValue );
	}
}

bool ConVar_L4D::IsFlagSet( int flag ) const
{
	return ( flag & m_pParent->m_nFlags ) ? true : false;
}

const char *ConVar_L4D::GetHelpText( void ) const
{
	return m_pParent->m_pszHelpString;
}

void ConVar_L4D::AddFlags( int flags )
{
	m_pParent->m_nFlags |= flags;

#ifdef ALLOW_DEVELOPMENT_CVARS
	m_pParent->m_nFlags &= ~FCVAR_DEVELOPMENTONLY;
#endif
}

int ConVar_L4D::GetFlags( void ) const
{
	return m_pParent->m_nFlags;
}

bool ConVar_L4D::IsRegistered( void ) const
{
	return m_pParent->m_bRegistered;
}

const char *ConVar_L4D::GetName( void ) const
{
	return m_pParent->m_pszName;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ConVar_L4D::IsCommand( void ) const
{ 
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
//-----------------------------------------------------------------------------
void ConVar_L4D::Init()
{
	BaseClass::Init();
}

const char *ConVar_L4D::GetBaseName( void ) const
{
	return m_pParent->m_pszName;
}

int ConVar_L4D::GetSplitScreenPlayerSlot( void ) const
{
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
//-----------------------------------------------------------------------------
void ConVar_L4D::InternalSetValue( const char *value )
{
	float fNewValue;
	char  tempVal[ 32 ];
	char  *val;

	Assert(m_pParent == this); // Only valid for root convars.

	float flOldValue = m_fValue;

	val = (char *)value;
	fNewValue = ( float )atof( value );

	if ( ClampValue( fNewValue ) )
	{
		Q_snprintf( tempVal,sizeof(tempVal), "%f", fNewValue );
		val = tempVal;
	}
	
	// Redetermine value
	m_fValue		= fNewValue;
	m_nValue		= ( int )( m_fValue );

	if ( !( m_nFlags & FCVAR_NEVER_AS_STRING ) )
	{
		ChangeStringValue( val, flOldValue );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tempVal - 
//-----------------------------------------------------------------------------
void ConVar_L4D::ChangeStringValue( const char *tempVal, float flOldValue )
{
	Assert( !( m_nFlags & FCVAR_NEVER_AS_STRING ) );

 	char* pszOldValue = (char*)stackalloc( m_StringLength );
	memcpy( pszOldValue, m_pszString, m_StringLength );
	
	int len = Q_strlen(tempVal) + 1;

	if ( len > m_StringLength)
	{
		if (m_pszString)
		{
			delete[] m_pszString;
		}

		m_pszString	= new char[len];
		m_StringLength = len;
	}

	memcpy( m_pszString, tempVal, len );

	// Invoke any necessary callback function
	if ( m_fnChangeCallback )
	{
		m_fnChangeCallback( (ConVar*)this, pszOldValue, flOldValue );
	}

	g_pCVar->CallGlobalChangeCallbacks( (ConVar*)this, pszOldValue, flOldValue );

	stackfree( pszOldValue );
}

//-----------------------------------------------------------------------------
// Purpose: Check whether to clamp and then perform clamp
// Input  : value - 
// Output : Returns true if value changed
//-----------------------------------------------------------------------------
bool ConVar_L4D::ClampValue( float& value )
{
	if ( m_bHasMin && ( value < m_fMinVal ) )
	{
		value = m_fMinVal;
		return true;
	}
	
	if ( m_bHasMax && ( value > m_fMaxVal ) )
	{
		value = m_fMaxVal;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
//-----------------------------------------------------------------------------
void ConVar_L4D::InternalSetFloatValue( float fNewValue )
{
	if ( fNewValue == m_fValue )
		return;

	Assert( m_pParent == this ); // Only valid for root convars.

	// Check bounds
	ClampValue( fNewValue );

	// Redetermine value
	float flOldValue = m_fValue;
	m_fValue		= fNewValue;
	m_nValue		= ( int )m_fValue;

	if ( !( m_nFlags & FCVAR_NEVER_AS_STRING ) )
	{
		char tempVal[ 32 ];
		Q_snprintf( tempVal, sizeof( tempVal), "%f", m_fValue );
		ChangeStringValue( tempVal, flOldValue );
	}
	else
	{
		Assert( !m_fnChangeCallback );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
//-----------------------------------------------------------------------------
void ConVar_L4D::InternalSetIntValue( int nValue )
{
	if ( nValue == m_nValue )
		return;

	Assert( m_pParent == this ); // Only valid for root convars.

	float fValue = (float)nValue;
	if ( ClampValue( fValue ) )
	{
		nValue = ( int )( fValue );
	}

	// Redetermine value
	float flOldValue = m_fValue;
	m_fValue		= fValue;
	m_nValue		= nValue;

	if ( !( m_nFlags & FCVAR_NEVER_AS_STRING ) )
	{
		char tempVal[ 32 ];
		Q_snprintf( tempVal, sizeof( tempVal ), "%d", m_nValue );
		ChangeStringValue( tempVal, flOldValue );
	}
	else
	{
		Assert( !m_fnChangeCallback );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Private creation
//-----------------------------------------------------------------------------
void ConVar_L4D::Create( const char *pName, const char *pDefaultValue, int flags /*= 0*/,
	const char *pHelpString /*= NULL*/, bool bMin /*= false*/, float fMin /*= 0.0*/,
	bool bMax /*= false*/, float fMax /*= false*/, FnChangeCallback_t callback /*= NULL*/ )
{
	static const char *empty_string = "";

	m_pParent = this;

	// Name should be static data
	m_pszDefaultValue = pDefaultValue ? pDefaultValue : empty_string;
	Assert( m_pszDefaultValue );

	m_StringLength = strlen( m_pszDefaultValue ) + 1;
	m_pszString = new char[m_StringLength];
	memcpy( m_pszString, m_pszDefaultValue, m_StringLength );
	
	m_bHasMin = bMin;
	m_fMinVal = fMin;
	m_bHasMax = bMax;
	m_fMaxVal = fMax;
	
	m_fnChangeCallback = callback;

	m_fValue = ( float )atof( m_pszString );

	// Bounds Check, should never happen, if it does, no big deal
	if ( m_bHasMin && ( m_fValue < m_fMinVal ) )
	{
		Assert( 0 );
	}

	if ( m_bHasMax && ( m_fValue > m_fMaxVal ) )
	{
		Assert( 0 );
	}

	m_nValue = ( int )m_fValue;

	BaseClass::Create( pName, pHelpString, flags );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
//-----------------------------------------------------------------------------
void ConVar_L4D::SetValue(const char *value)
{
	ConVar_L4D *var = ( ConVar_L4D * )m_pParent;
	var->InternalSetValue( value );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : value - 
//-----------------------------------------------------------------------------
void ConVar_L4D::SetValue( float value )
{
	ConVar_L4D *var = ( ConVar_L4D * )m_pParent;
	var->InternalSetFloatValue( value );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : value - 
//-----------------------------------------------------------------------------
void ConVar_L4D::SetValue( int value )
{
	ConVar_L4D *var = ( ConVar_L4D * )m_pParent;
	var->InternalSetIntValue( value );
}

//-----------------------------------------------------------------------------
// Purpose: Reset to default value
//-----------------------------------------------------------------------------
void ConVar_L4D::Revert( void )
{
	// Force default value again
	ConVar_L4D *var = ( ConVar_L4D * )m_pParent;
	var->SetValue( var->m_pszDefaultValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : minVal - 
// Output : true if there is a min set
//-----------------------------------------------------------------------------
bool ConVar_L4D::GetMin( float& minVal ) const
{
	minVal = m_pParent->m_fMinVal;
	return m_pParent->m_bHasMin;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : maxVal - 
//-----------------------------------------------------------------------------
bool ConVar_L4D::GetMax( float& maxVal ) const
{
	maxVal = m_pParent->m_fMaxVal;
	return m_pParent->m_bHasMax;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *ConVar_L4D::GetDefault( void ) const
{
	return m_pParent->m_pszDefaultValue;
}