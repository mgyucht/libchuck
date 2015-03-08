//-------------------------------------------------------------------------------------------------------
// VST Effect Plug-in
//
// Filename     : Compressor.h
// Created by   : music424 staff
// Company      : CCRMA - Stanford
// Description  : 
//
// Date         : 4/5/12
//-------------------------------------------------------------------------------------------------------

#ifndef __compressor__
#define __compressor__

#include "public.sdk/source/vst2.x/audioeffectx.h"

#include <math.h>

struct chuck_inst;

#ifndef max
#define max(a,b)			(((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)			(((a) < (b)) ? (a) : (b))
#endif

#ifndef dB
// if below -100dB, set to -100dB to prevent taking log of zero
#define dB(x)               20.0 * ((x) > 0.00001 ? log10(x) : log10(0.00001))
#endif

#ifndef dB2lin
#define dB2lin(x)           pow( 10.0, (x) / 20.0 )
#endif

#define kMaxLen             32


//-------------------------------------------------------------------------------------------------------
// The VST plug-in
class ChucKVST : public AudioEffectX
{
public:
	ChucKVST (audioMasterCallback audioMaster);
	~ChucKVST();
    
	// Processing
	virtual void processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames);
    
	// Program
	virtual void setProgramName (char* name);
	virtual void getProgramName (char* name);
    
	// Parameters
	virtual void setParameter (VstInt32 index, float value);
	virtual float getParameter (VstInt32 index);
	virtual void getParameterLabel (VstInt32 index, char* label);
	virtual void getParameterDisplay (VstInt32 index, char* text);
	virtual void getParameterName (VstInt32 index, char* text);
    virtual bool getParameterProperties (VstInt32 index, VstParameterProperties* p);

	virtual bool getEffectName (char* name);
	virtual bool getVendorString (char* text);
	virtual bool getProductString (char* text);
	virtual VstInt32 getVendorVersion ();
    
protected:
	// param IDs
	enum {
		kParamChuckIt = 0,
		kParamOutputGain,
		kNumParams
	};
    
    
	// knob vars
    float ChuckItKnob;
	float OutputGainKnob;
	
	// config
	enum { 
		kNumProgs	= 1,
		kNumInputs	= 2,       // our process replacing assumes stereo inputs
		kNumOutputs	= 2
	};
    
	char	programName[kVstMaxProgNameLen + 1];
    
    
	// internal state var declaration and initialization
	float the_sample_rate;    // the sampling rate (this is usually set in the VST host,
                               // so don't hard-code it in your plug-in)
	float output_gain;        // output gain linear scale
    float chuck_it; // whether or not we are chucking it
    
    chuck_inst *ck;
    float *input_buffer;
    float *output_buffer;
};


//-------------------------------------------------------------------------------------------------------
// Parameters (knobs) ranges and scale types

// output filter gain limits, dB; taper, exponent
const static float OGainLimits[2] = {-30.0, 30.0};
const static float OGainTaper = 1.0; 


// "static" class to faciliate the knob handling
class SmartKnob {
public:
    // convert knob on [0,1] to value in [limits[0],limits[1]] according to taper
    static float knob2value(float knob, const float *limits, float taper)
    {
        float value;
        if (taper > 0.0) {  // algebraic taper
            value = limits[0] + (limits[1] - limits[0]) * pow(knob, taper);
        } else {            // exponential taper
            value = limits[0] * exp(log(limits[1]/limits[0]) * knob);
        }
        return value;
    };
    
    // convert value in [limits[0],limits[1]] to knob on [0,1] according to taper
    static float value2knob(float value, const float *limits, float taper)
    {
        float knob;
        if (taper > 0.0) {  // algebraic taper
            knob = pow((value - limits[0])/(limits[1] - limits[0]), 1.0/taper);
        } else {            // exponential taper
            knob = log(value/limits[0])/log(limits[1]/limits[0]);
        }
        return knob;
    };

};


#endif
