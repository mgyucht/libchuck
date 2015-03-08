//-------------------------------------------------------------------------------------------------------
// VST Effect Plug-in
//
// Filename     : ChucKVST.h
// Created by   : Spencer Salazar
// Company      : CCRMA - Stanford
// Description  : 
//
// Date         : 4/5/12
//-------------------------------------------------------------------------------------------------------

#include "ChucKVST.h"
#include "libchuck.h"

#include <math.h>
#include <stdio.h>
#include <assert.h>

//-------------------------------------------------------------------------------------------------------
AudioEffect* createEffectInstance (audioMasterCallback audioMaster)
{
	return new ChucKVST (audioMaster);
}

//-------------------------------------------------------------------------------------------------------
ChucKVST::ChucKVST (audioMasterCallback audioMaster)
: AudioEffectX (audioMaster, 1, kNumParams)	// 1 program, 1 parameter only
{
    chuck_options options;
    libchuck_options_reset(&options);
    options.buffer_size = getBlockSize();
    options.adaptive_buffer_size = 0;
    options.num_channels = kNumOutputs;
    options.sample_rate = getSampleRate();
    options.slave = true;
    
    ck = libchuck_create(&options);
    
    input_buffer = new float[options.buffer_size*options.num_channels];
    output_buffer = new float[options.buffer_size*options.num_channels];
    
    libchuck_vm_start(ck);
    
	setNumInputs (kNumInputs);       // stereo in
	setNumOutputs (kNumOutputs);     // stereo out
	setUniqueID ('ChcK');            // identify
	canProcessReplacing ();          // supports replacing output
    
    
	vst_strncpy (programName, "Default", kVstMaxProgNameLen);	// default program name
    
	// internal state var declaration and initialization
	the_sample_rate = getSampleRate(); // read the Sample Rate from the VST host
    
    
    // initial knob values (position in the slider)
	OutputGainKnob = 0.5;   // must be in the [0,1] range
    ChuckItKnob = 0;
    
    // translate knob position into parameter values
	output_gain = dB2lin(SmartKnob::knob2value(OutputGainKnob, OGainLimits, OGainTaper));
    chuck_it = 0;
}

//-------------------------------------------------------------------------------------------------------
ChucKVST::~ChucKVST ()
{
    // TODO: free vm
    libchuck_destroy(ck);
    ck = NULL;
    
    if(input_buffer) { delete[] input_buffer; input_buffer = NULL; }
    if(output_buffer) { delete[] output_buffer; output_buffer = NULL; }
}

//-------------------------------------------------------------------------------------------------------
void ChucKVST::setProgramName (char* name)
{
	vst_strncpy (programName, name, kVstMaxProgNameLen);
}

//-----------------------------------------------------------------------------------------
void ChucKVST::getProgramName (char* name)
{
	vst_strncpy (name, programName, kVstMaxProgNameLen);
}

//-----------------------------------------------------------------------------------------
void ChucKVST::setParameter (VstInt32 index, float value)
{
	switch (index)
	{
        case kParamChuckIt:
            ChuckItKnob=value;
            
            if(value >= 0.999 && chuck_it == 0)
            {
                chuck_it = 1;
                libchuck_add_shred(ck, "", "SqrOsc s => dac; 1::day => now;");
            }
            else if(value <= 0.001 && chuck_it == 1)
            {
                chuck_it = 0;
                libchuck_remove_shred(ck, 1);
            }

            
            fprintf(stderr, "chuck it! %f\n", value);
            break;
        case kParamOutputGain:
            OutputGainKnob=value;
            output_gain = dB2lin(SmartKnob::knob2value(OutputGainKnob, OGainLimits, OGainTaper));
            fprintf(stderr, "output gain! %f\n", output_gain);
            break;
        default :
            break;
	}
}

//-----------------------------------------------------------------------------------------
float ChucKVST::getParameter (VstInt32 index)
{
	switch (index)
	{
        case kParamChuckIt:
            return ChuckItKnob;
            break;
        case kParamOutputGain:
            return OutputGainKnob;
            break;
        default :
            return 0.0;
	}
}

//-----------------------------------------------------------------------------------------
void ChucKVST::getParameterName (VstInt32 index, char* label)
{
	switch (index)
	{
        case kParamChuckIt:
            vst_strncpy(label, "ChucK it! ",kVstMaxParamStrLen);
            break;
        case kParamOutputGain:
            vst_strncpy(label, "Output Gain ",kVstMaxParamStrLen);
            break;
        default :
            *label = '\0';
            break;
	};
}

//-----------------------------------------------------------------------------------------
void ChucKVST::getParameterDisplay (VstInt32 index, char* text)
{
	switch (index)
	{
        case kParamChuckIt:
            float2string(chuck_it, text, kVstMaxParamStrLen);
            break;
        case kParamOutputGain:
            float2string(dB(output_gain), text, kVstMaxParamStrLen);
            break;
        default :
            *text = '\0';
            break;
	};
}

//-----------------------------------------------------------------------------------------
void ChucKVST::getParameterLabel (VstInt32 index, char* label)
{
	switch (index)
	{
        case kParamChuckIt:
            vst_strncpy(label, "", kVstMaxParamStrLen);
            break;
        case kParamOutputGain:
            vst_strncpy(label, "dB", kVstMaxParamStrLen);
            break;
        default :
            *label = '\0';
            break;
	};
}

bool ChucKVST::getParameterProperties (VstInt32 index, VstParameterProperties* p)
{
    switch (index)
    {
        case kParamChuckIt:
            p->flags = kVstParameterIsSwitch;
            return true;
            break;
        case kParamOutputGain:
            break;
        default :
            break;
    };
    
    return false;
}

//------------------------------------------------------------------------
bool ChucKVST::getEffectName (char* name)
{
	vst_strncpy (name, "ChucK", kVstMaxEffectNameLen);
	return true;
}

//------------------------------------------------------------------------
bool ChucKVST::getProductString (char* text)
{
	vst_strncpy (text, "ChucK", kVstMaxProductStrLen);
	return true;
}

//------------------------------------------------------------------------
bool ChucKVST::getVendorString (char* text)
{
	vst_strncpy (text, "Stanford CCRMA", kVstMaxVendorStrLen);
	return true;
}

//-----------------------------------------------------------------------------------------
VstInt32 ChucKVST::getVendorVersion ()
{ 
	return 1000; 
}

//-----------------------------------------------------------------------------------------
void ChucKVST::processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames)
{
    // copy input
    for(int i = 0; i < sampleFrames; i++)
    {
        input_buffer[i*2] = inputs[0][i];
        input_buffer[i*2+1] = inputs[1][i];
    }
    
    libchuck_slave_process(ck, input_buffer, output_buffer, sampleFrames);
    
    // copy output
    for (int i = 0; i < sampleFrames; i++)
    {
        outputs[0][i] = output_buffer[i*2]*output_gain;
        outputs[1][i] = output_buffer[i*2+1]*output_gain;
    }
}
