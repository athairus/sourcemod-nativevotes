/**
 * vim: set ts=4 :
 * =============================================================================
 * NativeVotes
 * Copyright (C) 2011-2013 Ross Bemrose (Powerlord).  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */
 
//#include <menus>
#include <sourcemod>

// NativeVotes 0.9 series

#if defined _nativevotes_included
 #endinput
#endif
#define _nativevotes_included

#define CLIENT_DISCONNECTED			-1

#define NATIVEVOTES_EXTEND 			"Extend current Map" /** Defined in TF2, but doesn't appear to be localized */

#define NATIVEVOTES_ALL_TEAMS		-1 // Defined by TF2, may be the same in L4D/L4D2
#define NATIVEVOTES_TF2_ALL_TEAMS	0 // Defined by TF2, may be the same in L4D/L4D2, only used internally
#define NATIVEVOTES_TEAM_UNASSIGNED 0 // For completeness, do not otherwise use
#define NATIVEVOTES_TEAM_SPECTATOR	1 // Spectators
#define NATIVEVOTES_TEAM_1			2 // RED/Survivors/Terrorists
#define NATIVEVOTES_TEAM_2			3 // BLU/Infected/Counter-Terrorists

#define NATIVEVOTES_SERVER_INDEX 	99 // Defined by TF2, may be the same in L4D/L4D2

// These may seem backwards, but this is the order that the votes appear in the vote screen
#define NATIVEVOTES_VOTE_INVALID	-1		/**< Vote was invalid, currently only valid internally */
#define NATIVEVOTES_VOTE_YES		0		/**< Vote was yes */
#define NATIVEVOTES_VOTE_NO			1		/**< Vote was no */

/*
The following MenuActions are supported.  Arguments are also listed, as they differ slightly from the default
MenuAction_Start		A menu has been started (nothing passed). Only exists for compat reasons.
MenuAction_Display	A menu is about to be displayed (param1=client). If you choose to change the vote text,
 					To change the text, use RedrawVoteTitle()
					If you do so, return 1 or _:Plugin_Changed  Otherwise, return _:Plugin_Continue or 0.
MenuAction_Select		An item was selected (param1=client, param2=item). For subplugin support.
MenuAction_End		A vote has fully ended and the vote object is ready to be cleaned up
					param1 is MenuEnd reason, either MenuEnd_VotingCancelled or MenuEnd_VotingDone
MenuAction_VoteEnd	A vote sequence has succeeded (param1=chosen item) 
					This is not called if NativeVotes_SetResultCallback has been used on the vote.
					You should call NativeVotes_DisplayPass or NativeVotes_DisplayPassEx after this
MenuAction_VoteStart	A vote sequence has started (nothing passed).  Use this instead of MenuAction_Start
MenuAction_VoteCancel	A vote sequence has been cancelled (param1=reason)
MenuAction_DisplayItem	Item text is being drawn to the display (param1=client, param2=item)
					To change the text, use RedrawVoteItem().
					If you do so, return 1 or _:Plugin_Changed.  Otherwise, return _:Plugin_Continue or 0.
*/

#define NATIVEVOTES_ACTIONS_DEFAULT		MenuAction_VoteStart|MenuAction_VoteCancel|MenuAction_VoteEnd|MenuAction_End

/**
 * Vote types. These are mapped to translation strings and pass strings by VoteStart and VotePass handlers
 */
enum NativeVotesType:
{
	NativeVotesType_None = 0,		/** Special placeholder for callvote with no arguments for NativeVotes_OnCallVote */
	NativeVotesType_Custom_YesNo,	/**< Yes/No, details are vote text. */
	NativeVotesType_Custom_Mult,	/**< TF2/CS:GO: Multiple-choice, details are vote text. */
	NativeVotesType_ChgCampaign,	/**< L4D/L4D2: Yes/No, details are campaign name */
	NativeVotesType_ChgDifficulty,	/**< L4D/L4D2: Yes/No, details are difficulty number in L4D/L4D2 */
	NativeVotesType_ReturnToLobby,	/**< L4D/L4D2: Yes/No, details are ignored */
	NativeVotesType_AlltalkOn,		/**< L4D2: Yes/No, details are ignored (handled internally by extension) */
	NativeVotesType_AlltalkOff,		/**< L4D2: Yes/No, details are ignored (handled internally by extension) */
	NativeVotesType_Restart,		/**< Yes/No, details are ignored */
	NativeVotesType_Kick,			/**< Yes/No, target is player userid, details are auto-set by target */
	NativeVotesType_KickIdle,		/**< TF2/CS:GO: Yes/No, target is player userid, details are auto-set by target */
	NativeVotesType_KickScamming,	/**< TF2/CS:GO: Yes/No, target is player userid, details are auto-set by target */
	NativeVotesType_KickCheating,	/**< TF2/CS:GO: Yes/No, target is player userid, details are auto-set by target */
	NativeVotesType_ChgLevel,		/**< Yes/No, details are level number in L4D/L4D2 or map name in TF2 */
	NativeVotesType_NextLevel,		/**< TF2/CS:GO: Yes/No, details are map name */
	NativeVotesType_NextLevelMult,	/**< TF2/CS:GO: Multiple-choice, details are ignored */
	NativeVotesType_ScrambleNow,	/**< TF2/CS:GO: Yes/No, details are ignored */
	NativeVotesType_ScrambleEnd,	/**< TF2: Yes/No, details are ignored */
	NativeVotesType_ChgMission,		/**< TF2: Yes/No, details are popfile */
	NativeVotesType_SwapTeams,		/**< CS:GO: Yes/No, details are ignored */
	NativeVotesType_Surrender,		/**< CS:GO: Yes/No, details are ignored */
	NativeVotesType_Rematch,		/**< CS:GO: Yes/No, details are ignored */
	NativeVotesType_Continue,		/**< CS:GO: Yes/No, details are ignored */
	NativeVotesType_StartRound,		/**< TF2: Yes/No, details are ignored */
	NativeVotesType_Eternaween,		/**< TF2: Yes/No, details are ignored */
	NativeVotesType_AutoBalanceOn,	/**< TF2: Yes/No, details are ignored */
	NativeVotesType_AutoBalanceOff,	/**< TF2: Yes/No, details are ignored */
	NativeVotesType_ClassLimitsOn,	/**< TF2: Yes/No, details are ignored */
	NativeVotesType_ClassLimitsOff,	/**< TF2: Yes/No, details are ignored */
};

enum NativeVotesPassType:
{
	NativeVotesPass_None = 0,			/**< Special placeholder for error value */
	NativeVotesPass_Custom,				/**< Details are custom pass message */
	NativeVotesPass_ChgCampaign,		/**< L4D/L4D2: Details are campaign name */
	NativeVotesPass_ChgDifficulty,		/**< L4D/L4D2/TF2: Details are difficulty number in L4D/L4D2 and mission name in TF2 */
	NativeVotesPass_ReturnToLobby,      /**< L4D/L4D2: Details are ignored */
	NativeVotesPass_AlltalkOn,			/**< L4D2: Details are ignored */
	NativeVotesPass_AlltalkOff,			/**< L4D2: Details are ignored */
	NativeVotesPass_Restart,			/**< Details are ignored */
	NativeVotesPass_Kick,				/**< Details are player name */
	NativeVotesPass_ChgLevel,			/**< Details are level number in L4D/L4D2 or map name in TF2/CS:GO */
	NativeVotesPass_NextLevel,			/**< TF2/CS:GO: Details are map name */
	NativeVotesPass_Extend,				/**< TF2/CS:GO: Details are ignored */
	NativeVotesPass_Scramble,			/**< TF2/CS:GO: Details are ignored */
	NativeVotesPass_ChgMission,			/**< TF2: Details are popfile */
	NativeVotesPass_SwapTeams,			/**< CS:GO: Details are ignored */
	NativeVotesPass_Surrender,			/**< CS:GO: Details are ignored */
	NativeVotesPass_Rematch,			/**< CS:GO: Details are ignored */
	NativeVotesPass_Continue,			/**< CS:GO: Details are ignored */
	NativeVotesPass_StartRound,			/**< TF2: Details are ignored */
	NativeVotesPass_Eternaween,			/**< TF2: Details are ignored */
	NativeVotesPass_AutoBalanceOn,		/**< TF2: Yes/No, details are ignored */
	NativeVotesPass_AutoBalanceOff,		/**< TF2: Yes/No, details are ignored */
	NativeVotesPass_ClassLimitsOn,		/**< TF2: Yes/No, details are ignored */
	NativeVotesPass_ClassLimitsOff,		/**< TF2: Yes/No, details are ignored */
};

/**
 * Reasons a vote was canceled.  Not used for L4D/L4D2, as they don't care
 */
enum NativeVotesFailType:
{
	NativeVotesFail_Generic = 0,		/**< Vote was generically cancelled. */
	NativeVotesFail_Loses = 3,			/**< No votes outnumbered Yes votes */
	NativeVotesFail_NotEnoughVotes = 4,	/**< Vote did not receive enough votes. */
};

/**
 * Reasons a callvote command failed.
 * This is provided as a convenience to plugin authors as it's not strictly part of the vote system
 */
enum NativeVotesCallFailType:
{
	NativeVotesCallFail_Generic = 0,				/**< Generic fail. */
	NativeVotesCallFail_Loading = 1,				/**< L4D/L4D2: Players are still loading. */
	NativeVotesCallFail_Recent = 2,				/**< TF2/CS:GO: You can't call another vote yet: Argument is seconds until you can call another vote. */
	NativeVotesCallFail_Disabled = 5,			/**< TF2/CS:GO: Server has disabled that issue. */
	NativeVotesCallFail_MapNotFound = 6,			/**< TF2/CS:GO: Server does not have that map. */
	NativeVotesCallFail_SpecifyMap = 7,			/**< TF2/CS:GO: You must specify a map. */
	NativeVotesCallFail_Failed = 8,				/**< TF2/CS:GO: This vote failed recently. */
	NativeVotesCallFail_WrongTeam = 9,			/**< TF2/CS:GO: Team can't call this vote. */
	NativeVotesCallFail_Waiting = 10,			/**< TF2/CS:GO: Vote can't be called during Waiting For Players. */
	NativeVotesCallFail_PlayerNotFound = 11,		/**< TF2/CS:GO: Player to kick can't be found. Buggy in TF2. */
	NativeVotesCallFail_Unknown = 11,
	NativeVotesCallFail_CantKickAdmin = 12,		/**< TF2/CS:GO: Can't kick server admin. */
	NativeVotesCallFail_ScramblePending = 13,	/**< TF2/CS:GO: Team Scramble is pending.. */
	NativeVotesCallFail_Spectators = 14,			/**< TF2/CS:GO: Spectators aren't allowed to call votes. */
	NativeVotesCallFail_LevelSet = 15,			/**< TF2/CS:GO: Next level already set. */
	NativeVotesCallFail_MapNotValid = 16,		/**< TF2: Map is invalid. */
	NativeVotesCallFail_KickTime = 17,			/**< TF2: Cannot kick for time. */
	NativeVotesCallFail_KickDuringRound = 18,	/**< TF2: Cannot kick during a round. */
	NativeVotesCallFail_AlreadyActive = 19		/**< TF2: Cannot call vote because modification (Eternaween) is already active (may not work) */
};

/**
 * Called when a menu action is completed.
 * 
 * @param vote		The vote being acted upon.
 * @param action		The action of the vote.
 * @param param1		First action parameter (usually the client).
 * @param param2		Second action parameter (usually the item).
 */
typedef NativeVotesHandler = function int (NativeVote vote, MenuAction action, int param1, int param2);

// Technically this is a KeyValues, but the implementation type may change in the future
methodmap NativeVote < KeyValues
{
	// Creates a new, empty vote of the specified type
	//
	// @param handler		Function which will receive menu actions.
	// @param voteType	Type of the vote. Some votes also require Details to be set.
	// @param actions		Optionally set which actions to receive. Select,
	//					VoteCancel, and VoteEnd will always be received
	//					regardless of whether they are set or not.  They
	//					are also the only default actions.
	public native NativeVote(NativeVotesHandler handler, NativeVotesType voteType, MenuAction actions=NATIVEVOTES_ACTIONS_DEFAULT);
	
	// Appends a new item to the end of a vote.  Only valid for Multiple Choice votes.
	//
	// @param info		Item information string.
	// @param display		Default item display string.
	// @return			True on success, false on failure.
	// @error			Item limit reached.
	public native bool AddItem(const char[] info, const char[] display);
	
	// Inserts an item into the vote before a certain position; the new item will
	// be at the given position and all next items pushed forward.
	//
	// Only valid for Multiple Choice votes.
	//
	// @param position	Position, starting from 0.
	// @param info		Item information string.
	// @param display		Default item display string.
	// @return			True on success, false on failure.
	// @error			Invalid menu position
	public native bool InsertItem(int position, const char[] info, const char[] display);
	
	// Removes an item from tne vote.
	//
	// @param position	Position, starting from 0.
	// @return			True on success, false on failure.
	// @error			Invalid menu position.
	public native bool RemoveItem(int position);
	
	// Removes all items from the vote.
	public native void RemoveAllItems();
	
	// Retrieves information about a vote item.
	//
	// @param position      Position, starting from 0.
	// @param infoBuf       Info buffer.
	// @param infoBufLen    Maximum length of the info buffer.
	// @param dispBuf       Display buffer.
	// @param dispBufLen    Maximum length of the display buffer.
	// @return              True on success, false if position is invalid.	
	public native bool GetItem(int position, char[] infoBuf, char[] infoBufLen,
	                            char[] displayBuf="", int displayBufLen=0);
	
	// Sets a vote's details.
	// Vote types have documented what details they need.
	//
	// If this is a custom vote, use SetTitle instead.
	// 
	// @param fmt		Message string format
	// @param ...		Message string arguments.
	public native void SetDetails(const char[] fmt, any ...);
	
	// Returns the text of a vote's details.
	//
	// Note: A custom vote's title is copied here for backward compatibility
	// @param buffer		Buffer to store details.
	// @param maxlength	Maximum length of the buffer
	public native void GetDetails(char[] buffer, int maxlength);
	
	// Sets the target userid for vote
	// This should be used instead of SetArgument for votes that target players
	// 
	// Also sets target SteamID2
	// 
	// @param userid		Client index of target player
	// @param setDetails	If true, also sets vote details to client's name
	public native void SetTarget(int client, bool setDetails=true);

	// Returns the userid of a vote's target
	//
	// @return		Client index of target player or 0 for no target or target disconnected since vote started
	public native int GetTarget();
	
	// Get the SteamID2 of a vote's target
	// Useful if the target has disconnected from the server during a vote.
	// This was added in specifically for Kick/Ban votes
	//
	// @param buffer		Buffer to store steamId.  Should be 19 characters or more..
	// @param maxlength	Maximum length of the buffer.
	public native void GetTargetSteam(char[] buffer, int maxlength);
	
	// Sets a custom vote's default title/instruction message.
	//
	// @param fmt		Message string format
	// @param ...		Message string arguments.
	public native void SetTitle(const char[] fmt, any ...);
	
	// Returns the text of a vote's title.
	//
	// @param buffer		Buffer to store title.
	// @param maxlength	Maximum length of the buffer.
	public native void GetTitle(char[] buffer, int maxlength);
	
	// Broadcasts a vote to a list of clients.  The most selected item will be
	// returned through MenuAction_VoteEnd.  On a tie, a random item will be returned
	// from a list of the tied items.
	// 
	// Note that MenuAction_VoteEnd and MenuAction_VoteStart are both
	// default callbacks and do not need to be enabled.
	// 
	// @param clients		Array of clients to broadcast to.
	// @param numClients	Number of clients in the array.
	// @param time		Maximum time to leave vote on the screen.
	// @param flags		Optional voting flags.
	// @return			True on success, false if this vote already has a
	// 					vote session in progress.
	// @error			A vote is already in progress.
	public native bool DisplayVote(int[] clients, int numClients, int time, int flags=0);

	// Sends a vote to all clients.  See DisplayVote() for more information.
	//
	// @param time		Maximum time to leave vote on the screen.
	// @param flags		Optional voting flags.
	// @return			True on succhess, false if this vote already has a
	//					vote session in progress.
	public bool DisplayVoteToAll(int time, int flags=0) {
		int total = 0;
		int[] players = new int[MaxClients];
		for (int i=1; i<=MaxClients; i++)
		{
			if (!IsClientInGame(i) || IsFakeClient(i))
				continue;
			players[total++] = i;
		}
		return this.DisplayVote(players, total, time, flags);
	}
	
	// Display vote passed screen
	//
	// You MUST call one of the DisplayPass* or DisplayFail functions
	// to hide the vote screen for users who didn't vote, and to clear out their selection
	// for the next vote.
	// 
	// @param details			Normally the item that won the vote or format string. Also used for custom vote winners.
	// @param ...				Variable number of format parameters.
	public native void DisplayPass(const char[] details="", any ...);
	
	// Display vote passed screen with custom text to a single user
	//
	// You MUST call one of the DisplayPass* or DisplayFail functions
	// to hide the vote screen for users who didn't vote, and to clear out their selection
	// for the next vote.
	// 
	// @param client			client to display to
	// @param format			A format string.
	// @param any				Variable number of format parameters
	public native void DisplayPassCustomToOne(int client, const char[] format, any ...);
	
	// Display vote passed screen with custom text
	//
	// You MUST call one of the DisplayPass* or DisplayFail functions
	// to hide the vote screen for users who didn't vote, and to clear out their selection
	// for the next vote.
	// 
	// @param format			A format string.
	// @param any				Variable number of format parameters
	public void DisplayPassCustom(const char[] format, any ...)
	{
		char buffer[192];
		for (int i = 1; i <= MaxClients; ++i)
		{
			if (IsClientInGame(i))
			{
				SetGlobalTransTarget(i);
				VFormat(buffer, sizeof(buffer), format, 3);
				this.DisplayPassCustomToOne(i, "%s", buffer);
			}
		}
	}
	
	// Display vote passed screen with a custom type.
	//
	// A sample usage of this would be if Extend won an RTV vote: vote.DisplayPassEx(NativeVotesPass_Extend);
	//
	// You MUST call one of the DisplayPass* or DisplayFail functions
	// to hide the vote screen for users who didn't vote, and to clear out their selection
	// for the next vote.
	// 
	// @param passType		The pass screen to display
	// @param details			Normally the item that won the vote. Also used for custom vote winners
	public native void DisplayPassEx(NativeVotesPassType passType, const char[] details="", any ...);
	
	// Display failure screen.
	//
	// You MUST call one of the DisplayPass* or DisplayFail functions
	// to hide the vote screen for users who didn't vote, and to clear out their selection
	// for the next vote.
	// 
	// @param reason			Vote failure reason from NativeVotesFailType enum
	public native void DisplayFail(NativeVotesFailType reason=NativeVotesFail_Generic);
	
	// Frees all handles related to a vote.
	// 
	// THIS MUST BE CALLED TO AVOID HANDLE LEAKS
	public native void Close();
	
	// Get or set the vote's option flags.
	//
	// If a certain bit is not supported, it will be stripped before being set.
	property int OptionFlags {
		public native get();
		public native set(int value);
	}
		
	// Sets whether or not the vote has a "no vote" button in slot 1.
	// By default, votes do not have a no vote button.
	property bool NoVoteButton {
		public native set(bool value);
	}
	
	// Sets an advanced vote handling callback. If this callback is set,
	// MenuAction_VoteEnd will not be called.
	property NativeVotes_VoteHandler VoteResultCallback {
		public native set(NativeVotes_VoteHandler handler);
	}
	
	// Returns the number of items in a menu.
	property int ItemCount {
		public native get();
	}
	
	// Get or set the team this vote is for.
	// Use NATIVEVOTES_ALL_TEAMS for all teams.
	// 
	// Defaults to NATIVEVOTES_ALL_TEAMS if not explicitly set.
	property int Team {
		public native get();
		public native set(int value);
	}
	
	// Get or set the client index of the player who initiated the vote or NATIVEVOTES_SERVER_INDEX if 
	// initiated by the server itself.
	// 
	// @return					Client index or NATIVEVOTES_SERVER_INDEX
	property int Initiator {
		public native get();
		public native set(int value);
	}
}

/*
 * Is a specific vote type supported by this game?
 * 
 * @param voteType			Vote type
 */
native bool NativeVotes_IsVoteTypeSupported(NativeVotesType voteType);

/**
 * Creates a new, empty vote.
 * 
 * @param handler			Function which will receive vote actions.
 * @param voteType		Vote type, cannot be changed after set
 * @param actions			Optionally set which actions to receive.  Start, 
 *                          Cancel, and End will always be received regardless
 *                          of whether they are set or not.  They are also
 *                          the only default actions.
 * @return				A new vote Handle or null if a vote type is unsupported by this game.
 */
native NativeVote NativeVotes_Create(NativeVotesHandler handler, NativeVotesType voteType,
							MenuAction actions=NATIVEVOTES_ACTIONS_DEFAULT);

/**
 * Frees all handles related to a vote.
 * 
 * THIS MUST BE CALLED TO AVOID HANDLE LEAKS
 * 
 * @param vote				Vote handle
 * @noreturn
 */
native void NativeVotes_Close(NativeVote vote);

/**
 * Appends a new item to the end of a vote.  Only valid for Multiple Choice votes.
 *
 * @param vote				NativeVotes Handle.
 * @param info				Item information string.
 * @return					True on success, false on failure.
 * @error					Invalid Handle, item limit reached, or if the vote is not multiple choice.
 */
native bool NativeVotes_AddItem(NativeVote vote, const char[] info, const char[] display);

/**
 * Inserts an item into the menu before a certain position; the new item will
 * be at the given position and all next items pushed forward.
 *
 * Only valid for Multiple Choice votes.
 *
 * @param vote				Vote Handle.
 * @param position			Position, starting from 0.
 * @param info				Item information string.
 * @return					True on success, false on failure.
 * @error					Invalid Handle or vote position, or if the vote is not multiple choice.
 */
native bool NativeVotes_InsertItem(NativeVote vote, int position, const char[] info, const char[] display);

/**
 * Removes an item from the menu.
 *
 * Only valid for Multiple Choice votes.
 *
 * @param vote				Vote Handle.
 * @param position			Position, starting from 0.
 * @return					True on success, false on failure.
 * @error					Invalid Handle or vote position,  or if the vote is not multiple choice.
 */
native bool NativeVotes_RemoveItem(NativeVote vote, int position);

/**
 * Removes all items from a vote.
 *
 * Only valid for Multiple Choice votes.
 *
 * @param vote				Vote Handle.
 * @noreturn
 * @error					Invalid Handle or vote position, or if the vote is not multiple choice.
 */
native void NativeVotes_RemoveAllItems(NativeVote vote);

/**
 * Retrieves information about a vote item.
 *
 * @param vote				Vote Handle.
 * @param position			Position, starting from 0.
 * @param infoBuf			Info buffer.
 * @param infoBufLen		Maximum length of the info buffer.
 * @return					True on success, false if position is invalid.
 * @error					Invalid Handlem
 */
native bool NativeVotes_GetItem(NativeVote vote, 
						int position, 
						char[] infoBuf, 
						int infoBufLen,
						char[] dispBuf="",
						int dispBufLen=0);
						
/**
 * Returns the number of items in a vote.
 *
 * @param vote				Vote Handle.
 * @return					Number of items in the vote.
 * @error					Invalid Handle.
 */
native int NativeVotes_GetItemCount(NativeVote vote);

/**
 * Sets the vote's details for votes that support details
 * If this is a custom vote, use NativeVotes_SetTitle to set the vote's title.
 *
 * @param vote				Vote Handle.
 * @param argument			Details string. See vote types for what details stands for.
 * @noreturn
 * @error					Invalid Handle.
 */
native void NativeVotes_SetDetails(NativeVote vote, const char[] argument, any ...);

/**
 * Returns the text of a vote's details if set.
 *
 * @param vote				Vote Handle.
 * @param buffer			Buffer to store details.
 * @param maxlength			Maximum length of the buffer.
 * @noreturn
 * @error					Invalid Handle.
 */
native void NativeVotes_GetDetails(NativeVote vote, char[] buffer, int maxlength);

/**
 * Sets a custom vote's title
 *
 * @param vote				Vote Handle.
 * @param title			 	Vote title string.
 * @noreturn
 * @error					Invalid Handle.
 */
native void NativeVotes_SetTitle(NativeVote vote, const char[] argument, any ...);

/**
 * Return the vote's Title.
 * If not set, returns Details instead.
 *     This behavior is for compatibility with NativeVotes 0.8.0 and below.
 *
 * @param vote				Vote Handle.
 * @param buffer			Buffer to store title.
 * @param maxlength			Maximum length of the buffer.
 * @noreturn
 * @error					Invalid Handle.
 */
native void NativeVotes_GetTitle(NativeVote vote, char[] buffer, int maxlength);

/**
 * Sets the target userid for vote
 * This should be used instead of SetArgument for votes that target players
 * 
 * Also sets target SteamID
 * 
 * @param vote				Vote Handle.
 * @param userid			Client index of target player
 * @param setDetails		If true, also sets vote details to client's name
 * @noreturn
 * @error					Invalid Handle.
 */
native void NativeVotes_SetTarget(NativeVote vote, int client, bool setDetails=true);

/**
 * Returns the userid of a vote's target
 *
 * @param vote				Vote Handle.
 * @return					Client index of target player or 0 for no target or target disconnected since vote started
 * @error					Invalid Handle.
 */
native int NativeVotes_GetTarget(NativeVote vote);

/**
 * Get the Steam ID of a vote's target
 * Useful if the target has disconnect from the server during a vote.
 * This was added in specifically for Kick/Ban votes
 *
 * @param vote				Vote Handle.
 * @param buffer			Buffer to store steamId.  Should be 19 characters or more..
 * @param maxlength			Maximum length of the buffer.
 * @noreturn
 * @error					Invalid Handle.
 */
native void NativeVotes_GetTargetSteam(NativeVote vote, char[] buffer, int maxlength);

/**
 * Returns whether a vote is in progress.
 *
 * @return					True if a NativeVotes vote is in progress, false otherwise.
 */
native bool NativeVotes_IsVoteInProgress();

/**
 * Returns a style's maximum items
 * 
 * @return Maximum items
 */
native int NativeVotes_GetMaxItems();

/**
 * Sets a vote's option flags.
 *
 * If a certain bit is not supported, it will be stripped before being set.
 * 
 * NOTE: This is currently unused, but reserved for future use.
 *
 * @param menu				Builtin Vote Handle.
 * @param flags				A new bitstring of VOTEFLAG bits.
 * @noreturn
 * @error					Invalid Handle.
 */
native void NativeVotes_SetOptionFlags(NativeVote vote, int flags);

/**
 * Retrieves a menu's option flags.
 *
 * NOTE: This is currently unused, but reserved for future use.
 *
 * @param vote				Builtin Vote Handle.
 * @return					A bitstring of VOTEFLAG bits.
 * @error					Invalid Handle.
 */
native int NativeVotes_GetOptionFlags(NativeVote vote);

/**
 * Cancels the vote in progress.
 *
 * @noreturn
 * @error					If no vote is in progress.
 */
native void NativeVotes_Cancel();

/**
 * Callback for when a vote has ended and results are available.
 * 
 * Due to SourceMod Forward limitations in plugins, multi-dimension arrays can't be passed
 * to forwards.  This means we have to split the client_info and item_info arrays into
 * their components.
 * 
 * @param vote			The vote being voted on.
 * @param num_votes		Number of votes tallied in total.
 * @param num_clients		Number of clients who could vote.
 * @param client_indexes	Array of client indexes. Parallel with client_votes.
 * @param client_votes		Array of client votes. Parallel with client_indexes.
 * @param num_items		Number of unique items that were selected.
 * @param item_indexes		Array of vote item indexes. Parallel with item_votes..
 * @param item_votes		Array of vote vote counts. Parallel with item_indexes.
 * @noreturn
 */
typedef NativeVotes_VoteHandler = function void(NativeVote vote,
							int num_votes,
							int num_clients,
							const int[] client_indexes,
							const int[] client_votes,
							int num_items,
							const int[] item_indexes,
							const int[] item_votes);


/**
 * Function to convert client/vote arrays into their two-dimensional versions,
 * which can then be passed to a standard vote handler.
 * 
 * NOTE:  It's recommended you have a separate handler for NativeVotes instead.
 * This is particularly true in SM 1.7 as NativeVotes now has its own VoteHandler.
 * 
 * client_info and item_info are the resulting arrays.
 * 
 * Note: When declaring client_info and item_info, you'll probably want to declare them like this:
 * new client_info[num_clients][2];
 * new item_info[num_items][2];
 * 
 * @param num_clients		Number of clients who could vote.
 * @param client_indexes	Array of client indexes. Parallel with client_votes.
 * @param client_votes		Array of client votes. Parallel with client_indexes.
 * @param num_items		Number of unique items that were selected.
 * @param item_indexes		Array of vote item indexes. Parallel with item_votes..
 * @param item_votes		Array of vote vote counts. Parallel with item_indexes.
 * @param client_info		Array of clients.  Use VOTEINFO_CLIENT_ defines.
 * @param item_info		Array of items, sorted by count.  Use VOTEINFO_ITEM
 *                         defines.
 * @noreturn
 */
stock void NativeVotes_FixResults(int num_clients,
							const int[] client_indexes,
							const int[] client_votes,
							int num_items,
							const int[] item_indexes,
							const int[] item_votes,
							int[][] client_info, 
							int[][] item_info)
{
	for (int i = 0; i < num_clients; ++i)
	{
		client_info[i][VOTEINFO_CLIENT_INDEX] = client_indexes[i];
		client_info[i][VOTEINFO_CLIENT_ITEM] = client_votes[i];
	}
	
	for (int i = 0; i < num_items; ++i)
	{
		item_info[i][VOTEINFO_ITEM_INDEX] = item_indexes[i];
		item_info[i][VOTEINFO_ITEM_VOTES] = item_votes[i];
	}
}

/**
 * Sets an advanced vote handling callback.  If this callback is set,
 * MenuAction_VoteEnd will not be called.
 *
 * @param vote			NativeVotes Handle.
 * @param callback		Callback function.
 * @noreturn
 * @error				Invalid Handle or callback.
 */
native void NativeVotes_SetResultCallback(NativeVote vote, NativeVotes_VoteHandler callback);

/**
 * Returns the number of seconds you should "wait" before displaying
 * a public vote.  This number is the time remaining until
 * (last_vote + sm_vote_delay).
 *
 * @return					Number of seconds to wait, or 0 for none.
 */
native int NativeVotes_CheckVoteDelay();

/**
 * Returns whether a client is in the pool of clients allowed 
 * to participate in the current vote.  This is determined by 
 * the client list passed to NativeVotes_Display().
 *
 * @param client			Client index.
 * @return					True if client is allowed to vote, false otherwise.
 * @error					If no vote is in progress or client index is invalid.
 */
native bool NativeVotes_IsClientInVotePool(int client);

/**
 * Redraws the current vote to a client in the voting pool.
 *
 * @param client			Client index.
 * @param revotes			True to allow revotes, false otherwise.
 * @return					True on success, false if the client is in the vote pool 
 *							but cannot vote again.
 * @error					No vote in progress, client is not in the voting pool, 
 *							or client index is invalid.
 */
native bool NativeVotes_RedrawClientVote(int client, bool revotes=true);

/**
 * Retrieve the vote type
 * 
 * @param vote				NativeVotes Handle.
 * @return					The built in vote type
 * @error					Invalid Handle
 */
native NativeVotesType NativeVotes_GetType(NativeVote vote);

/**
 * Set the team this vote is for, or NATIVEVOTES_ALL_TEAMS for all teams.
 * 
 * Defaults to NATIVEVOTES_ALL_TEAMS if not explicitly set.
 * 
 * @param vote				NativeVotes Handle.
 * @param team				Team number this vote is for
 * @noreturn
 * @error					Invalid Handle
 */
native void NativeVotes_SetTeam(NativeVote vote, int team);

/**
 * Retrieve the team this vote is for
 * 
 * @param vote				NativeVotes Handle.
 * @return					Team index or NATIVEVOTES_ALL_TEAMS for all teams.
 * @error					Invalid Handle
 */
native int NativeVotes_GetTeam(NativeVote vote);

/**
 * Set the client index of the player who initiated the vote.
 * Use NATIVEVOTES_SERVER_INDEX if initiated by the server itself.
 * 
 * Defaults to NATIVEVOTES_SERVER_INDEX if not explicitly set.
 * 
 * @param vote			NativeVotes Handle.
 * @param client			Client who initiated the vote or NATIVEVOTES_SERVER_INDEX
 * @noreturn
 * @error				Invalid Handle
 */
native void NativeVotes_SetInitiator(NativeVote vote, int client);

/**
 * Retrieve the client index of the player who initiated the vote or NATIVEVOTES_SERVER_INDEX if 
 * initiated by the server itself.
 * 
 * @param					Vote handle
 * @return					Client index or NATIVEVOTES_SERVER_INDEX
 * @error					Invalid Handle
 */
native int NativeVotes_GetInitiator(NativeVote vote);

/**
 * Broadcasts a vote to a list of clients.  The most selected item will be 
 * returned through MenuAction_VoteEnd.  On a tie, a random item will be returned 
 * from a list of the tied items.
 *
 * Note that MenuAction_VoteStart, MenuAction_VoteCancel, MenuAction_VoteEnd, and MenuAction_End are all
 * default callbacks and do not need to be enabled.
 *
 * @param vote				Vote Handle.
 * @param clients				Array of clients to broadcast to.
 * @param numClients			Number of clients in the array.
 * @param time				Maximum time to leave menu on the screen.
 * @return					True on success, false if a vote is already in progress.
 * @error					Invalid Handle, or a vote is already in progress.
 */
native bool NativeVotes_Display(NativeVote vote, int[] clients, int numClients, int time, int flags=0);

/**
 * Sends a vote menu to all clients.  See NativeVotes_Display() for more information.
 *
 * @param vote				Vote Handle.
 * @param time				Maximum time to leave menu on the screen.
 * @return					True on success, false if this menu already has a vote session
 *							in progress.
 * @error					Invalid Handle, or a vote is already in progress.
 */
stock bool NativeVotes_DisplayToAll(NativeVote vote, int time, int flags=0)
{
	int total = 0;
	int[] players = new int[MaxClients];
	for (int i=1; i<=MaxClients; i++)
	{
		if (!IsClientInGame(i) || IsFakeClient(i))
			continue;
		players[total++] = i;
	}
	return NativeVotes_Display(vote, players, total, time, flags);
}

/**
 * Sends a vote menu to a single team.  See NativeVotes_Display() for more information.
 *
 * @param vote				Vote Handle.
 * @param team				Team to send vote to. 1 = spectators, 2 = RED/Survivors/Terrorists, 3 = BLU/Infected/Counter-Terrorists
 * @param time				Maximum time to leave menu on the screen.
 * @return					True on success, false if this menu already has a vote session
 *							in progress.
 * @error					Invalid Handle, or a vote is already in progress.
 */
stock bool NativeVotes_DisplayToTeam(NativeVote vote, int team, int time)
{
	NativeVotes_SetTeam(vote, team);

	int total;
	int[] players = new int[MaxClients];
	
	for (int i=1; i<=MaxClients; i++)
	{
		if (!IsClientInGame(i) || IsFakeClient(i) || (GetClientTeam(i) != team))
		{
			continue;
		}
		players[total++] = i;
	}
	
	return NativeVotes_Display(vote, players, total, time);
}

/**
 * Sends a vote menu to all clients who are not spectators or waiting to choose a team.  See NativeVotes_Display() for more information.
 *
 * @param vote				Vote Handle.
 * @param time				Maximum time to leave menu on the screen.
 * @return					True on success, false if this menu already has a vote session
 *							in progress.
 * @error					Invalid Handle, or a vote is already in progress.
 */
stock bool NativeVotes_DisplayToAllNonSpectators(NativeVote vote, int time)
{
	int total;
	int[] players = new int[MaxClients];
	
	for (int i=1; i<=MaxClients; i++)
	{
		if (!IsClientInGame(i) || IsFakeClient(i) || (GetClientTeam(i) < 2))
		{
			continue;
		}
		players[total++] = i;
	}
	
	return NativeVotes_Display(vote, players, total, time);
}

/**
 * Display vote passed screen
 *
 * You MUST call one of the NativeVotesDisplayPass* or NativeVotes_DisplayFail functions
 * to hide the vote screen for users who didn't vote, and to clear out their selection
 * for the next vote.
 * 
 * @param vote				Vote handle
 * @param details			Normally the item that won the vote or format string. Also used for custom vote winners
 * @param ...				Variable number of format parameters.
 * @noreturn
 */
native void NativeVotes_DisplayPass(NativeVote vote, const char[] details="", any ...);

/**
 * Display vote passed screen with custom text to a single user
 *
 * You MUST call one of the NativeVotesDisplayPass* or NativeVotes_DisplayFail functions
 * to hide the vote screen for users who didn't vote, and to clear out their selection
 * for the next vote.
 * 
 * @param vote				Vote handle
 * @param client			client to display to
 * @param format			A format string.
 * @param any				Variable number of format parameters
 * @noreturn
 */
native void NativeVotes_DisplayPassCustomToOne(NativeVote vote, int client, const char[] format, any ...);

/**
 * Display vote passed screen with custom text
 *
 * You MUST call one of the NativeVotesDisplayPass* or NativeVotes_DisplayFail functions
 * to hide the vote screen for users who didn't vote, and to clear out their selection
 * for the next vote.
 * 
 * @param vote				Vote handle
 * @param format			A format string.
 * @param any				Variable number of format parameters
 * @noreturn
 */
stock void NativeVotes_DisplayPassCustom(NativeVote vote, const char[] format, any ...)
{
	char buffer[192];
	
	for (int i = 1; i <= MaxClients; ++i)
	{
		if (IsClientInGame(i))
		{
			SetGlobalTransTarget(i);
			VFormat(buffer, sizeof(buffer), format, 3);
			NativeVotes_DisplayPassCustomToOne(vote, i, "%s", buffer);
		}
	}
}

/**
 * Display vote passed screen with a custom type.
 *
 * A sample usage of this would be if Extend won an RTV vote: NativeVotes_DisplayPassEx(vote, NativeVotesPass_Extend, map);
 *
 * You MUST call one of NativeVotes_DisplayPass, NativeVotes_DisplayPassEx,
 * or NativeVotes_DisplayFail to hide the vote screen for users who didn't vote
 * and to clear out their selection for the next vote.
 * 
 * @param vote				Vote handle
 * @param passType			The pass screen to display
 * @param details			Normally the item that won the vote. Also used for custom vote winners
 * @noreturn
 */
native void NativeVotes_DisplayPassEx(NativeVote vote, NativeVotesPassType passType, const char[] details="", any ...);

/**
 * Display failure screen.
 *
 * You MUST call one of NativeVotes_DisplayPass, NativeVotes_DisplayPassEx,
 * or NativeVotes_DisplayFail to hide the vote screen for users who didn't vote,
 * and to clear out their selection for the next vote.
 * 
 * @param reason			Vote failure reason from NativeVotesFailType enum
 * @noreturn
 */
native void NativeVotes_DisplayFail(NativeVote vote, NativeVotesFailType reason=NativeVotesFail_Generic);

/**
 * Quick stock to determine whether voting is allowed.  This doesn't let you 
 * fine-tune a reason for not voting, so it's not recommended for lazily 
 * telling clients that voting isn't allowed.
 * 
 * @return				True if voting is allowed, false if voting is in progress
 *						or the cooldown is active.
 */
stock bool NativeVotes_IsNewVoteAllowed()
{
	if (NativeVotes_IsVoteInProgress() || NativeVotes_CheckVoteDelay() != 0)
	{
		return false;
	}
	
	return true;
}

/**
 *  Send a call vote fail screen to a user
 * Used to respond to a callvote with invalid arguments or for other reasons
 * (such as trying to target an admin for a kick/ban vote)
 * 
 * @param client		The client to display the fail screen to
 * @param reason		A vote call fail reason
 * @param time		For NativeVotesCallFail_Recent, the number of seconds until the vote
 *                     can be called again
 */
native void NativeVotes_DisplayCallVoteFail(int client, NativeVotesCallFailType reason, int time);

/**
 * Redraws the vote title from inside a MenuAction_Display callback
 * Not supported on L4D
 * 
 * @param text			Vote title to draw
 * @error 				If called from outside MenuAction_Display
 * @return				Plugin_Changed if the change is allowed, Plugin_Continue if it isn't.
 */
native Action NativeVotes_RedrawVoteTitle(const char[] text);

/**
 * Redraws the vote text from inside a MenuAction_DisplayItem callback.
 * Only supported on multiple-choice votes
 * 
 * @param text			Vote text to draw.
 * @error 				If called from outside MenuAction_DisplayItem
 * @return				Plugin_Changed if the change is allowed, Plugin_Continue if it isn't.
 */
native Action NativeVotes_RedrawVoteItem(const char[] text);

/**
 * Retrieves voting information from MenuAction_VoteEnd.
 *
 * @param param2		Second parameter of MenuAction_VoteEnd.
 * @param winningVotes	Number of votes received by the winning option.
 * @param totalVotes	Number of total votes received.
 * @noreturn
 */
stock void NativeVotes_GetInfo(int param2, int &winningVotes, int &totalVotes)
{
	winningVotes = param2 & 0xFFFF;
	totalVotes = param2 >> 16;
}

/**
 * Do not edit below this line!
 */
public SharedPlugin __pl_nativevotes = 
{
	name = "nativevotes",
	file = "nativevotes.smx",
#if defined REQUIRE_PLUGINS
	required = 1,
#else
	required = 0,
#endif
};

public void __pl_nativevotes_SetNTVOptional()
{
	MarkNativeAsOptional("NativeVotes_IsVoteTypeSupported");
	MarkNativeAsOptional("NativeVotes_Create");
	MarkNativeAsOptional("NativeVotes_Close");
	MarkNativeAsOptional("NativeVotes_AddItem");
	MarkNativeAsOptional("NativeVotes_InsertItem");
	MarkNativeAsOptional("NativeVotes_RemoveItem");
	MarkNativeAsOptional("NativeVotes_RemoveAllItems");
	MarkNativeAsOptional("NativeVotes_GetItem");
	MarkNativeAsOptional("NativeVotes_GetItemCount");
	MarkNativeAsOptional("NativeVotes_SetDetails");
	MarkNativeAsOptional("NativeVotes_GetDetails");
	MarkNativeAsOptional("NativeVotes_SetTitle");
	MarkNativeAsOptional("NativeVotes_GetTitle");
	MarkNativeAsOptional("NativeVotes_SetTarget");
	MarkNativeAsOptional("NativeVotes_GetTarget");
	MarkNativeAsOptional("NativeVotes_GetTargetSteam");
	MarkNativeAsOptional("NativeVotes_IsVoteInProgress");
	MarkNativeAsOptional("NativeVotes_GetMaxItems");
	MarkNativeAsOptional("NativeVotes_SetOptionFlags");
	MarkNativeAsOptional("NativeVotes_GetOptionFlags");
	MarkNativeAsOptional("NativeVotes_Cancel");
	MarkNativeAsOptional("NativeVotes_SetResultCallback");
	MarkNativeAsOptional("NativeVotes_CheckVoteDelay");
	MarkNativeAsOptional("NativeVotes_IsClientInVotePool");
	MarkNativeAsOptional("NativeVotes_RedrawClientVote");
	MarkNativeAsOptional("NativeVotes_RedrawClientVote");
	MarkNativeAsOptional("NativeVotes_GetType");
	MarkNativeAsOptional("NativeVotes_SetTeam");
	MarkNativeAsOptional("NativeVotes_GetTeam");
	MarkNativeAsOptional("NativeVotes_SetInitiator");
	MarkNativeAsOptional("NativeVotes_GetInitiator");
	MarkNativeAsOptional("NativeVotes_Display");
	MarkNativeAsOptional("NativeVotes_DisplayPass");
	MarkNativeAsOptional("NativeVotes_DisplayPassCustomToOne");
	MarkNativeAsOptional("NativeVotes_DisplayPassEx");
	MarkNativeAsOptional("NativeVotes_DisplayFail");
	MarkNativeAsOptional("NativeVotes_RegisterVoteManager");
	MarkNativeAsOptional("NativeVotes_DisplayCallVoteFail");
	MarkNativeAsOptional("NativeVotes_RedrawVoteTitle");
	MarkNativeAsOptional("NativeVotes_RedrawVoteItem");
}
