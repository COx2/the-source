#pragma once

#include "JuceHeader.h"
#include <cmath>
#include "SynthParams.h"
#include "ModulationMatrix.h"
#include "Envelope.h"
#include "Oscillator.h"
#include "Filter.h"

class Sound : public SynthesiserSound {
public:
    bool appliesToNote(int /*midiNoteNumber*/) override { return true; }
    bool appliesToChannel(int /*midiChannel*/) override { return true; }
};

class Voice : public SynthesiserVoice {
public:
    Voice(SynthParams &p, int blockSize)
    : params(p)
    , filter({ { {p.filter[0],p.filter[1] },{ p.filter[0],p.filter[1] },{ p.filter[0],p.filter[1] } } })
    , envToVolume(getSampleRate(), params.envVol[0].attack, params.envVol[0].decay, params.envVol[0].sustain, params.envVol[0].release,
        params.envVol[0].attackShape, params.envVol[0].decayShape, params.envVol[0].releaseShape, params.envVol[0].keyVelToEnv)
    , env1(getSampleRate(), params.env[0].attack, params.env[0].decay, params.env[0].sustain, params.env[0].release,
        params.env[0].attackShape, params.env[0].decayShape, params.env[0].releaseShape, params.env[0].keyVelToEnv)
    , env2(getSampleRate(), params.env[1].attack, params.env[1].decay, params.env[1].sustain, params.env[1].release,
        params.env[1].attackShape, params.env[1].decayShape, params.env[1].releaseShape, params.env[1].keyVelToEnv)
    , modMatrix(p.globalModMatrix)
    , filterModBuffer(1, blockSize)
    , envToVolBuffer(1, blockSize)
    , env1Buffer(1, blockSize)
    , env2Buffer(1, blockSize)
    , modDestBuffer(destinations::MAX_DESTINATIONS, blockSize)
    {
        lfoBuffers[0] = AudioSampleBuffer(1, blockSize);
        lfoBuffers[1] = AudioSampleBuffer(1, blockSize);
        lfoBuffers[2] = AudioSampleBuffer(1, blockSize);

        std::fill(totSamples.begin(), totSamples.end(), 0);
        // set all to velocity so that the modulation matrix does not crash
        std::fill(modSources.begin(), modSources.end(), &currentVelocity);
        //std::fill(modSources.begin(), modSources.end(), nullptr);
        std::fill(modDestinations.begin(), modDestinations.end(), nullptr);

        //set connection bewtween source and matrix here
        // midi
        modSources[eModSource::eAftertouch] = &channelAfterTouch;
        modSources[eModSource::eKeyBipolar] = &keyBipolar;
        modSources[eModSource::eInvertedVelocity] = &currentInvertedVelocity;
        modSources[eModSource::eVelocity] = &currentVelocity;
        modSources[eModSource::eFoot] = &footControlValue;
        modSources[eModSource::eExpPedal] = &expPedalValue;
        modSources[eModSource::eModwheel] = &modWheelValue;
        modSources[eModSource::ePitchbend] = &pitchBend;
        // internal
        modSources[eModSource::eLFO1] = lfoBuffers[0].getWritePointer(0);
        modSources[eModSource::eLFO2] = lfoBuffers[1].getWritePointer(0);
        modSources[eModSource::eLFO3] = lfoBuffers[2].getWritePointer(0);
        modSources[eModSource::eVolEnv] = envToVolBuffer.getWritePointer(0);
        modSources[eModSource::eEnv2] = env1Buffer.getWritePointer(0);
        modSources[eModSource::eEnv3] = env2Buffer.getWritePointer(0);

        //set connection between destination and matrix here
        for (size_t u = 0; u < MAX_DESTINATIONS; ++u) {
            modDestinations[u] = modDestBuffer.getWritePointer(u);
        }
    }

    bool canPlaySound(SynthesiserSound* sound) override
    {
        ignoreUnused(sound);
        return true;
    }

    void startNote(int midiNoteNumber, float velocity,
        SynthesiserSound*, int currentPitchWheelPosition) override
    {
        currentVelocity = velocity;

        std::fill(totSamples.begin(), totSamples.end(), 0);

        // reset attackDecayCounter
        envToVolume.startEnvelope(currentVelocity);
        env1.startEnvelope(currentVelocity);
        env2.startEnvelope(currentVelocity);

        // Initialization of midi values
        channelAfterTouch = 0.f;
        keyBipolar = (static_cast<float>(midiNoteNumber) - 64.f) / 64.f;
        currentInvertedVelocity = 1.f - velocity;
        currentVelocity = velocity;
        footControlValue = 0.f;
        expPedalValue = 0.f;
        modWheelValue = 0.f;
        pitchBend = (currentPitchWheelPosition - 8192.0f) / 8192.0f;

        const float sRate = static_cast<float>(getSampleRate());
        float freqHz = static_cast<float>(MidiMessage::getMidiNoteInHertz(midiNoteNumber, params.freq.get()));

        // change the phases of both lfo waveforms, in case the user switches them during a note
        for (size_t l = 0; l < lfo.size(); ++l) {
            if (params.lfo[l].tempSync.get() == 1.f) {

                lfo[l].sine.phase = .5f*float_Pi;
                lfo[l].square.phase = 0.f;
                lfo[l].random.phase = 0.f;

                lfo[l].sine.phaseDelta = static_cast<float>(params.positionInfo[params.getGUIIndex()].bpm) /
                    (60.f*sRate)*(params.lfo[l].noteLength.get() / 4.f)*2.f*float_Pi;
                lfo[l].square.phaseDelta = static_cast<float>(params.positionInfo[params.getGUIIndex()].bpm) /
                    (60.f*sRate)*(params.lfo[l].noteLength.get() / 4.f)*2.f*float_Pi;
                lfo[l].random.phaseDelta = static_cast<float>(params.positionInfo[params.getGUIIndex()].bpm) /
                    (60.f*sRate)*(params.lfo[l].noteLength.get() / 4.f)*2.f*float_Pi;
                lfo[l].random.heldValue = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 2.f)) - 1.f;
        }
            else {
                lfo[l].sine.phase = .5f*float_Pi;
                lfo[l].sine.phaseDelta = params.lfo[l].freq.get() / sRate * 2.f * float_Pi;
                lfo[l].square.phase = .5f*float_Pi;
                lfo[l].square.phaseDelta = params.lfo[l].freq.get() / sRate * 2.f * float_Pi;

                lfo[l].random.phase = 0.f;
                lfo[l].random.phaseDelta = params.lfo[l].freq.get() / sRate * 2.f * float_Pi;
                lfo[l].random.heldValue = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 2.f)) - 1.f;
        }
            for (size_t o = 0; o < osc.size(); ++o) {
                // das macht ja hier nicht mehr so viel sinn, denn velocity ist hardcoded ...
                osc[o].level = Param::fromDb((velocity - 1.f) * params.osc[o].gainModAmount1.get());

            switch (params.osc[o].waveForm.getStep())
        {
            case eOscWaves::eOscSquare:
                osc[o].square.phase = 0.f;
                osc[o].square.phaseDelta = freqHz * Param::fromCent(params.osc[o].fine.get()) * Param::fromSemi(params.osc[o].coarse.get()) / sRate * 2.f * float_Pi;
                osc[o].square.width = params.osc[o].pulseWidth.get();
            break;
            case eOscWaves::eOscSaw:
                osc[o].saw.phase = 0.f;
                osc[o].saw.phaseDelta = freqHz * Param::fromCent(params.osc[o].fine.get()) * Param::fromSemi(params.osc[o].coarse.get()) / sRate * 2.f * float_Pi;
                osc[o].saw.trngAmount = params.osc[o].trngAmount.get();
            break;
            case eOscWaves::eOscNoise:
                break;
            }
        }
        }

        for (auto& filters : filter) 
        {
            for (Filter& f : filters) 
            {
                f.reset(sRate);
            }
        }
    }

    void stopNote(float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            // start a tail-off by setting this flag. The render callback will pick up on
            // this and do a fade out, calling clearCurrentNote() when it's finished.

            if (envToVolume.getReleaseCounter() == -1)      // we only need to begin a tail-off if it's not already doing so - the
            {                                               // stopNote method could be called more than once.
                envToVolume.resetReleaseCounter();
            }

            if (env1.getReleaseCounter() == -1)
            {
                env1.resetReleaseCounter();
            }
            if (env2.getReleaseCounter() == -1)
            {
                env2.resetReleaseCounter();
        }
        }
        else
        {
            // we're being told to stop playing immediately, so reset everything..
            clearCurrentNote();

            for (Lfo& l : lfo) {
                l.sine.reset();
                l.square.reset();
                l.random.reset();
            }

            for (Osc& o : osc) 
            {
                o.square.reset();
                o.saw.reset();
                o.noise.reset();
            }
        }
    }

    void channelPressureChanged(int newValue) override 
    {
        channelAfterTouch = static_cast<float>(newValue) / 127.f;
    }

    void pitchWheelMoved(int newValue) override
    {
        pitchBend = (newValue - 8192.f) / 8192.f;
    }

    //Midi Control
    void controllerMoved(int controllerNumber, int newValue) override
    {
        switch(controllerNumber)
        {
        //Modwheel
        case 1:
            modWheelValue = static_cast<float>(newValue) / 127.f;
            break;
        //Foot Controller
        case 4:
            footControlValue = (static_cast<float>(newValue) / 127.f); //TODO: test
            break;
        //Expression Control
        case 11:
            expPedalValue = static_cast<float>(newValue) / 127.f; //TODO: test
            break;
        }
    }

    void renderNextBlock(AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override
    {
        // if voice active
        if (lfo[0].sine.isActive() || lfo[0].square.isActive() ||
            lfo[1].sine.isActive() || lfo[1].square.isActive() ||
            lfo[2].sine.isActive() || lfo[2].square.isActive() ){

        // Modulation
        renderModulation(numSamples);

            const float *envToVolMod = envToVolBuffer.getReadPointer(0);

            for(int s = 0; s < numSamples; ++s) {

            // oscillators
            for (size_t o = 0; o < params.osc.size(); ++o) {

                const float *pitchMod = modDestBuffer.getReadPointer(DEST_OSC1_PI + o);
                const float *shapeMod = modDestBuffer.getReadPointer(DEST_OSC1_PW + o);
                    const float *panMod = modDestBuffer.getReadPointer(DEST_OSC1_PAN + o);

                    float currentSample = 0.0f;

                    switch (params.osc[o].waveForm.getStep())
                {
                    case eOscWaves::eOscSquare:
                    {
                        // In case of pulse width modulation
                        float deltaWidth = osc[o].square.width > .5f
                            ? params.osc[o].pulseWidth.getMax() - osc[o].square.width
                            : osc[o].square.width - params.osc[o].pulseWidth.getMin();
                        // Pulse width must not reach 0 or 1
                        if (deltaWidth > (.5f - params.osc[o].pulseWidth.getMin()) && deltaWidth < (.5f + params.osc[o].pulseWidth.getMin())) {
                            deltaWidth = .49f;
                        }

                        // LFO mod has values [-1 .. 1], max amp for amount = 1
                        deltaWidth = deltaWidth * shapeMod[s];
                        // Next sample will be fetched with the new width
                        currentSample = (osc[o].square.next(pitchMod[s], deltaWidth));
                    }
                    //currentSample = (osc1Sine.next(osc1PitchMod[s]));
                    break;
                    case eOscWaves::eOscSaw:
                    {
                        // In case of triangle modulation
                        float deltaTr = osc[o].saw.trngAmount > .5f
                            ? params.osc[o].trngAmount.getMax() - osc[o].saw.trngAmount
                            : osc[o].saw.trngAmount - params.osc[o].trngAmount.getMin();
                        // LFO mod has values [-1 .. 1], max amp for amount = 1
                        // deltaTr = deltaTr * shapeMod[s] * params.osc[o].shapeModAmount1.get();
                        deltaTr = deltaTr * shapeMod[s];
                        // Next sample will be fetch with the new width
                        currentSample = (osc[o].saw.next(pitchMod[s], deltaTr));
                    }
                    break;
                    case eOscWaves::eOscNoise:
                        currentSample = (osc[o].noise.next(pitchMod[s]));
                        break;
                }

                    // filter
                    for (size_t f = 0; f < params.filter.size(); ++f) 
                    {
                        const float *filterMod = modDestBuffer.getReadPointer(DEST_FILTER1_LC + f);
                        currentSample = filter[o][f].run(currentSample, filterMod[s]);
                    }

                    // gain + pan
                    currentSample *= envToVolMod[s];
                    const float currentAmp = params.osc[o].vol.get();

                    // check if the output is a stereo output
                    if (outputBuffer.getNumChannels() == 2){
                        // Pan Influence
                        const float currentPan = params.osc[o].panDir.get() + panMod[s] * 100.f;
                        //const float currentPan = panMod[s] * 100.f;
                        const float currentAmpRight = currentAmp + (currentAmp / 100.f * currentPan);
                        const float currentAmpLeft = currentAmp - (currentAmp / 100.f * currentPan);
                    outputBuffer.addSample(0, startSample + s, currentSample*currentAmpLeft);
                    outputBuffer.addSample(1, startSample + s, currentSample*currentAmpRight);
                }
                    else{
                    for (int c = 0; c < outputBuffer.getNumChannels(); ++c) {
                        outputBuffer.addSample(c, startSample + s, currentSample * currentAmp);
                    }
                }
                    if (static_cast<int>(getSampleRate() * params.envVol[0].release.get()) <= envToVolume.getReleaseCounter()) {
                        // next osc 
                        break;
                    }
                }
            }

            if (static_cast<int>(getSampleRate() * params.envVol[0].release.get()) <= envToVolume.getReleaseCounter()) {

                    clearCurrentNote();
            for (size_t l = 0; l < lfo.size(); ++l) {
                lfo[l].sine.reset();
                lfo[l].square.reset();
            }
            }
        }
        //Update of the total samples variable
    totSamples[0] = totSamples[0] + numSamples;
    }

protected:
    void renderModulation(int numSamples) {

        // Variables
        const float sRate = static_cast<float>(getSampleRate());
        std::array<float, 3> factorFadeInLFO;
        std::array<int, 3> samplesFadeInLFO;
        std::array<float, 3> lfoGain;

        // Init
        for (size_t l = 0; l < lfo.size(); ++l) {
            factorFadeInLFO[l] = 1.f;
            // Length in samples of the LFO fade in
            samplesFadeInLFO[l] = static_cast<int>(params.lfo[l].fadeIn.get() * sRate);
            // Lfo Gain
            lfoGain[l] = params.lfo[l].gainModSrc.get() == eModSource::eNone
            ? 1.f
                : *(modSources[static_cast<int>(params.lfo[l].gainModSrc.get()) - 1]);
        }

        //clear the buffers
        modDestBuffer.clear();
        lfoBuffers[0].clear();
        lfoBuffers[1].clear();
        lfoBuffers[2].clear();
        envToVolBuffer.clear();
        env1Buffer.clear();
        env2Buffer.clear();

        //set the write point in the buffers
        for (size_t u = 0; u < MAX_DESTINATIONS; ++u) {
            modDestinations[u] = modDestBuffer.getWritePointer(u);
        }

        modSources[eModSource::eLFO1] = lfoBuffers[0].getWritePointer(0);
        modSources[eModSource::eLFO2] = lfoBuffers[1].getWritePointer(0);
        modSources[eModSource::eLFO3] = lfoBuffers[2].getWritePointer(0);
        modSources[eModSource::eVolEnv] = envToVolBuffer.getWritePointer(0);
        modSources[eModSource::eEnv2] = env1Buffer.getWritePointer(0);
        modSources[eModSource::eEnv3] = env2Buffer.getWritePointer(0);

        //for each sample
        for (int s = 0; s < numSamples; ++s) {

            //calc lfo stuff
            for (size_t l = 0; l < lfoBuffers.size(); ++l) {
                
            // Fade in factor calculation
                if (samplesFadeInLFO[l] == 0 || (totSamples[l] + s > samplesFadeInLFO[l]))
            {
                // If the fade in is reached or no fade in is set, the factor is 1 (100%)
                    factorFadeInLFO[l] = 1.f;
            }
            else
            {
                // Otherwise the factor is determined
                    factorFadeInLFO[l] = static_cast<float>(totSamples[l] + s) / static_cast<float>(samplesFadeInLFO[l]);
            }

            // calculate lfo values and fill the buffers
                switch (params.lfo[l].wave.getStep()) {
                case eLfoWaves::eLfoSine:
                        lfoBuffers[l].setSample(0, s, lfo[l].sine.next() * factorFadeInLFO[l] * lfoGain[l]);
                    break;
                case eLfoWaves::eLfoSampleHold:
                        lfoBuffers[l].setSample(0, s, lfo[l].random.next() * factorFadeInLFO[l] * lfoGain[l]);
                    break;
                case eLfoWaves::eLfoSquare:
                        lfoBuffers[l].setSample(0, s, lfo[l].square.next() * factorFadeInLFO[l] * lfoGain[l]);
                    break;
                }
            }
            // Calculate the Envelope coefficients and fill the buffers
            envToVolBuffer.setSample(0, s, envToVolume.calcEnvCoeff());
            env1Buffer.setSample(0, s, env1.calcEnvCoeff());
            env2Buffer.setSample(0, s, env2.calcEnvCoeff());

            //run the matrix
            modMatrix.doModulationsMatrix(&*modSources.begin(), &*modDestinations.begin());

            for (size_t u = 0; u < MAX_DESTINATIONS; ++u) {
                ++modDestinations[u];
            }

            // internal modSources
            ++modSources[eModSource::eLFO1];
            ++modSources[eModSource::eLFO2];
            ++modSources[eModSource::eLFO3];
            ++modSources[eModSource::eVolEnv];
            ++modSources[eModSource::eEnv2];
            ++modSources[eModSource::eEnv3];

        //! \todo check whether this should be at the place where the values are actually used
            modDestBuffer.setSample(DEST_OSC1_PI, s, Param::fromSemi(modDestBuffer.getSample(DEST_OSC1_PI, s) * params.osc[0].pitchModAmount1.getMax()));
            modDestBuffer.setSample(DEST_OSC2_PI, s, Param::fromSemi(modDestBuffer.getSample(DEST_OSC2_PI, s) * params.osc[1].pitchModAmount1.getMax()));
            modDestBuffer.setSample(DEST_OSC3_PI, s, Param::fromSemi(modDestBuffer.getSample(DEST_OSC2_PI, s) * params.osc[2].pitchModAmount1.getMax()));
        }
    }

public:

private:
    SynthParams &params;

    struct Osc {
        Oscillator<&Waveforms::square> square;
        Oscillator<&Waveforms::saw> saw;
        Oscillator<&Waveforms::whiteNoise> noise;
        float level;
    };
    std::array<Osc, 3> osc;

    struct Lfo {
        Oscillator<&Waveforms::sinus> sine;
        Oscillator<&Waveforms::square> square;
        RandomOscillator<&Waveforms::square> random;
    };
    std::array<Lfo, 3> lfo;

    std::array<std::array<Filter,2>,3> filter;
    std::array<float*, eModSource::nSteps> modSources;
    std::array<float*, MAX_DESTINATIONS> modDestinations;
    std::array<int, 3> totSamples;

    // Midi
    float channelAfterTouch;
    float keyBipolar;
    float currentInvertedVelocity;
    float currentVelocity;
    float footControlValue;
    float expPedalValue;
    float modWheelValue;
    float pitchBend;

    //Mod matrix
    ModulationMatrix& modMatrix;

    // Buffers
    AudioSampleBuffer modDestBuffer;
    AudioSampleBuffer filterModBuffer;
    std::array<AudioSampleBuffer, 3> lfoBuffers; //todo: hardcoded size
    AudioSampleBuffer envToVolBuffer;
    AudioSampleBuffer env1Buffer;
    AudioSampleBuffer env2Buffer;
    // Envelopes
    Envelope envToVolume;
    Envelope env1;
    Envelope env2;
};
