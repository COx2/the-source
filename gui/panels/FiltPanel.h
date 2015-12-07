/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.2.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_7B9503E899CF8C9A__
#define __JUCE_HEADER_7B9503E899CF8C9A__

//[Headers]     -- You can add your own extra header files here --
#include "JuceHeader.h"
#include "PanelBase.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class FiltPanel  : public PanelBase,
                   public SliderListener,
                   public ComboBoxListener
{
public:
    //==============================================================================
    FiltPanel (SynthParams &p);
    ~FiltPanel();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void sliderValueChanged (Slider* sliderThatWasMoved);
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<MouseOverKnob> cutoffSlider;
    ScopedPointer<MouseOverKnob> resonanceSlider;
    ScopedPointer<MouseOverKnob> FilterAttack;
    ScopedPointer<MouseOverKnob> FilterDecay;
    ScopedPointer<MouseOverKnob> FilterSustain;
    ScopedPointer<MouseOverKnob> FilterRelease;
    ScopedPointer<ComboBox> modSrc;
    ScopedPointer<Slider> modSliderCut;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FiltPanel)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

#endif   // __JUCE_HEADER_7B9503E899CF8C9A__
