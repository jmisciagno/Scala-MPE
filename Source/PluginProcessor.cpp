/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>

using namespace std;

class Scale {      
  public:             
    string description;
    int count;
    double scale_array[128];
    int i = 0;
};

Scale scale;
ifstream myfile("/Users/johnmisciagno/scala/scl/test.scl");
int midi_note;

int isComment(string str)
{
  return str.length() && str.at(0) == '!';
}

double interpretValue(string str)
{
    if (str.find('.') != std::string::npos)
    {
        return stof(str) / 100;
    }
    else
    {
      int pos = str.find("/");
      if (pos != string::npos)
      {
        string num = str.substr(0, pos);
        string den = str.substr(pos+1, str.length() - pos);
	    return 12.0 * log2(stof(num) / stof(den));
      }
      else
      {
	    return 12.0 * log2(stof(str));
      }
    }
}

int interpretLine(Scale *scale, string line, int line_num)
{
  if (line_num == 0)
  {
    scale->description = line;
  }
  else if (line_num == 1)
  {
    scale->count = stoi(line);
  }
  else
  {
    if (scale->i == 0)
    {
      scale->scale_array[0] = 0.0;
      scale->i++;
    }
    if (scale->i < 12)
    {
      scale->scale_array[scale->i++] = interpretValue(line);
    }
  }
    return 0;
}

int interpretFile(Scale *scale, ifstream *file)
{
  string line;
  int line_num = 0;
  if (file->is_open())
  {
    while (getline(*file, line))
    {
      if (!isComment(line))
      {
	    interpretLine(scale, line, line_num);
        line_num++;
      }
    }
    file->close();
    cout << "\n";
    return 0;
  }

  else
  {
    cout << "Unable to open file\n";
    return 1;
  }
}

double scale_value(double value, double dmin, double dmax, double cmin, double cmax)
{
  double drange = (dmax - dmin);
  double crange = (cmax - cmin);
  return (((value - dmin) * crange) / drange) + cmin;
}

double semitones_to_pitchbend(double value)
{
  return value * (170 + 2/3);
}

double pitchbend_to_semitones(double value)
{
  return value / (170 + 2/3);
}

double midi_note_scala(int midi_note)
{
  return scale.scale_array[midi_note % scale.count] + (floor(midi_note / scale.count) * 12);
}

double new_pitchbend(int midi_note, int pitchbend)
{
  double midi_note_f = midi_note + pitchbend_to_semitones(pitchbend - 8192);
  double new_midi_note_f = scale_value(midi_note_f, floor(midi_note_f), ceil(midi_note_f + .001), midi_note_scala(floor(midi_note_f)), midi_note_scala(ceil(midi_note_f + .001)));
  return 8192 + semitones_to_pitchbend(new_midi_note_f - midi_note);
}


//==============================================================================
NewProjectAudioProcessor::NewProjectAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    interpretFile(&scale, &myfile);
}

NewProjectAudioProcessor::~NewProjectAudioProcessor()
{
}

//==============================================================================
const juce::String NewProjectAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool NewProjectAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool NewProjectAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool NewProjectAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double NewProjectAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int NewProjectAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int NewProjectAudioProcessor::getCurrentProgram()
{
    return 0;
}

void NewProjectAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String NewProjectAudioProcessor::getProgramName (int index)
{
    return {};
}

void NewProjectAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void NewProjectAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void NewProjectAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NewProjectAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void NewProjectAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();
    
    juce::MidiBuffer processedMidi;
                
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        const auto time = metadata.samplePosition;
 
        if (message.isNoteOn())
        {
            message = juce::MidiMessage::noteOn (message.getChannel(),
                                                 message.getNoteNumber(),
                                                 message.getVelocity());
            midi_note = message.getNoteNumber();
            double updated_pitchbend = new_pitchbend(midi_note, 8192);
            processedMidi.addEvent (juce::MidiMessage::pitchWheel(message.getChannel(), updated_pitchbend), time);
            processedMidi.addEvent (message, time);
        }
        if (message.isNoteOff())
        {
            message = juce::MidiMessage::noteOff (message.getChannel(),
                                                  message.getNoteNumber(),
                                                  message.getVelocity());
            processedMidi.addEvent (message, time);
        }
        if (message.isPitchWheel()) // 0 - 16384 (Roli has range of 4 octaves), 8192 is neutral
        {
            double pitchbend = message.getPitchWheelValue();
            double updated_pitchbend = new_pitchbend(midi_note, pitchbend);
            
            processedMidi.addEvent (juce::MidiMessage::pitchWheel(message.getChannel(), updated_pitchbend), time);
        }
    }
    midiMessages.swapWith (processedMidi);
}

//==============================================================================
bool NewProjectAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* NewProjectAudioProcessor::createEditor()
{
    return new NewProjectAudioProcessorEditor (*this);
}

//==============================================================================
void NewProjectAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void NewProjectAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NewProjectAudioProcessor();
}
