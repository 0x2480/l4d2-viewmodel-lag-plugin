//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Basic BOT handling.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#include "interface.h"
#include "filesystem.h"
#undef VECTOR_NO_SLOW_OPERATIONS
#include "mathlib/vector.h"

#include "eiface.h"
#include "edict.h"
#include "game/server/iplayerinfo.h"
#include "igameevents.h"
#include "convar.h"
#include "convar_l4d.h"
#include "vstdlib/random.h"
#include "../../game/shared/in_buttons.h"
#include "../../game/shared/shareddefs.h"
#include "engine/IEngineTrace.h"
#include "utils.h"
#include "sigscan.h"
#include "icliententity.h"
#include "minhook/minhook.h"
#define USE_LASTTIMESTAMP
#include "interpolatedvar.h"
#include "cdll_int.h"

extern IBotManager *botmanager; 
extern IUniformRandomStream *randomStr;
extern IVEngineServer	*engine; 
extern IEngineTrace *enginetrace;
extern IPlayerInfoManager *playerinfomanager; 
extern IServerPluginHelpers *helpers;

extern CGlobalVars* gpGlobals;

enum ESourceEngine {
	k_eL4D1,
	k_eL4D2,
	k_eOther
};

CGlobalVars *gpClientGlobals;
IVEngineClient* engineclient;
ICvar* pcvar;

// -------------------------------------------------------------------------
// Variables so that CInterpolatedVar can work
bool CInterpolationContext::s_bAllowExtrapolation;
float CInterpolationContext::s_flLastTimeStamp;
ConVar cl_extrapolate_amount = ConVar("", "", FCVAR_HIDDEN);
// -------------------------------------------------------------------------

typedef void(__fastcall* CalcViewModelView_t)(void* thisptr, void*, void* owner, const Vector& eyePosition, const QAngle& eyeAngles);
CalcViewModelView_t C_TerrorViewModel_CalcViewModelView_Original;

ConVar* plugin_wpn_sway_cvar;
ConVar* plugin_wpn_sway_scale;
ConVar* plugin_wpn_sway_interp;

CInterpolatedVar<QAngle> m_LagAnglesHistory;
QAngle m_vLagAngles;
void CalcViewModelLag(Vector& origin, QAngle& angles, QAngle& original_angles)
{
	// Calculate our drift
	Vector	forward, right, up;
	AngleVectors(angles, &forward, &right, &up);

	// Add an entry to the history.
	m_vLagAngles = angles;
	m_LagAnglesHistory.NoteChanged(gpClientGlobals->curtime, plugin_wpn_sway_interp->GetFloat(), false);

	// Interpolate back 100ms.
	m_LagAnglesHistory.Interpolate(gpClientGlobals->curtime, plugin_wpn_sway_interp->GetFloat());

	// Now take the 100ms angle difference and figure out how far the forward vector moved in local space.
	Vector vLaggedForward;
	QAngle angleDiff = m_vLagAngles - angles;
	AngleVectors(-angleDiff, &vLaggedForward, 0, 0);
	Vector vForwardDiff = Vector(1, 0, 0) - vLaggedForward;

	// Now offset the origin using that.
	vForwardDiff *= plugin_wpn_sway_scale->GetFloat();
	origin += forward * vForwardDiff.x + right * -vForwardDiff.y + up * vForwardDiff.z;
}

void __fastcall C_TerrorViewModel_CalcViewModelView(void* thisptr, void* edx, void* owner, const Vector& eyePosition, const QAngle& eyeAngles)
{
	if (!plugin_wpn_sway_cvar->GetBool()) {
		C_TerrorViewModel_CalcViewModelView_Original(thisptr, edx, owner, eyePosition, eyeAngles);
		return;
	}
	
	QAngle vmangoriginal = eyeAngles;
	QAngle vmangles = eyeAngles;
	Vector vmorigin = eyePosition;

	CalcViewModelLag(vmorigin, vmangles, vmangoriginal);
	C_TerrorViewModel_CalcViewModelView_Original(thisptr, edx, owner, vmorigin, vmangles);
}

ESourceEngine eEngine = k_eOther;
CSigScan C_TerrorViewModel_CalcViewModelView_Sig, gpGlobals_Sig;
bool Viewmodel_Run(CreateInterfaceFn interfaceFactory)
{
	if (!CSigScan::SetDllMemInfo("client.dll")) {
		Warning("Failed to set client.dll memory info for signature scan!\n");
		return false;
	}

	static char modDir[MAX_PATH];
	if (Q_strlen(modDir) == 0)
	{
		const char* gamedir = CommandLine()->ParmValue("-game", CommandLine()->ParmValue("-defaultgamedir", "hl2"));
		Q_strncpy(modDir, gamedir, sizeof(modDir));
		if (strchr(modDir, '/') || strchr(modDir, '\\'))
		{
			Q_StripLastDir(modDir, sizeof(modDir));
			int dirlen = Q_strlen(modDir);
			Q_strncpy(modDir, gamedir + dirlen, sizeof(modDir) - dirlen);
		}
	}

	if (V_strcmp(modDir, "left4dead2") == 0) {
		eEngine = k_eL4D2;
		Msg("This is the L4D2 engine!\n");
	} else if (V_strcmp(modDir, "left4dead") == 0) {
		eEngine = k_eL4D1;
		Msg("This is the L4D1 engine!\n");
	} else {
		Warning("Plugin running on an engine different from Left 4 Dead series.\n");
		return false;
	}

	engineclient = (IVEngineClient*)interfaceFactory(VENGINE_CLIENT_INTERFACE_VERSION, NULL);
	pcvar = (ICvar*)interfaceFactory(CVAR_INTERFACE_VERSION, NULL);

	if (eEngine == k_eL4D2) {
		plugin_wpn_sway_cvar = new ConVar("pl_wpn_sway_enabled", "1", FCVAR_CLIENTDLL, "Restores HL2 sway.");
		plugin_wpn_sway_scale = new ConVar("pl_wpn_sway_scale", "1.5", FCVAR_CLIENTDLL);
		plugin_wpn_sway_interp = new ConVar("pl_wpn_sway_interp", "0.1", FCVAR_CLIENTDLL);

		C_TerrorViewModel_CalcViewModelView_Sig.Init((unsigned char*)
			"\x55\x8B\xEC\x83\xEC\x48\xA1\x00\x00\x00\x00\x33\xC5\x89\x45\xFC\x8B\x45\x10\x8B", "xxxxxxx????xxxxxxxxx", 20);
		gpGlobals_Sig.Init((unsigned char*)
			"\xA3\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8D\x55", "x????x????xx", 12);
	}
	else {
		plugin_wpn_sway_cvar = (ConVar*)new ConVar_L4D("pl_wpn_sway_enabled", "1", FCVAR_CLIENTDLL, "Restores HL2 sway.");
		plugin_wpn_sway_scale = (ConVar*)new ConVar_L4D("pl_wpn_sway_scale", "1.5", FCVAR_CLIENTDLL);
		plugin_wpn_sway_interp = (ConVar*)new ConVar_L4D("pl_wpn_sway_interp", "0.1", FCVAR_CLIENTDLL);

		C_TerrorViewModel_CalcViewModelView_Sig.Init((unsigned char*)
			"\x83\xEC\x44\x8B\x44\x24\x50", "xxxxxxx", 7);
		gpGlobals_Sig.Init((unsigned char*)
			"\xA3\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8D\x54", "x????x????xx", 12);
	}

    if (!C_TerrorViewModel_CalcViewModelView_Sig.is_set || !gpGlobals_Sig.is_set) {
		Warning("Signature scan failed!\n");
        return false;
    }
	m_LagAnglesHistory.Setup(&m_vLagAngles, 0);

	gpClientGlobals = **(CGlobalVars***)((uintptr_t)gpGlobals_Sig.sig_addr + 1);

    MH_Initialize();
	MH_CreateHook(C_TerrorViewModel_CalcViewModelView_Sig.sig_addr, &C_TerrorViewModel_CalcViewModelView, (LPVOID*)&C_TerrorViewModel_CalcViewModelView_Original);
	MH_EnableHook(MH_ALL_HOOKS);

	return true;
}

void Viewmodel_Stop()
{
	MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}
