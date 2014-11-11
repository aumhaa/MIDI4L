#include <map>
#include "RtMidi.h"
#include "maxcpp6.h"

const int MAX_PORT_NAME_SIZE = 1024;

class Example : public MaxCpp6<Example> {
public:
	Example(t_symbol * sym, long ac, t_atom * av) { 
		setupIO(2, 2); // inlets / outlets
        
        unsigned int nPorts;
        std::string portName;
        char cPortName[MAX_PORT_NAME_SIZE];
        std::map<std::string, int> inPortMap;
        std::map<std::string, int> outPortMap;
        
        try {
            midiin = new RtMidiIn();
        }
        catch ( RtMidiError &error ) {
            error("RtMidiIn constructor failure");
            error.printMessage(); // TODO: print this to the Max console
            return;
        }
        
        try {
            midiout = new RtMidiOut();
        }
        catch ( RtMidiError &error ) {
            error("RtMidiOut consturctor failure");
            error.printMessage(); // TODO: print this to the Max console
            return;
        }
        
        nPorts = midiin->getPortCount();
        post("MIDI input Port count: %u", nPorts);
        for ( unsigned int i=0; i<nPorts; i++ ) {
            try {
                portName = midiin->getPortName(i);
                strncpy(cPortName, portName.c_str(), MAX_PORT_NAME_SIZE);
                cPortName[MAX_PORT_NAME_SIZE - 1] = NULL;
                post("inport %u: %s", i, cPortName);
            }
            catch ( RtMidiError &error ) {
                error("Error getting MIDI input port name");
                error.printMessage(); // TODO: print this to the Max console
            }
           
        }
        
        post(" ");
        
        nPorts = midiout->getPortCount();
        post("MIDI output port count: %u", nPorts);
        for ( unsigned int i=0; i<nPorts; i++ ) {
            try {
                portName = midiout->getPortName(i);
                strncpy(cPortName, portName.c_str(), MAX_PORT_NAME_SIZE);
                cPortName[MAX_PORT_NAME_SIZE - 1] = NULL;
                post("outport %u: %s", i, cPortName);
            }
            catch (RtMidiError &error) {
                error("Error getting MIDI output port name");
                error.printMessage(); // TODO: print this to the Max console
            }
        }
	}
	
    ~Example() {
        if(midiin)  delete midiin;
        if(midiout) delete midiout;
    }
	
	// methods:
	void bang(long inlet) { 
		outlet_bang(m_outlets[0]);
	}
	void testfloat(long inlet, double v) { 
		outlet_float(m_outlets[0], v);
	}
	void testint(long inlet, long v) { 
		outlet_int(m_outlets[0], v);
	}
	void test(long inlet, t_symbol * s, long ac, t_atom * av) { 
		outlet_anything(m_outlets[1], gensym("test"), ac, av); 
	}
    
private:
    RtMidiIn*  midiin  = NULL;
    RtMidiOut* midiout = NULL;
};

C74_EXPORT int main(void) {
	// create a class with the given name:
	Example::makeMaxClass("example");
	REGISTER_METHOD(Example, bang);
	REGISTER_METHOD_FLOAT(Example, testfloat);
	REGISTER_METHOD_LONG(Example, testint);
	REGISTER_METHOD_GIMME(Example, test);
}