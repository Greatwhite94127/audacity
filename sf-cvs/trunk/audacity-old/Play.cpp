/**********************************************************************

  Audacity: A Digital Audio Editor

  Play.cpp

  Dominic Mazzoni

**********************************************************************/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include <wx/msgdlg.h>
#endif

#include "Play.h"
#include "Project.h"
#include "Track.h"
#include "WaveTrack.h"
#include "Mix.h"
#include "APalette.h"

SoundPlayer *gSoundPlayer;

void InitSoundPlayer()
{
  gSoundPlayer = new SoundPlayer();
  
}

SoundPlayer::SoundPlayer()
{
  mProject = NULL;
  mTracks = NULL;
  mStop = false;
  // Run our timer function once every 100 ms, i.e. 10 times/sec
  mTimer.Start(100, FALSE);
}

SoundPlayer::~SoundPlayer()
{
}

bool SoundPlayer::Begin(AudacityProject *project,
						TrackList *tracks,
						double t0, double t1)
{
  if (mProject)
	return false;

  mTracks = tracks;
  mT0 = t0;
  mT1 = t1;
  mT = mT0;

  mAudioOut.device = SND_DEVICE_AUDIO;
  mAudioOut.write_flag = SND_WRITE;
  mAudioOut.format.channels = 2;
  mAudioOut.format.mode = SND_MODE_PCM;
  mAudioOut.format.bits = 16;
  mAudioOut.format.srate = project->GetRate();
  strcpy(mAudioOut.u.audio.devicename,"");
  strcpy(mAudioOut.u.audio.interfacename,"");
  mAudioOut.u.audio.descriptor = 0;
  mAudioOut.u.audio.protocol = SND_COMPUTEAHEAD;
  mAudioOut.u.audio.latency = 1.0;
  mAudioOut.u.audio.granularity = 0.0;

  long flags = 0;
  int err = snd_open(&mAudioOut, &flags);

  if (err) {
	wxMessageBox(wxString::Format("Error opening audio device: %d",err));
	return false;
  }

  mTicks = 0;

  // Do this last because this is what signals the timer to go
  mProject = project;

  return true;
}

void SoundPlayer::Finish()
{
  snd_close(&mAudioOut);  

  gAPalette->SetPlay(false);
  gAPalette->SetStop(false);
  mStop = false;

  // TODO mProject->SoundDone();
  mProject = NULL;
}

void SoundPlayer::OnTimer()
{
  if (!mProject) return;

  if (mStop) {
	Finish();
	return;
  }

  if (mT>=mT1) {
      if (snd_flush(&mAudioOut) == SND_SUCCESS) {
        Finish();
        return;
      }
  }

  // TODO: Don't fill the buffer with more data every time
  // timer is called

    double deltat = 1.0;
    if (mT + deltat > mT1)
      deltat = mT1 - mT;

    int maxFrames = int(mProject->GetRate() * deltat);
    int block = snd_poll(&mAudioOut);

    #ifdef __WXGTK__
    // snd for Linux is not written yet...
    block = 44100;
    #endif

    if (block > maxFrames)
      block = maxFrames;

    if (block == 0) {
      return;
    }

    int i;

	Mixer *mixer = new Mixer(2, maxFrames, true);
	mixer->UseVolumeSlider(true);
	mixer->Clear();

    VTrack *vt = mTracks->First();
    while(vt) {
      if (vt->GetKind() == VTrack::Wave) {
	    WaveTrack *t = (WaveTrack *)vt;

		switch(t->channel) {
		case VTrack::LeftChannel:
		  mixer->MixLeft(t, mT, mT+deltat);
		  break;
		case VTrack::RightChannel:
		  mixer->MixRight(t, mT, mT+deltat);
		  break;
		case VTrack::MonoChannel:
		  mixer->MixMono(t, mT, mT+deltat);
		  break;
	    }
      }

      vt = mTracks->Next();
    }

	sampleType *outbytes = mixer->GetBuffer();
    snd_write(&mAudioOut, outbytes, block);

	delete mixer;

    mT += deltat;
}

void SoundPlayer::Stop()
{
  mStop = true;
}

bool SoundPlayer::IsBusy()
{
  return (mProject != NULL);
}



/*
bool SoundPlayer::Begin(WaveTrack *track, double t0, double t1)
{
  int s0 = (int)((t0 - track->tOffset) * track->rate);
  if (s0 < 0)
	  s0 = 0;
  int s1 = (int)((t1 - track->tOffset) * track->rate);
  if ((sampleCount)s1 > track->numSamples)
	  s1 = track->numSamples;

  if (s0 >= s1) {
    return true;
  }

  sampleCount len = s1 - s0;

  if (numSoundsPlaying != 0)
	return false;

  wxBusyCursor busy;

  numSoundsPlaying++;

  snd_node out;
  out.device = SND_DEVICE_AUDIO;
  out.write_flag = SND_WRITE;
  out.format.channels = 1;
  out.format.mode = SND_MODE_PCM;
  out.format.bits = 16;
  out.format.srate = track->rate;
  strcpy(out.u.audio.devicename,"");
  strcpy(out.u.audio.interfacename,""); //RBD 15jun00
  out.u.audio.descriptor = 0;
  out.u.audio.protocol = SND_COMPUTEAHEAD;
  out.u.audio.latency = 1.0;  // use defaults
  out.u.audio.granularity = 0.0;  // use defaults

  long flags = 0;
  int err = snd_open(&out, &flags);

  if (err) {
	wxString errStr;
	errStr.Printf("Error playing sound: %d",err);
	wxMessageBox(errStr);
    numSoundsPlaying--;
	return false;
  }

  int pos=0;

  sampleType *buffer = new sampleType[len];
  if (buffer) {
	track->Get(buffer, s0, len);

	int rslt;
	int p=0;
	while (p < len) {
	    // this loop is a busy-wait loop!
		rslt = snd_poll(&out); // wait for buffer space
		rslt = min(rslt, len - p);
		if (rslt) {
			snd_write(&out, &buffer[p], rslt);
			p += rslt;
		}
    }
	
	while(snd_flush(&out) != SND_SUCCESS)
	{
		// wait until it finishes...
	}

    delete[] buffer;
  }

  snd_close(&out);

  numSoundsPlaying--;
  return true;
}
*/

void SoundTimer::Notify()
{
  gSoundPlayer->OnTimer();
}
