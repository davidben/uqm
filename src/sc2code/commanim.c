//Copyright Paul Reiche, Fred Ford. 1992-2002

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#define COMM_INTERNAL
#include "commanim.h"

#include "comm.h"
#include "element.h"
#include "setup.h"
#include "libs/compiler.h"
#include "libs/graphics/gfx_common.h"
#include "libs/mathlib.h"


// Maximum ambient animation frame rate (actual execution rate)
// A gfx frame is not always produced during an execution frame,
// and several animations are combined into one gfx frame.
// The rate was originally 120fps which allowed for more animation
// precision which is ultimately wasted on the human eye anyway.
// The highest known stable animation rate is 40fps, so that's what we use.
#define AMBIENT_ANIM_RATE   40

// Oscilloscope frame rate
// Should be <= AMBIENT_ANIM_RATE
// XXX: was 32 picked experimentally?
#define OSCILLOSCOPE_RATE   32

static int ambient_anim_task (void *data);

volatile BOOLEAN PauseAnimTask = FALSE;


static void
SetUpSequence (PSEQUENCE pSeq)
{
	COUNT i;
	ANIMATION_DESCPTR ADPtr;

	i = CommData.NumAnimations;
	pSeq = &pSeq[i];
	ADPtr = &CommData.AlienAmbientArray[i];
	while (i--)
	{
		--ADPtr;
		--pSeq;

		if (ADPtr->AnimFlags & COLORXFORM_ANIM)
			pSeq->AnimType = COLOR_ANIM;
		else
			pSeq->AnimType = PICTURE_ANIM;
		pSeq->Direction = UP_DIR;
		pSeq->FramesLeft = ADPtr->NumFrames;
		if (pSeq->AnimType == COLOR_ANIM)
			pSeq->AnimObj.CurCMap = SetAbsColorMapIndex (
					CommData.AlienColorMap, ADPtr->StartIndex);
		else
			pSeq->AnimObj.CurFrame = SetAbsFrameIndex (
					CommData.AlienFrame, ADPtr->StartIndex);

		if (ADPtr->AnimFlags & RANDOM_ANIM)
		{
			if (pSeq->AnimType == COLOR_ANIM)
				pSeq->AnimObj.CurCMap =
						SetRelColorMapIndex (pSeq->AnimObj.CurCMap,
						(COUNT)((COUNT)TFB_Random () % pSeq->FramesLeft));
			else
				pSeq->AnimObj.CurFrame =
						SetRelFrameIndex (pSeq->AnimObj.CurFrame,
						(COUNT)((COUNT)TFB_Random () % pSeq->FramesLeft));
		}
		else if (ADPtr->AnimFlags & YOYO_ANIM)
		{
			--pSeq->FramesLeft;
			if (pSeq->AnimType == COLOR_ANIM)
				pSeq->AnimObj.CurCMap =
						SetRelColorMapIndex (pSeq->AnimObj.CurCMap, 1);
			else
				pSeq->AnimObj.CurFrame =
						IncFrameIndex (pSeq->AnimObj.CurFrame);
		}

		pSeq->Alarm = ADPtr->BaseRestartRate
				+ ((COUNT)TFB_Random () % (ADPtr->RandomRestartRate + 1)) + 1;
	}
}

static int
ambient_anim_task (void *data)
{
	SIZE TalkAlarm;
	FRAME TalkFrame;
	FRAME ResetTalkFrame = NULL;
	FRAME TransitionFrame = NULL;
	FRAME AnimFrame[MAX_ANIMATIONS + 1];
	COUNT i;
	DWORD LastTime;
	FRAME CommFrame;
	SEQUENCE Sequencer[MAX_ANIMATIONS];
	ANIMATION_DESC TalkBuffer = CommData.AlienTalkDesc;
	PSEQUENCE pSeq;
	ANIMATION_DESCPTR ADPtr;
	DWORD ActiveMask;
	DWORD LastOscillTime;
	Task task = (Task) data;
	BOOLEAN TransitionDone = FALSE;
	BOOLEAN TalkFrameChanged = FALSE;
	BOOLEAN FrameChanged[MAX_ANIMATIONS];
	BOOLEAN ColorChange = FALSE;

	while ((CommFrame = CommData.AlienFrame) == 0
			&& !Task_ReadState (task, TASK_EXIT))
		TaskSwitch ();

	LockMutex (GraphicsLock);
	memset ((PSTR)&DisplayArray[0], 0, sizeof (DisplayArray));
	SetUpSequence (Sequencer);
	UnlockMutex (GraphicsLock);

	ActiveMask = 0;
	TalkAlarm = 0;
	TalkFrame = 0;
	LastTime = GetTimeCounter ();
	LastOscillTime = LastTime;
	memset (FrameChanged, 0, sizeof (FrameChanged));
	memset (AnimFrame, 0, sizeof (AnimFrame));
	for (i = 0; i <= CommData.NumAnimations; i++)
		if (CommData.AlienAmbientArray[i].AnimFlags & YOYO_ANIM)
			AnimFrame[i] =  SetAbsFrameIndex (CommFrame,
					CommData.AlienAmbientArray[i].StartIndex);
		else
			AnimFrame[i] =  SetAbsFrameIndex (CommFrame,
					(COUNT)(CommData.AlienAmbientArray[i].StartIndex
					+ CommData.AlienAmbientArray[i].NumFrames - 1));

	while (!Task_ReadState (task, TASK_EXIT))
	{
		BOOLEAN Change, CanTalk;
		DWORD CurTime, ElapsedTicks;

		SleepThreadUntil (LastTime + ONE_SECOND / AMBIENT_ANIM_RATE);

		LockMutex (GraphicsLock);
		BatchGraphics ();
		CurTime = GetTimeCounter ();
		ElapsedTicks = CurTime - LastTime;
		LastTime = CurTime;

		Change = FALSE;
		i = CommData.NumAnimations;
		if (CommData.AlienFrame)
			CanTalk = TRUE;
		else
		{
			i = 0;
			CanTalk = FALSE;
		}

		pSeq = &Sequencer[i];
		ADPtr = &CommData.AlienAmbientArray[i];
		while (i-- && !Task_ReadState (task, TASK_EXIT))
		{
			--ADPtr;
			--pSeq;
			if (ADPtr->AnimFlags & ANIM_DISABLED)
				continue;
			if (pSeq->Direction == NO_DIR)
			{
				if (!(ADPtr->AnimFlags
						& CommData.AlienTalkDesc.AnimFlags & WAIT_TALKING))
					pSeq->Direction = UP_DIR;
			}
			else if ((DWORD)pSeq->Alarm > ElapsedTicks)
				pSeq->Alarm -= (COUNT)ElapsedTicks;
			else
			{
				if (!(ActiveMask & ADPtr->BlockMask)
						&& (--pSeq->FramesLeft
						|| ((ADPtr->AnimFlags & YOYO_ANIM)
						&& pSeq->Direction == UP_DIR)))
				{
					ActiveMask |= 1L << i;
					pSeq->Alarm = ADPtr->BaseFrameRate
							+ ((COUNT)TFB_Random ()
							% (ADPtr->RandomFrameRate + 1)) + 1;
				}
				else
				{
					ActiveMask &= ~(1L << i);
					pSeq->Alarm = ADPtr->BaseRestartRate
							+ ((COUNT)TFB_Random ()
							% (ADPtr->RandomRestartRate + 1)) + 1;
					if (ActiveMask & ADPtr->BlockMask)
						continue;
				}

				if (pSeq->AnimType == COLOR_ANIM)
				{
					XFormColorMap (GetColorMapAddress (pSeq->AnimObj.CurCMap),
							(COUNT) (pSeq->Alarm - 1));
				}
				else
				{
					Change = TRUE;
					AnimFrame[i] = pSeq->AnimObj.CurFrame;
					FrameChanged[i] = 1;
				}

				if (pSeq->FramesLeft == 0)
				{
					pSeq->FramesLeft = (BYTE)(ADPtr->NumFrames - 1);

					if (pSeq->Direction == DOWN_DIR)
						pSeq->Direction = UP_DIR;
					else if (ADPtr->AnimFlags & YOYO_ANIM)
						pSeq->Direction = DOWN_DIR;
					else
					{
						++pSeq->FramesLeft;
						if (pSeq->AnimType == COLOR_ANIM)
							pSeq->AnimObj.CurCMap = SetRelColorMapIndex (
									pSeq->AnimObj.CurCMap,
									(SWORD) (-pSeq->FramesLeft));
						else
						{
							pSeq->AnimObj.CurFrame = SetRelFrameIndex (
									pSeq->AnimObj.CurFrame,
									(SWORD) (-pSeq->FramesLeft));
						}
					}
				}

				if (ADPtr->AnimFlags & RANDOM_ANIM)
				{
					COUNT nextIndex = ADPtr->StartIndex +
							(TFB_Random () % ADPtr->NumFrames);

					if (pSeq->AnimType == COLOR_ANIM)
						pSeq->AnimObj.CurCMap = SetAbsColorMapIndex (
								pSeq->AnimObj.CurCMap, nextIndex);
					else
						pSeq->AnimObj.CurFrame = SetAbsFrameIndex (
								pSeq->AnimObj.CurFrame, nextIndex);
				}
				else if (pSeq->AnimType == COLOR_ANIM)
				{
					if (pSeq->Direction == UP_DIR)
						pSeq->AnimObj.CurCMap = SetRelColorMapIndex (
								pSeq->AnimObj.CurCMap, 1);
					else
						pSeq->AnimObj.CurCMap = SetRelColorMapIndex (
								pSeq->AnimObj.CurCMap, -1);
				}
				else
				{
					if (pSeq->Direction == UP_DIR)
						pSeq->AnimObj.CurFrame =
								IncFrameIndex (pSeq->AnimObj.CurFrame);
					else
						pSeq->AnimObj.CurFrame =
								DecFrameIndex (pSeq->AnimObj.CurFrame);
				}
			}

			if (pSeq->AnimType == PICTURE_ANIM
					&& (ADPtr->AnimFlags
					& CommData.AlienTalkDesc.AnimFlags & WAIT_TALKING)
					&& pSeq->Direction != NO_DIR)
			{
				COUNT index;

				CanTalk = FALSE;
				if (!(pSeq->Direction != UP_DIR
						|| (index = GetFrameIndex (pSeq->AnimObj.CurFrame)) >
						ADPtr->StartIndex + 1
						|| (index == ADPtr->StartIndex + 1
						&& (ADPtr->AnimFlags & CIRCULAR_ANIM))))
					pSeq->Direction = NO_DIR;
			}
		}

		ADPtr = &CommData.AlienTalkDesc;
		if (CanTalk
				&& ADPtr->NumFrames
				&& (ADPtr->AnimFlags & WAIT_TALKING)
				&& !(CommData.AlienTransitionDesc.AnimFlags & PAUSE_TALKING))
		{
			BOOLEAN done = FALSE;
			for (i = 0; i < CommData.NumAnimations; i++)
				if (ActiveMask & (1L << i)
					&& CommData.AlienAmbientArray[i].AnimFlags & WAIT_TALKING)
					done = TRUE;
			if (!done)
			{

				if (ADPtr->StartIndex != TalkBuffer.StartIndex)
				{
					Change = TRUE;
					ResetTalkFrame = SetAbsFrameIndex (CommFrame,
							TalkBuffer.StartIndex);
					TalkBuffer = CommData.AlienTalkDesc;
				}

				if ((long)TalkAlarm > (long)ElapsedTicks)
					TalkAlarm -= (SIZE)ElapsedTicks;
				else
				{
					BYTE AFlags;
					SIZE FrameRate;

					if (TalkAlarm > 0)
						TalkAlarm = 0;
					else
						TalkAlarm = -1;

					AFlags = ADPtr->AnimFlags;
					if (!(AFlags & (TALK_INTRO | TALK_DONE)))
					{
						FrameRate =
								ADPtr->BaseFrameRate
								+ ((COUNT)TFB_Random ()
								% (ADPtr->RandomFrameRate + 1));
						if (TalkAlarm < 0
								|| GetFrameIndex (TalkFrame) ==
								ADPtr->StartIndex)
						{
							TalkFrame = SetAbsFrameIndex (CommFrame,
									(COUNT) (ADPtr->StartIndex + 1
									+ ((COUNT)TFB_Random ()
									% (ADPtr->NumFrames - 1))));
							FrameRate += ADPtr->BaseRestartRate
									+ ((COUNT)TFB_Random ()
									% (ADPtr->RandomRestartRate + 1));
						}
						else
						{
							TalkFrame = SetAbsFrameIndex (CommFrame,
									ADPtr->StartIndex);
							if (ADPtr->AnimFlags & PAUSE_TALKING)
							{
								if (!(CommData.AlienTransitionDesc.AnimFlags
										& TALK_DONE))
								{
									CommData.AlienTransitionDesc.AnimFlags |=
											PAUSE_TALKING;
									ADPtr->AnimFlags &= ~PAUSE_TALKING;
								}
								else if (CommData.AlienTransitionDesc.NumFrames)
									ADPtr->AnimFlags |= TALK_DONE;
								else
									ADPtr->AnimFlags &=
											~(WAIT_TALKING | PAUSE_TALKING);

								FrameRate = 0;
							}
						}
					}
					else
					{
						ADPtr = &CommData.AlienTransitionDesc;
						if (AFlags & TALK_INTRO)
						{
							FrameRate = ADPtr->BaseFrameRate
									+ ((COUNT)TFB_Random ()
									% (ADPtr->RandomFrameRate + 1));
							if (TalkAlarm < 0 || TransitionDone)
							{
								TalkFrame = SetAbsFrameIndex (CommFrame,
										ADPtr->StartIndex);
								TransitionDone = FALSE;
							}
							else
								TalkFrame = IncFrameIndex (TalkFrame);

							if ((BYTE)(GetFrameIndex (TalkFrame)
									- ADPtr->StartIndex + 1) ==
									ADPtr->NumFrames)
							{
								CommData.AlienTalkDesc.AnimFlags &=
										~TALK_INTRO;
								TransitionDone = TRUE;
							}
							TransitionFrame = TalkFrame;
						}
						else /* if (AFlags & TALK_DONE) */
						{
							FrameRate = ADPtr->BaseFrameRate
									+ ((COUNT)TFB_Random ()
									% (ADPtr->RandomFrameRate + 1));
							if (TalkAlarm < 0)
								TalkFrame = SetAbsFrameIndex (CommFrame,
										(COUNT) (ADPtr->StartIndex
										+ ADPtr->NumFrames - 1));
							else
								TalkFrame = DecFrameIndex (TalkFrame);

							if (GetFrameIndex (TalkFrame) ==
									ADPtr->StartIndex)
							{
								CommData.AlienTalkDesc.AnimFlags &=
										~(PAUSE_TALKING | TALK_DONE);
								if (ADPtr->AnimFlags & TALK_INTRO)
									CommData.AlienTalkDesc.AnimFlags &=
											~WAIT_TALKING;
								else
								{
									ADPtr->AnimFlags |= PAUSE_TALKING;
									ADPtr->AnimFlags &= ~TALK_DONE;
								}
								FrameRate = 0;
							}
							TransitionFrame = NULL;
						}
					}
					TalkFrameChanged = TRUE;

					Change = TRUE;

					TalkAlarm = FrameRate;
				}
			}
			else if (done && (ADPtr->AnimFlags & PAUSE_TALKING))
			{
				ADPtr->AnimFlags &= ~(WAIT_TALKING | PAUSE_TALKING);	
			}
		}

		if (!PauseAnimTask)
		{
			CONTEXT OldContext;
			BOOLEAN CheckSub = FALSE;
			BOOLEAN ClearSub;
			SUBTITLE_STATE sub_state;

			ClearSub = SetClearSubtitle (FALSE, &sub_state);

			OldContext = SetContext (TaskContext);

			if (ColorChange || ClearSummary)
			{
				FRAME F;
				F = CommData.AlienFrame;
				CommData.AlienFrame = CommFrame;
				DrawAlienFrame (TalkFrame,
						&Sequencer[CommData.NumAnimations - 1]);
				CommData.AlienFrame = F;
				CheckSub = TRUE;
				ClearSub = ClearSummary;
				ColorChange = FALSE;
				ClearSummary = FALSE;
			}
			if (Change || ClearSub)
			{
				STAMP s;
				s.origin.x = -SAFE_X;
				s.origin.y = 0;
				if (ClearSub)
				{
					s.frame = CommFrame;
					DrawStamp (&s);
				}
				i = CommData.NumAnimations;
				while (i--)
				{
					if (ClearSub || FrameChanged[i])
					{
						s.frame = AnimFrame[i];
						DrawStamp (&s);
						FrameChanged[i] = 0;
					}
				}
				if (ClearSub && TransitionFrame)
				{
					s.frame = TransitionFrame;
					DrawStamp (&s);
				}
				if (ResetTalkFrame)
				{
					s.frame = ResetTalkFrame;
					DrawStamp (&s);
					ResetTalkFrame = NULL;
				}
				if (TalkFrame && TalkFrameChanged)
				{
					s.frame = TalkFrame;
					DrawStamp (&s);
					TalkFrameChanged = FALSE;
				}
				Change = FALSE;
				CheckSub = TRUE;
			}

			if (CheckSub && sub_state >= SPACE_SUBTITLE)
				RedrawSubtitles ();

			SetContext (OldContext);
		}
		if (LastOscillTime + (ONE_SECOND / OSCILLOSCOPE_RATE) < CurTime)
		{
			LastOscillTime = CurTime;
			UpdateSpeechGraphics (FALSE);
		}
		UnbatchGraphics ();
		UnlockMutex (GraphicsLock);
		ColorChange = XFormColorMap_step ();
	}
	FinishTask (task);
	return 0;
}

Task
StartCommAnimTask(void)
{
	return AssignTask (ambient_anim_task, 3072, "ambient animations");
}


