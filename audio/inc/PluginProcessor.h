/*
  ==============================================================================

    This file was auto-generated by the Introjucer!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#ifndef PLUGINPROCESSOR_H_INCLUDED
#define PLUGINPROCESSOR_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

#include "SynthParams.h"
#include "StepSequencer.h"

//==============================================================================
/**
*/
class Sequencer;
class PluginAudioProcessor  : public AudioProcessor, public SynthParams
{
public:
    //==============================================================================
    PluginAudioProcessor();
    ~PluginAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock (AudioSampleBuffer&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    const String getInputChannelName (int channelIndex) const override;
    const String getOutputChannelName (int channelIndex) const override;
    bool isInputChannelStereoPair (int index) const override;
    bool isOutputChannelStereoPair (int index) const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool silenceInProducesSilenceOut() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
private:
    //==============================================================================
    Synthesiser synth;

    void updateHostInfo();

    // StepSequencer main call
    void runSeq(AudioSampleBuffer& buffer, MidiBuffer& midiMessages);
    void stopSeq(MidiBuffer& midiMessages);
    bool seqIsPlaying = false;

    // StepSequencer gui params
    int midiSeq[8];
    int seqMode;
    int seqNumSteps;
    float seqStepSpeed;
    double seqNoteLength;

    // StepSequencer host sync mode
    void seqHostSync(AudioSampleBuffer& buffer, MidiBuffer& midiMessages);
    int seqNote = -1;
    double seqNextStep = 0.0;
    double stopNoteTime = 0.0;

    // StepSequencer no host sync mode
    void seqNoHostSync(AudioSampleBuffer& buffer, MidiBuffer& midiMessages);
    int nextPlaySample = 0;
    int noteOffSample = 0;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginAudioProcessor)
};


#endif  // PLUGINPROCESSOR_H_INCLUDED
