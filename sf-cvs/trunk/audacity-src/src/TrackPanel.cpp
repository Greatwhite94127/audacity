/**********************************************************************

  Audacity: A Digital Audio Editor

  TrackPanel.cpp

  Dominic Mazzoni

  AS: The TrackPanel class is responsible for rendering the panel
      displayed to the left of a track.  TrackPanel also takes care
      of the functionality for each of the buttons in that panel.

**********************************************************************/

#include <math.h>
#include <algorithm>

#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/image.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <wx/textctrl.h>
#include <wx/statusbr.h>

#include "TrackPanel.h"

#include "APalette.h"
#include "AColor.h"
#include "AudioIO.h"
#include "LabelTrack.h"
#include "NoteTrack.h"
#include "Track.h"
#include "TrackArtist.h"
#include "Project.h"
#include "WaveTrack.h"

#ifdef BOUNCE
#include "Bounce.h"
extern Bounce *gBounce;
#endif

#define kLeftInset 4
#define kTopInset 4

enum {
   TrackPanelFirstID = 2000,

   OnSetNameID,

   OnMoveUpID,
   OnMoveDownID,

   OnUpOctaveID,
   OnDownOctaveID,

   OnChannelLeftID,
   OnChannelRightID,
   OnChannelMonoID,

   OnRate8ID,
   OnRate11ID,
   OnRate16ID,
   OnRate22ID,
   OnRate44ID,
   OnRate48ID,
   OnRateOtherID,

   OnWaveformID,
   OnWaveformDBID,
   OnSpectrumID,
   OnPitchID,

   OnSplitStereoID,
   OnMergeStereoID
};

BEGIN_EVENT_TABLE(TrackPanel, wxWindow)
    EVT_MOUSE_EVENTS(TrackPanel::OnMouseEvent)

    EVT_CHAR(TrackPanel::OnKeyEvent)

    EVT_PAINT(TrackPanel::OnPaint)

    EVT_MENU(OnSetNameID, TrackPanel::OnSetName)

    EVT_MENU_RANGE(OnMoveUpID     , OnMoveDownID   , TrackPanel::OnMoveTrack    )
    EVT_MENU_RANGE(OnUpOctaveID   , OnDownOctaveID , TrackPanel::OnChangeOctave )
    EVT_MENU_RANGE(OnChannelLeftID, OnChannelMonoID, TrackPanel::OnChannelChange)
    EVT_MENU_RANGE(OnWaveformID   , OnPitchID      , TrackPanel::OnSetDisplay   )
    EVT_MENU_RANGE(OnRate8ID      , OnRate48ID     , TrackPanel::OnRateChange   )

    EVT_MENU(OnRateOtherID, TrackPanel::OnRateOther)
    EVT_MENU(OnSplitStereoID, TrackPanel::OnSplitStereo)
    EVT_MENU(OnMergeStereoID, TrackPanel::OnMergeStereo)
    END_EVENT_TABLE()

TrackPanel::TrackPanel(wxWindow * parent, wxWindowID id,
                           const wxPoint & pos,
                           const wxSize & size,
                           TrackList * tracks,
                           ViewInfo * viewInfo,
                           TrackPanelListener * listener):wxWindow(parent,
                                                                   id, pos,
                                                                   size,
                                                                   wxWANTS_CHARS),
mListener(listener), mTracks(tracks), mViewInfo(viewInfo), mBitmap(NULL),
mAutoScrolling(false)
{
   mIsClosing = false;
   mIsSelecting = false;
   mIsResizing = false;
   mIsSliding = false;
   mIsEnveloping = false;
   mIsMuting = false;
   mIsZooming = false;
   mIsSoloing = false;

   mIndicatorShowing = false;

   mArrowCursor = new wxCursor(wxCURSOR_ARROW);
   mSelectCursor = new wxCursor(wxCURSOR_IBEAM);
   mSlideCursor = new wxCursor(wxCURSOR_SIZEWE);
   mResizeCursor = new wxCursor(wxCURSOR_SIZENS);
   mZoomInCursor = new wxCursor(wxCURSOR_MAGNIFIER);
   mZoomOutCursor = new wxCursor(wxCURSOR_MAGNIFIER);

   mRateMenu = new wxMenu();
   mRateMenu->Append(OnRate8ID, "8000 Hz");
   mRateMenu->Append(OnRate11ID, "11025 Hz");
   mRateMenu->Append(OnRate16ID, "16000 Hz");
   mRateMenu->Append(OnRate22ID, "22050 Hz");
   mRateMenu->Append(OnRate44ID, "44100 Hz");
   mRateMenu->Append(OnRate48ID, "48000 Hz");
   mRateMenu->Append(OnRateOtherID, "Other...");

   mWaveTrackMenu = new wxMenu();
   mWaveTrackMenu->Append(OnSetNameID, "Name...");
   mWaveTrackMenu->AppendSeparator();
   mWaveTrackMenu->Append(OnMoveUpID, "Move Track Up");
   mWaveTrackMenu->Append(OnMoveDownID, "Move Track Down");
   mWaveTrackMenu->AppendSeparator();
   mWaveTrackMenu->Append(OnWaveformID, "Waveform");
   mWaveTrackMenu->Append(OnWaveformDBID, "Waveform (dB)");
   mWaveTrackMenu->Append(OnSpectrumID, "Spectrum");
   mWaveTrackMenu->Append(OnPitchID, "Pitch (EAC)");
   mWaveTrackMenu->AppendSeparator();
   mWaveTrackMenu->Append(OnChannelMonoID, "Mono");
   mWaveTrackMenu->Append(OnChannelLeftID, "Left Channel");
   mWaveTrackMenu->Append(OnChannelRightID, "Right Channel");
   mWaveTrackMenu->Append(OnMergeStereoID, "Make Stereo Track");
   mWaveTrackMenu->Append(OnSplitStereoID, "Split Stereo Track");
   mWaveTrackMenu->AppendSeparator();
   mWaveTrackMenu->Append(0, "Set Rate", mRateMenu);

   mNoteTrackMenu = new wxMenu();
   mNoteTrackMenu->Append(OnSetNameID, "Name...");
   mNoteTrackMenu->AppendSeparator();
   mNoteTrackMenu->Append(OnMoveUpID, "Move Track Up");
   mNoteTrackMenu->Append(OnMoveDownID, "Move Track Down");
   mNoteTrackMenu->AppendSeparator();
   mNoteTrackMenu->Append(OnUpOctaveID, "Up Octave");
   mNoteTrackMenu->Append(OnDownOctaveID, "Down Octave");

   mLabelTrackMenu = new wxMenu();
   mLabelTrackMenu->Append(OnSetNameID, "Name...");
   mLabelTrackMenu->AppendSeparator();
   mLabelTrackMenu->Append(OnMoveUpID, "Move Track Up");
   mLabelTrackMenu->Append(OnMoveDownID, "Move Track Down");

   mTrackArtist = new TrackArtist();
   mTrackArtist->SetInset(1, kTopInset + 1, kLeftInset + 2, 2);

   mCapturedTrack = NULL;

   mPopupMenuTarget = NULL;

   mTimeCount = 0;
   mTimer.parent = this;
   mTimer.Start(50, FALSE);
}

TrackPanel::~TrackPanel()
{
   if (mBitmap)
      delete mBitmap;

   delete mTrackArtist;

   delete mArrowCursor;
   delete mSelectCursor;
   delete mSlideCursor;
   delete mResizeCursor;
   delete mZoomInCursor;
   delete mZoomOutCursor;

   // Note that the submenus (mChannelMenu and mRateMenu) are deleted by their parent
   delete mWaveTrackMenu;
   delete mNoteTrackMenu;
   delete mLabelTrackMenu;
}

void TrackPanel::SelectNone()
{
   TrackListIterator iter(mTracks);
   VTrack *t = iter.First();
   while (t) {
      t->SetSelected(false);

      if (t->GetKind() == VTrack::Label)
         ((LabelTrack *) t)->Unselect();

      t = iter.Next();
   }
}

void TrackPanel::GetTracksUsableArea(int *width, int *height) const
{
   GetSize(width, height);

   *width -= GetLabelWidth();

   // AS: MAGIC NUMBER: What does 2 represent?
   *width -= 2 + kLeftInset;
}

// AS: Ok, this is handling some kind of scrolling case.
// not sure exactly what, though.
void TrackPanel::TimerFunc1()
{
  mAutoScrolling = true;
  
  if (mMouseMostRecentX > mCapturedRect.x + mCapturedRect.width)
    mListener->TP_ScrollRight();
  else if (mMouseMostRecentX < mCapturedRect.x)
    mListener->TP_ScrollLeft();

  wxMouseEvent *e2 = new wxMouseEvent(wxEVT_MOTION);
  HandleSelect(*e2);
  delete e2;

  mAutoScrolling = false;
}

// AS: This seems to have something to do with drawing the ruler
// that appears at the top of the screen.  Although it might
// just be for drawing the blinky cursor thing that appears there.
void TrackPanel::TimerFunc2()
{
  double ind = gAudioIO->GetIndicator();
  bool onScreen = (ind >= mViewInfo->h
		   && ind <= (mViewInfo->h + mViewInfo->screen));
  
  if (mIndicatorShowing || onScreen) {
    mIndicatorShowing = (onScreen &&
			 gAudioIO->IsBusy() &&
			 gAudioIO->GetProject() ==
			 (AudacityProject *) GetParent());

    wxClientDC dc(this);
    
    int width, height;
    GetSize(&width, &height);
    height = GetRulerHeight();
    
    wxMemoryDC *memDC = new wxMemoryDC();
    wxBitmap *rulerBitmap = new wxBitmap();
    rulerBitmap->Create(width, height);
    
    memDC->SelectObject(*rulerBitmap);
    
    DrawRuler(memDC, true);
    
    dc.Blit(0, 0, width, height, memDC, 0, 0, wxCOPY, FALSE);
    
    delete memDC;
    delete rulerBitmap;
    
  }
}

// AS: This function draws the blinky cursor things, both in the
// ruler as seen at the top of the screen, but also in each of the
// selected tracks.
void TrackPanel::DrawCursors()
{
  // The difference between a wxClientDC and a wxPaintDC
  // is that the wxClientDC is used outside of a paint loop,
  // whereas the wxPaintDC is used inside a paint loop
  wxClientDC dc(this);
  dc.SetLogicalFunction(wxINVERT);
  dc.SetPen(*wxBLACK_PEN);
  dc.SetBrush(*wxBLACK_BRUSH);
  
  int x = GetLeftOffset() +
    int ((mViewInfo->sel0 - mViewInfo->h) * mViewInfo->zoom);
  
  int y = -mViewInfo->vpos + GetRulerHeight();
  

  // AS: Ah, no, this is where we draw the blinky thing in the ruler.
  if (!mIndicatorShowing) {
    // Draw cursor in ruler
    dc.DrawLine(x, 1, x, GetRulerHeight() - 2);
  }
  
  if (x >= GetLeftOffset()) {
    // Draw cursor in all selected tracks
    TrackListIterator iter(mTracks);
    for (VTrack *t = iter.First(); t; t = iter.Next()) {
      int height = t->GetHeight();
      if (t->GetSelected() && t->GetKind() != VTrack::Label)
	dc.DrawLine(x, y + kTopInset + 1, x, y + height - 2);
      y += height;
    }
  }
}

// AS: What's this do?  Presumably we're repainting stuff on
// every timer tick... ?
void TrackPanel::OnTimer()
{
   if (mIsSelecting && mCapturedTrack) {
     TimerFunc1();
   }
#ifdef BOUNCE
   if (gAudioIO->IsPlaying() &&
       gAudioIO->GetProject() == (AudacityProject *) GetParent())
      gBounce->SetTime(gAudioIO->GetIndicator());
#endif

   if (mIndicatorShowing ||
       (gAudioIO->IsBusy() &&
        gAudioIO->GetProject() == (AudacityProject *) GetParent())) {
     TimerFunc2();
   }

   // AS: Um, I get the feeling we want to redraw the cursors
   // every 10 timer ticks or something...
   mTimeCount = (mTimeCount + 1) % 10;
   if (mTimeCount == 0 &&
       !mTracks->IsEmpty() && mViewInfo->sel0 == mViewInfo->sel1) {
     DrawCursors();
   }
}

// AS: This is what gets called during the normal course of needing
// to repaint.
void TrackPanel::OnPaint(wxPaintEvent & event)
{
   wxPaintDC dc(this);
   int width, height;
   GetSize(&width, &height);
   if (width != mPrevWidth || height != mPrevHeight || !mBitmap) {
      mPrevWidth = width;
      mPrevHeight = height;

      if (mBitmap)
         delete mBitmap;

      mBitmap = new wxBitmap(width, height);
   }

   wxMemoryDC memDC;

   memDC.SelectObject(*mBitmap);

   DrawTracks(&memDC);
   DrawRuler(&memDC);

   dc.Blit(0, 0, width, height, &memDC, 0, 0, wxCOPY, FALSE);
}

void TrackPanel::MakeParentPushState(wxString desc)
{
   mListener->TP_PushState(desc);
}

void TrackPanel::MakeParentRedrawScrollbars()
{
   mListener->TP_RedrawScrollbars();
}


// AS: I added this function.  Don't know what it's for,
// just came as part of some refactoring...
bool TrackPanel::SetCursor1()
{
   if(mIsSelecting ) SetCursor(*mSelectCursor);
   else if(mIsSliding   ) SetCursor(*mSlideCursor );
   else if(mIsEnveloping) SetCursor(*mArrowCursor );
   else return false;

   return true;
}

// AS: Still bad.  This is dependant on select=0, envelope=1, 
// move/slide=2, and zoom=3.  And this should go somewhere else...
const char *pMessages[] = {"Click and drag to select audio", 
			   "Click and drag to edit the amplitude envelope",
			   "Click and drag to move a track in time",
#if defined( __WXMAC__ )
			   "Click to Zoom In, Shift-Click to Zoom Out"
#elif defined( __WXMSW__ )
			   "Left-Click to Zoom In, Right-Click to Zoom Out"
#elif defined( __WXGTK__ )
			   "Left=Zoom In, Right=Zoom Out, Middle=Normal"
#endif
};

// AS: I think this function is setting what icon your cursor is
// using.  Also, it's setting a message in the status bar or something.
void TrackPanel::HandleCursor(wxMouseEvent & event)
{
  if (SetCursor1()) return;

   wxRect r;
   int num;

   VTrack *t = FindTrack(event.m_x, event.m_y, false, &r, &num);

   if (t) {

      // First test to see if we're over the area that
      // resizes a track

     // AS: MAGIC NUMBER: What is 5?
      if (event.m_y >= (r.y + r.height - 5) &&
          event.m_y < (r.y + r.height + 5)) {

         mListener->
             TP_DisplayStatusMessage("Click and drag to resize the track",
                                     0);

         SetCursor(*mResizeCursor);
         return;
      }

      // Otherwise set the cursor based on the current tool
      int operation = mListener->TP_GetCurrentTool();
      mListener->TP_DisplayStatusMessage(
		 pMessages[operation], 0);

      // AS: Actually set the cursor based on what tool the
      // user has selected.
      assert(operation >= 0 && operation <= 3);
      const wxCursor *pCursors[] = {mSelectCursor, mArrowCursor, mSlideCursor };
      if (operation < 3)
	SetCursor(*pCursors[operation]);
      else if (operation == 3) {
	if (event.ShiftDown())
	  SetCursor(*mZoomInCursor);
	else
	  SetCursor(*mZoomOutCursor);
      }
   } else {
      // Not over a track
      SetCursor(*mArrowCursor);
   }
}

// AS: Extend the selection to x, where x is in the context of the 
// current viewable area of the selected track?  
void TrackPanel::ExtendSelection(int x1, int x2)
{
  double selend =
    mViewInfo->h + ((x1 - x2) / mViewInfo->zoom);
    
  if (selend > mViewInfo->sel1) {
    mViewInfo->sel1 = selend;
    mSelStart = mViewInfo->sel0;
  } else if (selend < mViewInfo->sel0) {
    mViewInfo->sel0 = selend;
    mSelStart = mViewInfo->sel1;
  } else {
    mViewInfo->sel1 = selend;
    mSelStart = mViewInfo->sel0;
  }
}

// AS: ??
void TrackPanel::OtherSelection(int x1, int x2)
{
  double selend = mViewInfo->h + ((x1 - x2) / mViewInfo->zoom);
  
  if (selend < 0.0)
    selend = 0.0;
  
  if (selend >= mSelStart) {
    mViewInfo->sel0 = mSelStart;
    mViewInfo->sel1 = selend;
  } else {
    mViewInfo->sel0 = selend;
    mViewInfo->sel1 = mSelStart;
  }
}

// AS: Start making a selection?
void TrackPanel::StartSelection(int x1, int x2)
{
  mSelStart =
    mViewInfo->h + ((x1 - x2) / mViewInfo->zoom);
  
  mViewInfo->sel0 = mSelStart;
  mViewInfo->sel1 = mSelStart;
}

// AS: ??
void TrackPanel::Augustus3(wxMouseEvent &event, VTrack* pTrack, wxRect r, int num)
{
  mCapturedTrack = pTrack;
  mCapturedRect = r;
  mCapturedNum = num;
  
  mMouseClickX = event.m_x;
  mMouseClickY = event.m_y;
  
  // AS: If the shift button is being held down, 
  // extend the current selection.
  if (event.ShiftDown()) {

    ExtendSelection(event.m_x, r.x);

    mListener->
      TP_DisplayStatusMessage(wxString::
			      Format("Selection: %lf - %lf s",
				     mViewInfo->sel0,
				     mViewInfo->sel1), 1);
  } else {  // AS: Otherwise, start a new selection
    
    StartSelection(event.m_x, r.x);

    SelectNone();
    mTracks->Select(pTrack);
  

    mListener->
      TP_DisplayStatusMessage(wxString::
			      Format("Cursor: %lf s", mSelStart),
			      1);
    
    mIsSelecting = true;
    
    if (pTrack->GetKind() == VTrack::Label)
      ((LabelTrack *) pTrack)->MouseDown(mMouseClickX, mMouseClickY,
				    mCapturedRect,
				    mViewInfo->h,
				    mViewInfo->zoom);
  }
}

// AS: I added this function during refactoring.  Not sure what it
// does.
void TrackPanel::Augustus1(wxMouseEvent &event)
{
  wxRect r; // AS: Presumably, the bounding rect for the Track?
  int num;  // AS: Great, what is num?
  
  // AS: Returns NULL if there is no track selected?
  VTrack *t = FindTrack(event.m_x, event.m_y, false, &r, &num);
  
  if (t)
    Augustus3(event, t, r, num);
  else 
    SelectNone();

  // AS: For some reason, we don't want to refresh?
  Refresh(false);
}

// AS: TODO: Comment this function.  hehe
void TrackPanel::ExtendSelection(wxMouseEvent &event)
{

  if (!mIsSelecting)
    return;
  
  if (event.Dragging() || mAutoScrolling) {
    wxRect r = mCapturedRect;
    int num = mCapturedNum;
    VTrack *pTrack = mCapturedTrack;
    
    // AS: Note that FindTrack will replace r and num's values.
    if (!pTrack)
      pTrack = FindTrack(event.m_x, event.m_y, false, &r, &num);
    
    if (pTrack) {      // Selecting
      int x = mAutoScrolling ? mMouseMostRecentX : event.m_x;
      int y = mAutoScrolling ? mMouseMostRecentY : event.m_y;

      OtherSelection(x, r.x);
      
      mListener->
	TP_DisplayStatusMessage(wxString::
				Format("Selection: %lf - %lf s",
				       mViewInfo->sel0,
				       mViewInfo->sel1), 1);
      
      // Handle which tracks are selected
      int num2;
      if (0 != FindTrack(x, y, false, NULL, &num2)) {
	// The tracks numbered num...num2 should be selected
	
	TrackListIterator iter(mTracks);
	VTrack *t = iter.First();
	for (int i = 1; t; i++, t = iter.Next()) {
	  if ((i >= num && i <= num2) || (i >= num2 && i <= num))
	    mTracks->Select(t);
	}
      }
      
      // AS: ??
      Refresh(false);
      
#ifdef __WXMAC__
      if (mAutoScrolling)
	MacUpdateImmediately();
#endif
    }
  }
}
  
// AS: This function deals with a TrackPanel becoming selected?  Gaining the Focus?
void TrackPanel::HandleSelect(wxMouseEvent & event)
{
   if (event.ButtonDown())
     Augustus1(event);

   ExtendSelection(event);

   if (event.ButtonUp()) {
      mCapturedTrack = NULL;
      mIsSelecting = false;
   }
}

// AS: I created this function while decomposing...
// What is this damn captured shit?  It's littered all
// over here, and I can't figure out what it's for...
void TrackPanel::Envelope1(wxMouseEvent &event)
{
  wxRect r;
  int num;
  mCapturedTrack = FindTrack(event.m_x, event.m_y, false, &r, &num);
  
  if (!mCapturedTrack) 
    return;
  
  mCapturedRect = r;
  mCapturedRect.y += kTopInset;
  mCapturedRect.height -= kTopInset;
  mCapturedNum = num;
}

// AS: I created this function while decomposing...
void TrackPanel::Envelope2(wxMouseEvent &event)
{
  //  assert (mCapturedTrack);
  if (!mCapturedTrack || mCapturedTrack->GetKind() != VTrack::Wave)
    return;

  WaveTrack* pwavetrack = dynamic_cast<WaveTrack*>(mCapturedTrack);
  assert(pwavetrack);
  Envelope *penvelope = pwavetrack->GetEnvelope();
  int display = pwavetrack->GetDisplay();
  bool needUpdate = false;
  
  // AS: Okay, what does this display?  The captured track, cast to a WaveTrack,
  // getting it's GetDisplay value...
  if (display <= 1) {
    bool dB = (display == 1);
    
    needUpdate = penvelope->MouseEvent(event, mCapturedRect,
				       mViewInfo->h, mViewInfo->zoom, dB);
    
    // If this track is linked to another track, make the identical
    // change to the linked envelope:
    WaveTrack *link = dynamic_cast<WaveTrack*>(mTracks->GetLink(mCapturedTrack));
    if (link && link->GetKind() == VTrack::Wave) {
      Envelope *e2 = link->GetEnvelope();
      e2->MouseEvent(event, mCapturedRect,
		     mViewInfo->h, mViewInfo->zoom, dB);
    }
  }
  
  if (needUpdate)
    Refresh(false);
}

// AS: HandleEnvelope gets called when the user is changing the
// amplitude envelope on a track?
void TrackPanel::HandleEnvelope(wxMouseEvent & event)
{
   if (!mCapturedTrack)
     Envelope1(event);

   Envelope2(event);

   if (event.ButtonUp()) {
      mCapturedTrack = NULL;
      MakeParentPushState("Adjusted envelope.");
   }
}

// AS: Yay, more decomposition!
void TrackPanel::Slide1(wxMouseEvent &event, double& totalOffset, wxString& name)
{
  totalOffset = 0;
  
  wxRect r;
  int num;
  
  VTrack *vt = FindTrack(event.m_x, event.m_y, false, &r, &num);
  name = vt->GetName();
  
  if (vt) {
    
    mCapturedTrack = vt;
    mCapturedRect = r;
    mCapturedNum = num;
    
    mMouseClickX = event.m_x;
    mMouseClickY = event.m_y;
    
    mSelStart = mViewInfo->h + ((event.m_x - r.x) / mViewInfo->zoom);
    mIsSliding = true;
  }
}

//AS: Decomposing...
void TrackPanel::Slide2(wxMouseEvent &event, double& totalOffset, wxString& name)
{
  double selend = mViewInfo->h +
    ((event.m_x - mCapturedRect.x) / mViewInfo->zoom);
  
  if (selend < 0.0)
    selend = 0.0;
  
  if (selend != mSelStart) {
    mCapturedTrack->Offset(selend - mSelStart);
    totalOffset += selend - mSelStart;
    
    VTrack *link = mTracks->GetLink(mCapturedTrack);
    if (link)
      link->Offset(selend - mSelStart);
    
    Refresh(false);
  }
  
  mSelStart = selend;
}

// AS: Guess what.
void TrackPanel::Slide3(double &totalOffset, wxString& name)
{
      mCapturedTrack = NULL;
      mIsSliding = false;
      MakeParentRedrawScrollbars();
      if(totalOffset > 0)
         MakeParentPushState(
            wxString::Format("Slid track '%s' %s %.02f seconds", 
                             name.c_str(),
                             totalOffset > 0 ? "right" : "left",
                             totalOffset > 0 ? totalOffset : -totalOffset));
} 

// AS: HandleSlide gets called... ?
void TrackPanel::HandleSlide(wxMouseEvent & event)
{
  // AS: Aw hell, why do we have friggin static variables here?!?
   static double totalOffset;
   static wxString name;

   if (event.ButtonDown())
     Slide1(event, totalOffset, name);

   if (!mIsSliding)
      return;

   if (event.Dragging() && mCapturedTrack)
     Slide2(event, totalOffset, name);

   if (event.ButtonUp())
     Slide3(totalOffset, name);
}

// AS: Refactored stuff...
void TrackPanel::Zoom1(wxMouseEvent &event)
{
  mZoomEnd = event.m_x;

  int zoomLength = mZoomEnd - mZoomStart;
  if (zoomLength < 0)
    zoomLength = - zoomLength;

  if (zoomLength > 3)
    mIsZooming = true;

  if (mIsZooming)
    Refresh(false);
}

// AS: Refactored
void TrackPanel::Zoom3(wxMouseEvent &event, wxRect &r)
{
  double left =
    mViewInfo->h + ((mZoomStart - r.x) / mViewInfo->zoom);
  double right = 
    mViewInfo->h + ((mZoomEnd - r.x) / mViewInfo->zoom);      
  
  mViewInfo->zoom *= mViewInfo->screen/(right-left);
  
  if (mViewInfo->zoom > 6000000)
    mViewInfo->zoom = 6000000;
  
  mViewInfo->h = left;
}

// AS: Yup
void TrackPanel::Zoom4(wxMouseEvent &event, wxRect &r)
{
  double center_h = mViewInfo->h + (event.m_x - r.x) / mViewInfo->zoom;
    
  if (event.RightUp() || event.RightDClick() || event.ShiftDown())
    mViewInfo->zoom /= 2.0;
  else
    mViewInfo->zoom *= 2.0;
    
  if (event.MiddleUp() || event.MiddleDClick())
    mViewInfo->zoom = 44100.0 / 512.0;
    
  if (mViewInfo->zoom > 6000000)
    mViewInfo->zoom = 6000000;
    
  double new_center_h =
    mViewInfo->h + (event.m_x - r.x) / mViewInfo->zoom;
  
  mViewInfo->h += (center_h - new_center_h);
}

// AS: Uh-huh.
void TrackPanel::Zoom2(wxMouseEvent &event)
{
  int zoomLength = mZoomEnd - mZoomStart;      
  if (zoomLength < 0) {
    zoomLength = - zoomLength;
    std::swap(mZoomEnd, mZoomStart);
  }

  wxRect r;
  int num;
  VTrack *t = FindTrack(event.m_x, event.m_y, false, &r, &num);

  if (mIsZooming || zoomLength > 3)
    Zoom3(event, r);
  else
    Zoom4(event, r);
  mIsZooming = false;
    
  if (mViewInfo->h < 0)
    mViewInfo->h = 0;

  MakeParentRedrawScrollbars();
  Refresh(false);
}

// AS: This function takes care of all zoom requests?
void TrackPanel::HandleZoom(wxMouseEvent &event)
{
   if (event.ButtonDown() || event.ButtonDClick()) {
      mZoomStart = event.m_x;
      mZoomEnd = event.m_x;
   }
   else if (event.Dragging())
     Zoom1(event);
   else if (event.ButtonUp())
     Zoom2(event);
}

// AS: I'm guessing that this gets called when the application
// or at least the TrackPanel is being closed??
// Ah, this is probably for when a given track gets the x...
void TrackPanel::HandleClosing(wxMouseEvent & event)
{
   VTrack *t = mCapturedTrack;
   wxRect r = mCapturedRect;

   wxRect closeRect;
   GetCloseBoxRect(r, closeRect);

   wxClientDC dc(this);

   if (event.Dragging())
      DrawCloseBox(&dc, r, closeRect.Inside(event.m_x, event.m_y));
   else if (event.ButtonUp()) {
      DrawCloseBox(&dc, r, false);
      if (closeRect.Inside(event.m_x, event.m_y))
         RemoveTrack(t);

      mIsClosing = false;
   }
}

// AS: Handle when the mute or solo button is pressed for some track.
void TrackPanel::HandleMutingSoloing(wxMouseEvent & event, bool solo)
{
   VTrack *t = mCapturedTrack;
   wxRect r = mCapturedRect;

   wxRect buttonRect;
   GetMuteSoloRect(r, buttonRect, solo);
   
   wxClientDC dc(this);

   if (event.Dragging())
      DrawMuteSolo(&dc, r, t, buttonRect.Inside(event.m_x, event.m_y), solo);
   else if (event.ButtonUp()) {

      if (buttonRect.Inside(event.m_x, event.m_y))
	{
	  if (solo) t->SetSolo(!t->GetSolo()); 
	  else t->SetMute(!t->GetMute());
	}

      DrawMuteSolo(&dc, r, t, false, solo);
      if (solo) mIsSoloing = false; 
      else mIsMuting = false;
   }
}

// AS: This function gets called when a user clicks on the
// title of a track, dropping down the menu.
void TrackPanel::DoPopupMenu(wxMouseEvent &event, wxRect& titleRect, 
			     VTrack* t, wxRect& r, int num)
{
  mPopupMenuTarget = t;
  {
    wxClientDC dc(this);
    SetLabelFont(&dc);
    DrawTitleBar(&dc, r, t, true);
  }
  bool canMakeStereo = false;
  VTrack *next = mTracks->GetNext(t);

  wxMenu *theMenu = NULL;

  if (t->GetKind() == VTrack::Wave) {
    theMenu = mWaveTrackMenu;
    if (next && !t->GetLinked() && !next->GetLinked() &&
	t->GetKind() == VTrack::Wave
	&& next->GetKind() == VTrack::Wave)
      canMakeStereo = true;
    
    theMenu->Enable(theMenu->FindItem
		    ("Make Stereo Track"), canMakeStereo);
    theMenu->Enable(theMenu->FindItem
		    ("Split Stereo Track"), t->GetLinked());
    theMenu->Enable(theMenu->FindItem("Mono"), !t->GetLinked());
    theMenu->Enable(theMenu->FindItem("Left Channel"), !t->GetLinked());
    theMenu->Enable(theMenu->FindItem("Right Channel"), !t->GetLinked());
    
    int display = ((WaveTrack *) t)->GetDisplay();
    
    theMenu->Enable(theMenu->FindItem("Waveform"), display != 0);
    theMenu->Enable(theMenu->FindItem("Waveform (dB)"), display != 1);
    theMenu->Enable(theMenu->FindItem("Spectrum"), display != 2);
    theMenu->Enable(theMenu->FindItem("Pitch (EAC)"), display != 3);
  }
  
  if (t->GetKind() == VTrack::Note)
    theMenu = mNoteTrackMenu;
  
  if (t->GetKind() == VTrack::Label)
    theMenu = mLabelTrackMenu;
  
  if (theMenu) {
    
    theMenu->Enable(theMenu->FindItem
		    ("Move Track Up"), mTracks->CanMoveUp(t));
    theMenu->Enable(theMenu->FindItem
		    ("Move Track Down"), mTracks->CanMoveDown(t));
    
#ifdef __WXMAC__
    ::InsertMenu(mRateMenu->GetHMenu(), -1);
#endif
    
    PopupMenu(theMenu, titleRect.x + 1,
	      titleRect.y + titleRect.height + 1);
    
#ifdef __WXMAC__
    ::DeleteMenu(mRateMenu->MacGetMenuId());
#endif
  }
  
  {
    VTrack *t2 = FindTrack(event.m_x, event.m_y, true, &r, &num);
    if (t2 == t) {
      wxClientDC dc(this);
      SetLabelFont(&dc);
      DrawTitleBar(&dc, r, t, false);
    }
  }
}

// AS: Oh boy, this one's complicated.
void TrackPanel::HandleLabelClick(wxMouseEvent & event)
{
   if (!(event.ButtonDown() || event.ButtonDClick()))
      return;

   wxRect r;
   int num;

   VTrack *t = FindTrack(event.m_x, event.m_y, true, &r, &num);

   if (!t) {
      SelectNone();
      Refresh(false);
      return;
   }

   bool second = false;
   if (!t->GetLinked() && mTracks->GetLink(t))
      second = true;

   if (event.ShiftDown()) {
      mTracks->Select(t, !t->GetSelected());
      Refresh(false);
      return;
   }

   wxRect closeRect;
   GetCloseBoxRect(r, closeRect);
   if (!second && closeRect.Inside(event.m_x, event.m_y)) {
      wxClientDC dc(this);
      DrawCloseBox(&dc, r, true);
      mIsClosing = true;
      mCapturedTrack = t;
      mCapturedRect = r;
      return;
   }

   wxRect titleRect;
   GetTitleBarRect(r, titleRect);
   if (!second && titleRect.Inside(event.m_x, event.m_y)) {
     DoPopupMenu(event, titleRect, t, r, num);
     return;
   }
   
   // Check Mute and Solo buttons on WaveTracks:
   if (!second && t->GetKind() == VTrack::Wave) {
     if (MuteSoloFunc(t, r, event, false) ||
	 MuteSoloFunc(t, r, event, true ))
       return;
   }      
   
   // If it's a NoteTrack, it has special controls
   if (!second && t && t->GetKind() == VTrack::Note) {
      wxRect midiRect;
      GetTrackControlsRect(r, midiRect);
      if (midiRect.Inside(event.m_x, event.m_y)) {
         ((NoteTrack *) t)->LabelClick(midiRect, event.m_x, event.m_y,
                                       event.RightDown()
                                       || event.RightDClick());
         Refresh(false);
         return;
      }
   }

   // If they weren't clicking on a particular part of a track label,
   // deselect other tracks and select this one.
   SelectNone();
   mTracks->Select(t);
   mViewInfo->sel0 = 0.0;
   mViewInfo->sel1 = mTracks->GetMaxLen();
   Refresh(false);
}

bool TrackPanel::MuteSoloFunc(VTrack *t, wxRect r, wxMouseEvent &event, bool solo)
{
  wxRect buttonRect;
  GetMuteSoloRect(r, buttonRect, solo);
  if (buttonRect.Inside(event.m_x, event.m_y)) {

    wxClientDC dc(this);
    DrawMuteSolo(&dc, r, t, true, solo);

    if (solo) mIsSoloing = true; 
    else mIsMuting = true;

    mCapturedTrack = t;
    mCapturedRect = r;

    return true;
  }

  return false;
}

void TrackPanel::HandleResize(wxMouseEvent & event)
{
   if (event.ButtonDown()) {
      wxRect r;
      int num;

      VTrack *t = FindTrack(event.m_x, event.m_y, false, &r, &num);

      if (t) {
         mCapturedTrack = t;
         mCapturedRect = r;
         mCapturedNum = num;

         mMouseClickX = event.m_x;
         mMouseClickY = event.m_y;

         mIsResizing = true;
         mInitialTrackHeight = t->GetHeight();
      }
   }

   if (!mIsResizing)
      return;

   if (event.Dragging()) {
      int delta = (event.m_y - mMouseClickY);
      int newTrackHeight = mInitialTrackHeight + delta;
      if (newTrackHeight < 20)
         newTrackHeight = 20;
      mCapturedTrack->SetHeight(newTrackHeight);
      Refresh(false);
   }

   if (event.ButtonUp()) {
      mCapturedTrack = NULL;
      mIsResizing = false;
      MakeParentRedrawScrollbars();
      MakeParentPushState("TrackPanel::HandleResize() FIXME!!");
   }
}

void TrackPanel::OnKeyEvent(wxKeyEvent & event)
{
  if (event.ControlDown()) {
    event.Skip();
    return;
  }
  
  TrackListIterator iter(mTracks);
  
  switch (event.KeyCode()) {
  case WXK_SPACE:
    mListener->TP_OnPlayKey();
    break;
    
  default:
    
    for (VTrack * t = iter.First(); t; t = iter.Next()) {
      if (t->GetKind() == VTrack::Label) {
	((LabelTrack *) t)->KeyEvent(mViewInfo->sel0, mViewInfo->sel1,
				     event);
	Refresh(false);
	MakeParentPushState("TrackPanel::OnKeyEvent() FIXME!!");
	return;
      }
    }  
  }
}

void TrackPanel::OnMouseEvent(wxMouseEvent & event)
{
  mListener->TP_HasMouse();
  
  if (!mAutoScrolling) {
    mMouseMostRecentX = event.m_x;
    mMouseMostRecentY = event.m_y;
  }
  
  if (event.ButtonDown()) {
    mCapturedTrack = NULL;
    
    wxActivateEvent *e = new wxActivateEvent();
    GetParent()->ProcessEvent(*e);
    delete e;
  }
  
  if (mIsClosing)
    HandleClosing(event);
  else if (mIsMuting)
    HandleMutingSoloing(event, false);
  else if (mIsSoloing)
    HandleMutingSoloing(event, true);
  else if (mIsResizing) {
    HandleResize(event);
    HandleCursor(event);
  }
  else
    {
      wxRect r;
      int dummy;
      
      FindTrack(event.m_x, event.m_y, false, &r, &dummy);
      
      if (event.ButtonDown() &&
	  event.m_y >= (r.y + r.height - 5) &&
	  event.m_y < (r.y + r.height + 5)) {
	HandleResize(event);
	HandleCursor(event);
	return;
      }
      
      if (!mCapturedTrack && event.m_x < GetLabelWidth()) {
	HandleLabelClick(event);
	HandleCursor(event);
	return;
      }
      
      switch (mListener->TP_GetCurrentTool()) {
      case 0: HandleSelect  (event); break;
      case 1: HandleEnvelope(event); break;
      case 2: HandleSlide   (event); break;
      case 3: HandleZoom    (event); break;
      }
      
      if ((event.Moving() || event.ButtonUp()) &&
	  !mIsSelecting && !mIsEnveloping && !mIsSliding) {
	HandleCursor(event);
      }
      if (event.ButtonUp()) {
	mCapturedTrack = NULL;
      }
    }
}

void TrackPanel::SetLabelFont(wxDC * dc)
{
   int fontSize = 10;
#ifdef __WXMSW__
   fontSize = 8;
#endif

   wxFont labelFont(fontSize, wxSWISS, wxNORMAL, wxNORMAL);
   dc->SetFont(labelFont);
}

void TrackPanel::DrawRuler(wxDC * dc, bool text)
{
   wxRect r;

   GetSize(&r.width, &r.height);
   r.x = 0;
   r.y = 0;
   r.height = GetRulerHeight() - 1;

   DrawRulerBorder(dc, r);

   if (mViewInfo->sel0 < mViewInfo->sel1)
     DrawRulerSelection(dc, r);

   DrawRulerMarks(dc, r, text);

   if (gAudioIO->IsBusy() &&
       gAudioIO->GetProject() == (AudacityProject *) GetParent())
     DrawRulerIndicator(dc);
}

void TrackPanel::DrawRulerBorder(wxDC* dc, wxRect &r)
{
   // Draw ruler border
   AColor::Medium(dc, false);
   dc->DrawRectangle(r);

   r.width--;
   r.height--;
   AColor::Bevel(*dc, true, r);

   dc->SetPen(*wxBLACK_PEN);
   dc->DrawLine(r.x, r.y + r.height + 1, r.x + r.width + 1,
                r.y + r.height + 1);
}

void TrackPanel::DrawRulerSelection(wxDC* dc, const wxRect r)
{
   // Draw selection
  double sel0 = mViewInfo->sel0 - mViewInfo->h +
    GetLeftOffset() / mViewInfo->zoom;
  double sel1 = mViewInfo->sel1 - mViewInfo->h +
    GetLeftOffset() / mViewInfo->zoom;
  
  if (sel0 < 0.0)
    sel0 = 0.0;
  if (sel1 > (r.width / mViewInfo->zoom))
    sel1 = r.width / mViewInfo->zoom;
  
  int p0 = int (sel0 * mViewInfo->zoom + 0.5);
  int p1 = int (sel1 * mViewInfo->zoom + 0.5);
  
  wxBrush selectedBrush;
  selectedBrush.SetColour(148, 148, 170);
  wxPen selectedPen;
  selectedPen.SetColour(148, 148, 170);
  dc->SetBrush(selectedBrush);
  dc->SetPen(selectedPen);
  
  wxRect sr;
  sr.x = p0;
  sr.y = 1;
  sr.width = p1 - p0 - 1;
  sr.height = GetRulerHeight() - 3;
  dc->DrawRectangle(sr);
}

void TrackPanel::DrawRulerMarks(wxDC *dc, const wxRect r, bool text)
{
   // Draw marks on ruler
   dc->SetPen(*wxBLACK_PEN);

   SetLabelFont(dc);

   int minSpace = 60;           // min pixels between labels

   wxString unitStr;
   double unit = 1.0;
   double base;

   while (unit * mViewInfo->zoom < minSpace)
      unit *= 2.0;
   while (unit * mViewInfo->zoom > minSpace * 2)
      unit /= 2.0;

   if (unit < 0.0005) {
      unitStr = "us";           // microseconds
      base = 0.000001;
   } else if (unit < 0.5) {
      unitStr = "ms";           // milliseconds
      base = 0.001;
   } else if (unit < 30.0) {
      unitStr = "s";            // seconds
      base = 1.0;
   } else if (unit < 1800.0) {
      unitStr = "m";            // minutes
      base = 60.0;
   } else {
      unitStr = "h";            // hours
      base = 3600.0;
   }

   unit = base;

   bool hand = true;

   while (unit * mViewInfo->zoom < minSpace) {
      unit *= (hand ? 5.0 : 2.0);
      hand = !hand;
   }
   while (unit * mViewInfo->zoom > minSpace * (hand ? 2.0 : 5.0)) {
      unit /= (hand ? 2.0 : 5.0);
      hand = !hand;
   }

   unit /= 4;

   double pos = mViewInfo->h - GetLeftOffset() / mViewInfo->zoom;
   int unitcount = (int) (pos / unit);

   dc->SetTextForeground(wxColour(0, 0, 204));

   for (int pixel = 0; pixel < r.width; pixel++) {

      if (((int) (floor(pos / unit))) > unitcount) {
         unitcount = (int) (floor(pos / unit));

         switch ((unitcount) % 4) {
         case 0:
            dc->DrawLine(r.x + pixel, r.y + 8, r.x + pixel,
                         r.y + r.height);

            if (text) {
               char str[100];
               sprintf(str, "%.1f%s", unitcount * unit / base,
                       (const char *) unitStr);

               dc->DrawText(str, r.x + pixel + 3, r.y + 2);
            }

            break;

         case 1:
         case 3:
            dc->DrawLine(r.x + pixel, r.y + r.height - 4, r.x + pixel,
                         r.y + r.height);
            break;

         case 2:
            dc->DrawLine(r.x + pixel, r.y + r.height - 6, r.x + pixel,
                         r.y + r.height);
            break;
         }
      }
      pos += 1.0 / mViewInfo->zoom;
   }
}

void TrackPanel::DrawRulerIndicator(wxDC *dc)
{
   // Draw indicator
  double ind = gAudioIO->GetIndicator();
  
  if (ind >= mViewInfo->h && ind <= (mViewInfo->h + mViewInfo->screen)) {
    int indp =
      GetLeftOffset() +
      int ((ind - mViewInfo->h) * mViewInfo->zoom);
    
    dc->SetPen(*wxTRANSPARENT_PEN);
    dc->SetBrush(*wxBLACK_BRUSH);
    
    int indsize = 6;
    
    wxPoint tri[3];
    tri[0].x = indp;
    tri[0].y = indsize + 1;
    tri[1].x = indp - indsize;
    tri[1].y = 1;
    tri[2].x = indp + indsize;
    tri[2].y = 1;
    
    dc->DrawPolygon(3, tri);
  }
}

void TrackPanel::GetCloseBoxRect(const wxRect r, wxRect & dest) const
{
   dest.x = r.x;
   dest.y = r.y;
   dest.width = 16;
   dest.height = 16;
}

void TrackPanel::GetTitleBarRect(const wxRect r, wxRect & dest) const
{
   dest.x = r.x + 16;
   dest.y = r.y;
   dest.width = GetTitleWidth() - r.x - 16;
   dest.height = 16;
}

void TrackPanel::GetMuteSoloRect(const wxRect r, wxRect &dest, bool solo) const
{
   dest.x = r.x + 8;
   dest.y = r.y + 34;
   dest.width = 36;
   dest.height = 16;

   if (solo) dest.x += 36 + 8;
}

void TrackPanel::GetTrackControlsRect(const wxRect r, wxRect & dest) const
{
   dest = r;
   dest.width = GetTitleWidth();
   dest.x += 4 + kLeftInset;
   dest.width -= (8 + kLeftInset);
   dest.y += 18 + kTopInset;
   dest.height -= (24 + kTopInset);
}

void TrackPanel::DrawCloseBox(wxDC * dc, const wxRect r, bool down)
{
   dc->SetPen(*wxBLACK_PEN);
   dc->DrawLine(r.x + 3, r.y + 3, r.x + 13, r.y + 13);  // close "x"
   dc->DrawLine(r.x + 13, r.y + 3, r.x + 3, r.y + 13);
   wxRect bev;
   GetCloseBoxRect(r, bev);
   bev.Inflate(-1, -1);
   AColor::Bevel(*dc, !down, bev);
}

void TrackPanel::DrawTitleBar(wxDC * dc, const wxRect r, VTrack * t, bool down)
{
   wxRect bev;
   GetTitleBarRect(r, bev);
   bev.Inflate(-1, -1);
   AColor::Bevel(*dc, true, bev);

   // Draw title text
   wxString titleStr = t->GetName();
   int allowableWidth = GetTitleWidth() - 38 - kLeftInset;
   long textWidth, textHeight;
   dc->GetTextExtent(titleStr, &textWidth, &textHeight);
   while (textWidth > allowableWidth) {
      titleStr = titleStr.Left(titleStr.Length() - 1);
      dc->GetTextExtent(titleStr, &textWidth, &textHeight);
   }
   dc->DrawText(titleStr, r.x + 19, r.y + 2);

   // Pop-up triangle
   dc->SetPen(*wxBLACK_PEN);
   int xx = r.x + GetTitleWidth() - 16 - kLeftInset;
   int yy = r.y + 5;
   int triWid = 11;
   while (triWid >= 1) {
      dc->DrawLine(xx, yy, xx + triWid, yy);
      xx++;
      yy++;
      triWid -= 2;
   }

   AColor::Bevel(*dc, !down, bev);
}

// AS: Drow the Mute or the Solo button, depending on the value of solo.
void TrackPanel::DrawMuteSolo(wxDC * dc, const wxRect r, VTrack *t, bool down, bool solo)
{
   wxRect bev;
   GetMuteSoloRect(r, bev, solo);
   bev.Inflate(-1, -1);
   (solo) ? AColor::Solo(dc, t->GetSolo(), t->GetSelected()) : 
            AColor::Mute(dc, t->GetMute(), t->GetSelected());
   dc->DrawRectangle(bev);

   long textWidth, textHeight;
   wxString str = (solo) ? "Solo" : "Mute";
   SetLabelFont(dc);
   dc->GetTextExtent(str, &textWidth, &textHeight);
   dc->DrawText(str, bev.x + (bev.width - textWidth)/2, bev.y + 2);

   AColor::Bevel(*dc, !down, bev);
}

// AS: Draw the actual track areas.  We only draw the borders
// and the little buttons and menues and whatnot here, the
// actual contents of each track are drawn by the TrackArtist.
void TrackPanel::DrawTracks(wxDC * dc)
{
   wxRect clip;
   GetSize(&clip.width, &clip.height);

   wxRect panelRect = clip;
   panelRect.y = -mViewInfo->vpos;

   // Make room for ruler
   panelRect.y += GetRulerHeight();
   panelRect.height -= GetRulerHeight();

   wxRect tracksRect = panelRect;
   tracksRect.x += GetLabelWidth();
   tracksRect.width -= GetLabelWidth();

   // AS: Presumably 1 indicates that we are using the "Envelope"
   // adjusting tool (for changing the amplitude envelope).
   bool envelopeFlag = (mListener->TP_GetCurrentTool() == 1);

   // The track artist actually draws the stuff inside each track
   mTrackArtist->DrawTracks(mTracks, *dc, tracksRect,
                            clip, mViewInfo, envelopeFlag);

   DrawEverythingElse(dc, panelRect, clip);
}

void TrackPanel::DrawEverythingElse(wxDC *dc, const wxRect panelRect, const wxRect clip)
{
   // We draw everything else
   TrackListIterator iter(mTracks);

   wxRect trackRect = panelRect;
   wxRect r;

   for (VTrack *t = iter.First();t;t = iter.Next())
     DrawEverythingElse(t, dc, r, trackRect );

   if (mIsZooming)
     DrawZooming(dc, clip);

   // Paint over the part below the tracks
   GetSize(&trackRect.width, &trackRect.height);
   AColor::Medium(dc, false);
   dc->DrawRectangle(trackRect);
}

// AS: Note that this is being called in a loop and that the parameter values are
// expected to be maintained each time through.
void TrackPanel::DrawEverythingElse(VTrack *t, wxDC *dc, wxRect &r, wxRect &trackRect)
{
  trackRect.height = t->GetHeight();

  // Draw label area
  SetLabelFont(dc);
  dc->SetTextForeground(wxColour(0, 0, 0));
  
  int labelw = GetLabelWidth();
  int vrul = GetVRulerOffset();
  
  // If this track is linked to the next one, display a common
  // border for both, otherwise draw a normal border
  r = trackRect;
  
  bool skipBorder = false;
  if (t->GetLinked())
    r.height += mTracks->GetLink(t)->GetHeight();
  else if (mTracks->GetLink(t))
    skipBorder = true;
  
  if (!skipBorder)
    DrawOutside(t, dc, r, labelw, vrul, trackRect);
  
  r = trackRect;
  r.x += GetVRulerOffset();
  r.y += kTopInset;
  r.width = GetVRulerWidth();
  r.height -= (kTopInset + 2);
  mTrackArtist->DrawVRuler(t, dc, r);
  
  trackRect.y += t->GetHeight();
}

void TrackPanel::DrawZooming(wxDC* dc, const wxRect clip)
{
  // Draw zooming indicator
  wxRect r;

  r.x = mZoomStart;
  r.y = -1;
  r.width = mZoomEnd - mZoomStart;
  r.height = clip.height+2;

  dc->SetBrush(*wxTRANSPARENT_BRUSH);
  dc->SetPen(*wxBLACK_DASHED_PEN);

  dc->DrawRectangle(r);
}

void TrackPanel::DrawOutside(VTrack *t, wxDC* dc, const wxRect rec, const int labelw, 
			     const int vrul, const wxRect trackRect)
{
  wxRect r = rec;

  DrawOutsideOfTrack     (t, dc, r);

  r.x += kLeftInset;
  r.y += kTopInset;
  r.width -= kLeftInset * 2;
  r.height -= kTopInset;

  FillInLabel            (t, dc, r, labelw);
  DrawBordersAroundTrack (t, dc, r, labelw, vrul);
  DrawShadow             (t, dc, r);

  r.width = GetTitleWidth();
  DrawCloseBox(dc, r, false);
  DrawTitleBar(dc, r, t, false);
  
  if (t->GetKind() == VTrack::Wave) {
    DrawMuteSolo(dc, r, t, false, false);
    DrawMuteSolo(dc, r, t, false, true );
  }
  
  r = trackRect;
  
  if (t->GetKind() == VTrack::Wave)
    dc->DrawText(TrackSubText(t), r.x + 6, r.y + 22);
  else if (t->GetKind() == VTrack::Note) {
    wxRect midiRect;
    GetTrackControlsRect(trackRect, midiRect);
    ((NoteTrack *) t)->DrawLabelControls(*dc, midiRect);
    
  }
}

void TrackPanel::DrawOutsideOfTrack(VTrack *t, wxDC* dc, const wxRect r)
{
  // Fill in area outside of the track
  AColor::Medium(dc, false);
  wxRect side = r;
  side.width = kLeftInset;
  dc->DrawRectangle(side);
  side = r;
  side.height = kTopInset;
  dc->DrawRectangle(side);
  side = r;
  side.x += side.width - kTopInset;
  side.width = kTopInset;
  dc->DrawRectangle(side);
  
  if (t->GetLinked()) {
    side = r;
    side.y += t->GetHeight() - 1;
    side.height = kTopInset + 1;
    dc->DrawRectangle(side);
  }
  
}

void TrackPanel::FillInLabel(VTrack *t, wxDC* dc, const wxRect r, const int labelw)
{
  // fill in label
  wxRect fill = r;
  fill.width = labelw - r.x;
  AColor::Medium(dc, t->GetSelected());
  dc->DrawRectangle(fill);
  
}

void TrackPanel::DrawBordersAroundTrack(VTrack *t, wxDC* dc, const wxRect r, const int vrul, const int labelw)
{
  // Borders around track and label area
  dc->SetPen(*wxBLACK_PEN);
  dc->DrawLine(r.x, r.y, r.x + r.width - 1, r.y);        // top
  dc->DrawLine(r.x, r.y, r.x, r.y + r.height - 1);       // left
  dc->DrawLine(r.x, r.y + r.height - 2, r.x + r.width - 1, r.y + r.height - 2);  // bottom
  dc->DrawLine(r.x + r.width - 2, r.y, r.x + r.width - 2, r.y + r.height - 1);   // right
  dc->DrawLine(vrul, r.y, vrul, r.y + r.height - 1);
  dc->DrawLine(labelw, r.y, labelw, r.y + r.height - 1); // between vruler and track
  
  if (t->GetLinked()) {
    int h1 = r.y + t->GetHeight() - kTopInset;
    dc->DrawLine(vrul, h1 - 2, r.x + r.width - 1, h1 - 2);
    dc->DrawLine(vrul, h1 + kTopInset, r.x + r.width - 1,
		 h1 + kTopInset);
  }
  
  dc->DrawLine(r.x, r.y + 16, GetTitleWidth(), r.y + 16);        // title bar
  dc->DrawLine(r.x + 16, r.y, r.x + 16, r.y + 16);       // close box
}

void TrackPanel::DrawShadow(VTrack *t, wxDC* dc, const wxRect r)
{
  // Shadow
  AColor::Dark(dc, false);
  dc->DrawLine(r.x + 1, r.y + r.height - 1, r.x + r.width, r.y + r.height - 1);  // bottom
  dc->DrawLine(r.x + r.width - 1, r.y + 1, r.x + r.width - 1, r.y + r.height);   // right
}

// AS: Returns the string to be displayed in the track label
// indicating whether the track is mono, left, right, or 
// stereo and what sample rate it's using.
wxString TrackPanel::TrackSubText(VTrack *t)
{
  wxString s =
    wxString::Format("%dHz",
		     (int) (((WaveTrack *) t)->GetRate() +
			    0.5));
  if (t->GetLinked())
    s = "Stereo, " + s;
  else {
    if (t->GetChannel() == VTrack::MonoChannel)
      s = "Mono, " + s;
    else if (t->GetChannel() == VTrack::LeftChannel)
      s = "Left, " + s;
    else if (t->GetChannel() == VTrack::RightChannel)
      s = "Right, " + s;
  }

  return s;
}

// AS: Handle the menu options that change a track between
// left channel, right channel, and mono.
int channels[] = {VTrack::LeftChannel, VTrack::RightChannel,
		  VTrack::MonoChannel};
const char* channelmsgs[] = {"'left' channel", "'right' channel",
			     "'mono'"};
void TrackPanel::OnChannelChange(wxEvent &event)
{
  int id = event.GetId();
  assert (id >= OnChannelLeftID && id <= OnChannelMonoID);
  assert (mPopupMenuTarget);
  mPopupMenuTarget->SetChannel(channels[id - OnChannelLeftID]);
  MakeParentPushState(
      wxString::Format("Changed '%s' to %s",
		       mPopupMenuTarget->GetName().c_str(),
		       channelmsgs[id - OnChannelLeftID]));
  mPopupMenuTarget = NULL;
  Refresh(false);
}

// AS: Split a stereo track into two tracks... ??
void TrackPanel::OnSplitStereo()
{
   assert (mPopupMenuTarget);
   mPopupMenuTarget->SetLinked(false);
   MakeParentPushState(
         wxString::Format("Split stereo track '%s'",
			  mPopupMenuTarget->GetName().c_str()));
   Refresh(false);
}

// AS: Merge two tracks into one steroe track ??
void TrackPanel::OnMergeStereo()
{
   assert (mPopupMenuTarget); 
   mPopupMenuTarget->SetLinked(true);
   VTrack *partner = mTracks->GetLink(mPopupMenuTarget);
   if (partner) {
     mPopupMenuTarget->SetChannel(VTrack::LeftChannel);
     partner->SetChannel(VTrack::RightChannel);
     MakeParentPushState(
	  wxString::Format("Made '%s' a stereo track",
			   mPopupMenuTarget->GetName().c_str()));
   } else
     mPopupMenuTarget->SetLinked(false);
   Refresh(false);
}

// AS: This actually removes the specified track.  Not sure where
// it gets called from, though.  Seems oddly placed among these handlers.
void TrackPanel::RemoveTrack(VTrack * toRemove)
{
   TrackListIterator iter(mTracks);
   
   VTrack *partner = mTracks->GetLink(toRemove);
   wxString name;

   VTrack *t;
   for (t = iter.First(); t && t != toRemove && t != partner;
	t = iter.Next())
     ;
     
   if (t && (t == toRemove || t == partner)) {
     name = t->GetName();
     t = iter.RemoveCurrent();
   }

   MakeParentPushState(wxString::Format("Removed track '%s.'", name.c_str()));
   MakeParentRedrawScrollbars();

   Refresh(false);
}

// AS: Set the Display mode based on the menu choice in the Track Menu.
// Note that gModes MUST BE IN THE SAME ORDER AS THE MENU CHOICES!!
const char* gModes[] = {"waveform", "waveformDB", "spectrum", "pitch"};
void TrackPanel::OnSetDisplay(wxEvent &event)
{
  int id = event.GetId();
  assert (id >= OnWaveformID && id <= OnPitchID);
  assert (mPopupMenuTarget && mPopupMenuTarget->GetKind() == VTrack::Wave);
  ((WaveTrack *) mPopupMenuTarget)->SetDisplay(id - OnWaveformID);
  VTrack *partner = mTracks->GetLink(mPopupMenuTarget);
  if (partner)
    ((WaveTrack *) partner)->SetDisplay(id - OnWaveformID);
  MakeParentPushState(
		      wxString::Format("Changed '%s' to %s display",
				       mPopupMenuTarget->GetName().c_str(),
				       gModes[id - OnWaveformID]));
  mPopupMenuTarget = NULL;
  Refresh(false);
}

// AS: Sets the sample rate for a track, and if it is linked to
// another track, that one as well.
void TrackPanel::SetRate(VTrack *pTrack, double rate)
{
  ((WaveTrack *) pTrack)->SetRate(rate);
  VTrack *partner = mTracks->GetLink(pTrack);
  if (partner)
    ((WaveTrack *) partner)->SetRate(rate);
  MakeParentPushState(
		      wxString::Format("Changed '%s' to %d Hz", 
				       pTrack->GetName().c_str(), rate));
}

// AS: Ok, this function handles the selection from the Rate
// submenu of the track menu, except for "Other" (see OnRateOther).
// gRates MUST CORRESPOND DIRECTLY TO THE RATES AS LISTED IN THE MENU!!
// IN THE SAME ORDER!!
int gRates[] = {8000, 11025, 16000, 22050, 44100, 48000};
void TrackPanel::OnRateChange(wxEvent &event)
{
  int id = event.GetId();
  assert(id >= OnRate8ID && id <= OnRate48ID); 
  assert (mPopupMenuTarget && mPopupMenuTarget->GetKind() == VTrack::Wave);

  SetRate(mPopupMenuTarget, gRates[id - OnRate8ID]);

  mPopupMenuTarget = NULL;
  MakeParentRedrawScrollbars();
  Refresh(false);
}

// AS: This function handles the case when the user selects "Other"
// from the Rate submenu on the Track menu.
void TrackPanel::OnRateOther()
{
  assert (mPopupMenuTarget && mPopupMenuTarget->GetKind() == VTrack::Wave);

  wxString defaultStr;
  defaultStr.Printf("%d",(int)(((WaveTrack *) mPopupMenuTarget)->GetRate()+0.5));

  // AS: TODO: REMOVE ARTIFICIAL CONSTANTS!!
  // AS: Make a real dialog box out of this!!
  double theRate;
  do
    {
      wxString rateStr =
	wxGetTextFromUser("Enter a sample rate in Hz (per second) between 1 and 100000:",
			  "Set Rate",
			  defaultStr);
      
      // AS: Exit if they type in nothing.
      if ("" == rateStr)
	return;
      
      rateStr.ToDouble(&theRate);
      if (theRate >= 1 && theRate <= 100000)
	break;
      else
	wxMessageBox("Invalid rate.");

    } while (1);
  
  SetRate(mPopupMenuTarget, theRate);
  
  mPopupMenuTarget = NULL;
  MakeParentRedrawScrollbars();
  Refresh(false);
}

// AS: Move a track up or down, depending.
const char* gMove[] = {"up", "down"};
void TrackPanel::OnMoveTrack(wxEvent &event)
{
  assert(event.GetId() == OnMoveUpID || event.GetId() == OnMoveDownID );
  if (mTracks->Move(mPopupMenuTarget, OnMoveUpID == event.GetId())) {
    MakeParentPushState(
	 wxString::Format("Moved '%s' %s", mPopupMenuTarget->GetName().c_str(),
			  gMove[event.GetId() - OnMoveUpID]));
    Refresh(false);
   }
}

// AS: This only applies to MIDI tracks.  Presumably, it shifts the
// whole sequence by an octave.
void TrackPanel::OnChangeOctave(wxEvent &event)
{
  assert (event.GetId() == OnUpOctaveID || event.GetId() == OnDownOctaveID);
  assert(mPopupMenuTarget->GetKind() == VTrack::Note);
  NoteTrack *t = dynamic_cast<NoteTrack*>(mPopupMenuTarget);
  
  bool bDown = (OnDownOctaveID == event.GetId());
  t->mBottomNote += (bDown) ? -12 : 12;
  if (t->mBottomNote < 0)
    t->mBottomNote = 0;
  else if (t->mBottomNote > 96)
    t->mBottomNote = 96;
  
  MakeParentPushState("TrackPanel::OnChangeOctave() FIXME!!");
  Refresh(false);
}

void TrackPanel::OnSetName()
{
   VTrack *t = mPopupMenuTarget;

   if (t) {
      wxString defaultStr = t->GetName();
      wxString newName = wxGetTextFromUser("Change track name to:",
                                           "Track Name",
                                           defaultStr);
      if (newName != "")
         t->SetName(newName);
      MakeParentPushState(
          wxString::Format("Renamed '%s' to '%s'",
                           defaultStr.c_str(), newName.c_str()));
      Refresh(false);
   }
}

VTrack *TrackPanel::FindTrack(int mouseX, int mouseY, bool label,
                              wxRect * trackRect, int *trackNum)
{
   wxRect r;
   r.x = 0;
   r.y = -mViewInfo->vpos;
   r.y += GetRulerHeight();
   r.y += kTopInset;
   GetSize(&r.width, &r.height);

   if (label) {
      r.width = GetLabelWidth() - kLeftInset;
   } else {
      r.x += GetLabelWidth() + 1;
      r.width -= GetLabelWidth() - 3;
   }

   TrackListIterator iter(mTracks);

   int n = 1;

   for (VTrack *t = iter.First(); t; r.y += r.height, n++, t = iter.Next()) {
      r.height = t->GetHeight();

      if (r.Inside(mouseX, mouseY)) {
         if (trackRect) {
            r.y -= kTopInset;
            if (label) {
               r.x += kLeftInset;
               r.width -= kLeftInset;
               r.y += kTopInset;
               r.height -= kTopInset;
            }
            *trackRect = r;
         }
         if (trackNum)
            *trackNum = n;
         return t;
      }
   }

   if (mouseY >= r.y && trackNum)
      *trackNum = n - 1;

   return NULL;
}
