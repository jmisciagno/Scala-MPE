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

int isComment(string str)
{
  return str.length() && str.at(0) == '!';
}

double interpretValue(string str)
{
    if (str.find('.') != string::npos)
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
  if (file->good())
  {
    bool has_error = true;
    while (getline(*file, line))
    {
      has_error = false;
      if (!isComment(line))
      {
	    interpretLine(scale, line, line_num);
        line_num++;
      }
    }
    cout << "\n";
    return has_error; // return 1 if error
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
  return value * (512/3);
}

double pitchbend_to_semitones(double value)
{
  return value / (512/3);
}

double midi_note_scala(Scale *scale, int midi_note)
{
  return scale->scale_array[midi_note % scale->count] + (floor(midi_note / scale->count) * 12);
}

double new_pitchbend(Scale *scale, int midi_note, int pitchbend)
{
  double midi_note_f = midi_note + pitchbend_to_semitones(pitchbend - 8192);
  double new_midi_note_f = scale_value(midi_note_f,
                                       floor(midi_note_f),
                                       ceil(midi_note_f + .001),
                                       midi_note_scala(scale, floor(midi_note_f)),
                                       midi_note_scala(scale, ceil(midi_note_f + .001)));
  return semitones_to_pitchbend(new_midi_note_f - midi_note) + 8192;
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
    state = juce::ValueTree ("ScalaMPE");
}

NewProjectAudioProcessor::~NewProjectAudioProcessor()
{
}

//==============================================================================
int NewProjectAudioProcessor::loadFile(string filename)
{
    ifstream myfile(filename);
    loaded = !interpretFile(&scale, &myfile);  // file was loaded if there is no error
    myfile.close();
    return !loaded;  // return 1 if error;
}

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
                
    if (!loaded) return;  // Do nothing if file was note loaded.
    
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        const auto time = metadata.samplePosition;
 
        if (message.isNoteOn())
        {
            message = juce::MidiMessage::noteOn (message.getChannel(),
                                                 message.getNoteNumber(),
                                                 message.getVelocity());
            midi_note[message.getChannel()-1] = message.getNoteNumber();
            double updated_pitchbend = new_pitchbend(&scale, midi_note[message.getChannel()-1], 8192);
            processedMidi.addEvent (juce::MidiMessage::pitchWheel(message.getChannel(), updated_pitchbend), time);
            processedMidi.addEvent (message, time);
        }
        else if (message.isNoteOff())
        {
            message = juce::MidiMessage::noteOff (message.getChannel(),
                                                  message.getNoteNumber(),
                                                  message.getVelocity());
            processedMidi.addEvent(message, time);
        }
        else if (message.isPitchWheel()) // 0 - 16384 (Roli has range of 4 octaves), 8192 is neutral
        {
            double pitchbend = message.getPitchWheelValue();
            double updated_pitchbend = new_pitchbend(&scale, midi_note[message.getChannel()-1], pitchbend);
            
            processedMidi.addEvent(juce::MidiMessage::pitchWheel(message.getChannel(), updated_pitchbend), time);
        }
        else
        {
            processedMidi.addEvent(message, time);
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
    // Save text in tree
    if (error) 
    {
        state.removeProperty("path", nullptr);
    }
    if (!error) state.setProperty("path", juce::var(path), nullptr);
   
    // Save tre
    juce::MemoryOutputStream stream(destData, false);
    state.writeToStream (stream);  
}

void NewProjectAudioProcessor::setStateInformation (const void* data, int sizeInBytes) 
{
    // get editor    
    NewProjectAudioProcessorEditor *editor =
        dynamic_cast<NewProjectAudioProcessorEditor*>(getActiveEditor()); 
       
    // Load tree
    juce::ValueTree tree = juce::ValueTree::readFromData(data, sizeInBytes); 
    
    if (tree.isValid())  
    {
        // Load state 
         state = tree;
          
        // Load path
        if (tree.hasProperty("path"))
        {  
           path =  state.getProperty("path").toString().toStdString();
           error = this->loadFile(path);
            if (error)
            { 
                message = "Error loading file...reverting to 12-ET.";
                if (editor != NULL) editor->errorText.setColour (juce::Label::textColourId, juce::Colours::orange);
                if (editor != NULL) editor->errorText.setText (message, juce::dontSendNotification);
            }
            else 
            { 
                string filename =  path.substr(path.find_last_of("/\\") + 1);
                message = "Sucessfully loaded: " + filename;
                if (editor != NULL) editor->errorText.setColour (juce::Label::textColourId, juce::Colours::lightgreen);
                if (editor != NULL) editor->errorText.setText (message, juce::dontSendNotification);
            }
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NewProjectAudioProcessor();
}
