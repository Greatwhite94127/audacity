/**********************************************************************

  Audacity: A Digital Audio Editor

  ChangePitch.h

  Vaughan Johnson, Dominic Mazzoni
  
  Change Pitch effect provides raising or lowering 
  the pitch without changing the tempo.

**********************************************************************/

#if USE_SOUNDTOUCH

#ifndef __AUDACITY_EFFECT_CHANGEPITCH__
#define __AUDACITY_EFFECT_CHANGEPITCH__

// wxWindows controls 
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/radiobox.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <wx/intl.h>

#include "SoundTouchEffect.h"


class wxString;


class EffectChangePitch:public EffectSoundTouch {

 public:
   EffectChangePitch();

   virtual wxString GetEffectName() {
      return wxString(_("Change Pitch..."));
   }
   
   virtual wxString GetEffectAction() {
      return wxString(_("Changing Pitch"));
   }
   
   // Useful only after PromptUser values have been set. 
   virtual wxString GetEffectDescription(); 

   virtual bool Init();

	// DeduceFrequency is Dominic's extremely cool trick (Vaughan sez so!) 
	// to set deduce m_FromFrequency from the samples at the beginning of 
	// the selection. Then we set some other params accordingly.
	virtual void DeduceFrequencies(); 

   virtual bool PromptUser();
   virtual bool Process();
   
 private:
   int				m_FromPitchIndex;		// pitch index, per PitchIndex
	bool				m_bWantPitchDown;		// up to ToPitchNum if false (default), else down
   int				m_ToPitchIndex;		// pitch index, per PitchIndex

	double			m_SemitonesChange;	// how many semitones to change pitch
	
   float				m_FromFrequency;		// starting frequency of selection
   float				m_ToFrequency;			// target frequency of selection

   double			m_PercentChange;		// percent change to apply to pitch

friend class ChangePitchDialog;
};

//----------------------------------------------------------------------------
// ChangePitchDialog
//----------------------------------------------------------------------------

class ChangePitchDialog:public wxDialog {
 public:
   ChangePitchDialog(EffectChangePitch * effect, 
							wxWindow * parent, wxWindowID id, 
							const wxString & title, 
							const wxPoint & pos = wxDefaultPosition, 
							const wxSize & size = wxDefaultSize, 
							long style = wxDEFAULT_DIALOG_STYLE);

   virtual bool Validate();
   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

 private:
	// calculations
	void Calc_ToFrequency(); // Update m_ToFrequency from m_FromFrequency & m_PercentChange.
	void Calc_ToPitchIndex(); // Update m_ToPitchIndex from new m_SemitonesChange.
	void Calc_SemitonesChange_fromPitches(); // Update m_SemitonesChange from new m_*PitchIndex-es.
	void Calc_SemitonesChange_fromPercentChange(); // Update m_SemitonesChange from new m_PercentChange.
	void Calc_PercentChange(); // Update m_PercentChange based on new m_SemitonesChange.

	// handlers
   void OnChoice_FromPitch(wxCommandEvent & event); 
	void OnRadioBox_PitchUpDown(wxCommandEvent & event);
   void OnChoice_ToPitch(wxCommandEvent & event); 

   void OnText_SemitonesChange(wxCommandEvent & event); 
   
	void OnText_FromFrequency(wxCommandEvent & event); 
   void OnText_ToFrequency(wxCommandEvent & event); 

	void OnText_PercentChange(wxCommandEvent & event);
   void OnSlider_PercentChange(wxCommandEvent & event);

   void OnPreview( wxCommandEvent &event );
   void OnOk(wxCommandEvent & event);
   void OnCancel(wxCommandEvent & event);

	// helper fns for controls
	void Update_RadioBox_PitchUpDown();
	void Update_Choice_ToPitch(); 

	void Update_Text_SemitonesChange(); 
	
	void Update_Text_ToFrequency(); 

	void Update_Text_PercentChange(); // Update control per current m_PercentChange.
   void Update_Slider_PercentChange(); // Update control per current m_PercentChange.

 private:
   bool m_bLoopDetect;
	EffectChangePitch * m_pEffect;

   // controls
   wxChoice *		m_pChoice_FromPitch;
	wxRadioBox *	m_pRadioBox_PitchUpDown;
   wxChoice *		m_pChoice_ToPitch;
   
   wxTextCtrl *	m_pTextCtrl_SemitonesChange;

	wxTextCtrl *	m_pTextCtrl_FromFrequency;
   wxTextCtrl *	m_pTextCtrl_ToFrequency;
   
	wxTextCtrl *	m_pTextCtrl_PercentChange;
   wxSlider *		m_pSlider_PercentChange;

 public:
	// effect parameters
   int		m_FromPitchIndex;		// pitch index, per PitchIndex
	bool		m_bWantPitchDown;		// up to ToPitchNum if false (default), else down
   int		m_ToPitchIndex;		// pitch index, per PitchIndex

	double	m_SemitonesChange;	// how many semitones to change pitch
	
   float		m_FromFrequency;		// starting frequency of selection
   float		m_ToFrequency;			// target frequency of selection

   double	m_PercentChange;		// percent change to apply to pitch
											// Slider is (-100, 200], but textCtrls can set higher.

 private:
   DECLARE_EVENT_TABLE()
};


#endif // __AUDACITY_EFFECT_CHANGEPITCH__

#endif // USE_SOUNDTOUCH
